// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stack>
#include <unordered_map>
#include "profiler_pal.h"
#include "BPerfProfiler.h"
#include "strsafe.h"
#include "corhlpr.h"
#include "CComPtr.h"
#include "AssemblyLoadEvent.h"
#include "AssemblyUnloadEvent.h"
#include "ModuleLoadEvent.h"
#include "ModuleUnloadEvent.h"
#include "JITCompilationStartedEvent.h"
#include "JITCompilationFinishedEvent.h"
#include "MethodUnloadEvent.h"
#include "CQuickBytes.h"
#include "sha1.h"
#include "RundownList.h"

HRESULT BPerfProfiler::AssemblyLoadUnloadData(AssemblyID assemblyId, bool load)
{
    HRESULT hr;
    ULONG assemblyNameLength = 0;
    AppDomainID appDomainId;
    ModuleID moduleId;

    IfFailRet(this->corProfilerInfo->GetAssemblyInfo(assemblyId, 0, &assemblyNameLength, nullptr, &appDomainId, &moduleId));
    portable_wide_string assemblyName(assemblyNameLength, ZEROSTRING);
    IfFailRet(this->corProfilerInfo->GetAssemblyInfo(assemblyId, assemblyNameLength, nullptr, addr(assemblyName), nullptr, nullptr));

    CComPtr<IMetaDataAssemblyImport> metadataAssemblyImport;
    IfFailRet(this->corProfilerInfo->GetModuleMetaData(moduleId, ofRead, IID_IMetaDataAssemblyImport, reinterpret_cast<IUnknown **>(&metadataAssemblyImport)));

    mdAssembly mda;
    metadataAssemblyImport->GetAssemblyFromScope(&mda);

    ASSEMBLYMETADATA assemblyMetadata;
    DWORD assemblyFlags;
    const void *publicKey;
    ULONG publicKeySize;
    ULONG hashAlgorithm;

    assemblyMetadata.cbLocale = 0;
    assemblyMetadata.ulProcessor = 0;
    assemblyMetadata.ulOS = 0;

    IfFailRet(metadataAssemblyImport->GetAssemblyProps(mda, &publicKey, &publicKeySize, &hashAlgorithm, nullptr, 0, nullptr, &assemblyMetadata, &assemblyFlags));

    portable_wide_char publicKeyToken[17];

    if ((assemblyFlags & afPublicKey) != 0)
    {
        constexpr BYTE ecmaPublicKey[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        if (memcmp(publicKey, ecmaPublicKey, publicKeySize) == 0)
        {
            StringCchPrintf(publicKeyToken, 17, W("b77a5c561934e089"));
        }
        else
        {
            SHA1Hash sha1Hash;
            sha1Hash.AddData(PBYTE(publicKey), publicKeySize);
            PBYTE hash = sha1Hash.GetHash() + SHA1_HASH_SIZE - 8; // we only care about the last 8 bytes

            StringCchPrintfW(publicKeyToken, 17, W("%02x%02x%02x%02x%02x%02x%02x%02x"), hash[7], hash[6], hash[5], hash[4], hash[3], hash[2], hash[1], hash[0]);
        }
    }
    else
    {
        StringCchPrintfW(publicKeyToken, 5, W("null"));
    }

    portable_wide_string culture(assemblyMetadata.cbLocale, ZEROSTRING);
    assemblyMetadata.szLocale = addr(culture);
    IfFailRet(metadataAssemblyImport->GetAssemblyProps(mda, &publicKey, &publicKeySize, &hashAlgorithm, nullptr, 0, nullptr, &assemblyMetadata, &assemblyFlags));

    auto maxPossibleStringLength = assemblyName.length() + culture.length() + 120; // 120 is the length of formatstring + max possible version number lengths + publickeytoken + possible "neutral" culture
    portable_wide_string fullyQualifiedAssemblyName(maxPossibleStringLength, ZEROSTRING);
    StringCchPrintfW(addr(fullyQualifiedAssemblyName), maxPossibleStringLength, W("%s, Version=%d.%d.%d.%d, Culture=%s, PublicKeyToken=%s"), assemblyName.c_str(), assemblyMetadata.usMajorVersion, assemblyMetadata.usMinorVersion, assemblyMetadata.usBuildNumber, assemblyMetadata.usRevisionNumber, culture.length() == 0 ? W("neutral") : culture.c_str(), publicKeyToken);

    size_t realLength;
    StringCchLengthW(fullyQualifiedAssemblyName.c_str(), maxPossibleStringLength, &realLength);
    fullyQualifiedAssemblyName.resize(realLength + 1); // shrink since we over-estimate the size, this is required for correctness

    if (load)
    {
        auto temp = this->assemblyRundown.Add(LogEvent2<AssemblyLoadEvent>(this->eventQueue, assemblyId, appDomainId, 0, assemblyFlags, fullyQualifiedAssemblyName, this->clrInstanceId));

        {
            std::lock_guard<std::mutex> lock(this->assemblyMutex);
            this->assemblyInfoMap[assemblyId] = temp;
        }
    }
    else
    {
        this->eventQueue->push(LogEvent<AssemblyUnloadEvent>(assemblyId, appDomainId, 0, assemblyFlags, fullyQualifiedAssemblyName, this->clrInstanceId));

        {
            std::lock_guard<std::mutex> lock(this->assemblyMutex);

            auto iter = this->assemblyInfoMap.find(assemblyId);
            if (iter != this->assemblyInfoMap.end())
            {
                this->assemblyInfoMap.erase(iter);
                this->assemblyRundown.Remove(iter->second);
            }
        }
    }

    return S_OK;
}

template <typename T>
PIMAGE_DEBUG_DIRECTORY GetDebugDirectories(const IMAGE_OPTIONAL_HEADER *header, LPCBYTE imageBase, ULONG *numberOfDebugDirectories)
{
    PIMAGE_DEBUG_DIRECTORY debugDirectories = nullptr;
    auto optionalHeader = T(header);
    auto offset = optionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;

    if (offset != 0)
    {
        debugDirectories = reinterpret_cast<PIMAGE_DEBUG_DIRECTORY>(reinterpret_cast<ULONG_PTR>(imageBase) + offset);
        *numberOfDebugDirectories = optionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size / sizeof(*debugDirectories);
    }

    return debugDirectories;
}

using ImageHeaderType = std::conditional<sizeof(size_t) == 8, PIMAGE_OPTIONAL_HEADER64, PIMAGE_OPTIONAL_HEADER32>::type;

typedef struct _CV_INFO_PDB70
{
    DWORD CvSignature;
    GUID Signature;
    DWORD Age;
    char PdbFileName[1];
} CV_INFO_PDB70;

HRESULT BPerfProfiler::ModuleLoadUnloadData(ModuleID moduleId, bool load)
{
    HRESULT hr;
    ULONG moduleILPathLength = 0;
    LPCBYTE imageBase;
    DWORD moduleFlags;
    AssemblyID assemblyId;

    IfFailRet(this->corProfilerInfo->GetModuleInfo2(moduleId, &imageBase, 0, &moduleILPathLength, nullptr, &assemblyId, &moduleFlags));
    portable_wide_string moduleILPath(moduleILPathLength, ZEROSTRING);
    IfFailRet(this->corProfilerInfo->GetModuleInfo2(moduleId, nullptr, moduleILPathLength, nullptr, addr(moduleILPath), nullptr, nullptr));

    DWORD nativePdbAge = 0, managedPdbAge = 0;
    GUID nativePdbSignature = IID_NULL, managedPdbSignature = IID_NULL;
    portable_wide_string nativePdbBuildPath, managedPdbBuildPath, moduleNativePath;

    if (imageBase != nullptr)
    {
        auto dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER *>(imageBase);
        auto ntHeader = reinterpret_cast<const IMAGE_NT_HEADERS *>(imageBase + dosHeader->e_lfanew);
        const IMAGE_OPTIONAL_HEADER *header = &ntHeader->OptionalHeader;

        ULONG numberOfDebugDirectories = 0;
        auto debugDirectories = GetDebugDirectories<ImageHeaderType>(header, imageBase, &numberOfDebugDirectories);

        // Last two debug entries are NGEN and IL respectively.
        while (numberOfDebugDirectories > 2)
        {
            ++debugDirectories;
            --numberOfDebugDirectories;
        }

        if (numberOfDebugDirectories == 2)
        {
            if (debugDirectories->Type == IMAGE_DEBUG_TYPE_CODEVIEW)
            {
                auto data = reinterpret_cast<const CV_INFO_PDB70 *>(imageBase + debugDirectories->AddressOfRawData);
                std::string temp(data->PdbFileName);
                nativePdbAge = data->Age;
                nativePdbSignature = data->Signature;
                nativePdbBuildPath = portable_wide_string(temp.begin(), temp.end());
                nativePdbBuildPath.resize(nativePdbBuildPath.length() + 1); // we want the null terminator when we serialize
            }

            ++debugDirectories;
            --numberOfDebugDirectories;
        }

        if (numberOfDebugDirectories == 1)
        {
            if (debugDirectories->Type == IMAGE_DEBUG_TYPE_CODEVIEW)
            {
                auto data = reinterpret_cast<const CV_INFO_PDB70 *>(imageBase + debugDirectories->AddressOfRawData);
                std::string temp(data->PdbFileName);
                managedPdbAge = data->Age;
                managedPdbSignature = data->Signature;
                managedPdbBuildPath = portable_wide_string(temp.begin(), temp.end());
                managedPdbBuildPath.resize(managedPdbBuildPath.length() + 1); // // we want the null terminator when we serialize
            }
        }

        // NGEN Image
        if ((moduleFlags & COR_PRF_MODULE_NGEN) != 0)
        {
            /*
            if (GetMappedFileName(GetCurrentProcess(), LPVOID(imageBase), addr(moduleNativePath), MAX_PATH))
            {
            TranslateDevicePathToDrivePath(moduleNativePath);
            }
            */
        }
    }

    if (load)
    {
        auto temp = this->moduleRundown.Add(LogEvent2<ModuleLoadEvent>(this->eventQueue, moduleId, assemblyId, moduleFlags, moduleILPath, moduleNativePath, this->clrInstanceId, managedPdbSignature, managedPdbAge, managedPdbBuildPath, nativePdbSignature, nativePdbAge, nativePdbBuildPath));

        {
            std::lock_guard<std::mutex> lock(this->moduleMutex);
            this->moduleInfoMap[moduleId] = temp;
        }
    }
    else
    {
        this->eventQueue->push(LogEvent<ModuleUnloadEvent>(moduleId, assemblyId, moduleFlags, moduleILPath, moduleNativePath, this->clrInstanceId, managedPdbSignature, managedPdbAge, managedPdbBuildPath, nativePdbSignature, nativePdbAge, nativePdbBuildPath));

        {
            std::lock_guard<std::mutex> lock(this->moduleMutex);

            auto iter = this->moduleInfoMap.find(moduleId);
            if (iter != this->moduleInfoMap.end())
            {
                this->moduleInfoMap.erase(iter);
                this->moduleRundown.Remove(iter->second);
            }
        }
    }

    return S_OK;
}

std::stack<portable_wide_string> BuildClassStack(IMetaDataImport *metaDataImport, mdTypeDef classTypeDef)
{
    HRESULT hr;
    std::stack<portable_wide_string> classStack;
    DWORD typeDefFlags;
    ULONG classNameLength = 0;

    for (;;)
    {
        hr = metaDataImport->GetTypeDefProps(classTypeDef, nullptr, 0, &classNameLength, &typeDefFlags, nullptr);
        if (FAILED(hr))
        {
            break;
        }

        portable_wide_string className(classNameLength, ZEROSTRING);

        metaDataImport->GetTypeDefProps(classTypeDef, addr(className), classNameLength, &classNameLength, &typeDefFlags, nullptr);
        if (FAILED(hr))
        {
            break;
        }

        classStack.push(std::move(className));

        if (!IsTdNested(typeDefFlags))
        {
            break;
        }

        hr = metaDataImport->GetNestedClassProps(classTypeDef, &classTypeDef);
        if (FAILED(hr))
        {
            break;
        }
    }

    return classStack;
}

LPCWSTR PrettyPrintSigWorker(
    PCCOR_SIGNATURE &typePtr, // type to convert,
    size_t typeLen,           // length of type
    const WCHAR *name,        // can be "", the name of the method for this sig
    CQuickBytes *out,         // where to put the pretty printed string
    IMetaDataImport *pIMDI);  // Import api to use.

HRESULT BPerfProfiler::MethodJitStartFinishData(FunctionID functionId, bool finished, bool unload)
{
    HRESULT hr;
    mdToken methodToken;
    ClassID classId;
    ModuleID moduleId;
    LPCBYTE methodHeader;
    ULONG methodILSize;

    IfFailRet(this->corProfilerInfo->GetFunctionInfo(functionId, &classId, &moduleId, &methodToken));
    IfFailRet(this->corProfilerInfo->GetILFunctionBody(moduleId, methodToken, &methodHeader, &methodILSize));

    CComPtr<IMetaDataImport> metaDataImport;
    IfFailRet(this->corProfilerInfo->GetTokenAndMetaDataFromFunction(functionId, IID_IMetaDataImport, reinterpret_cast<IUnknown **>(&metaDataImport), &methodToken));

    mdTypeDef classTypeDef;

    ULONG methodNameLength = 0;
    ULONG sigLength = 0;
    PCCOR_SIGNATURE sig;

    IfFailRet(metaDataImport->GetMethodProps(methodToken, &classTypeDef, nullptr, 0, &methodNameLength, nullptr, &sig, &sigLength, nullptr, nullptr));
    portable_wide_string methodName(methodNameLength, ZEROSTRING);
    IfFailRet(metaDataImport->GetMethodProps(methodToken, nullptr, addr(methodName), methodNameLength, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));

    IfFailRet(metaDataImport->GetTypeDefProps(classTypeDef, nullptr, 0, nullptr, nullptr, nullptr));

    std::stack<portable_wide_string> classStack(BuildClassStack(metaDataImport, classTypeDef));

    portable_wide_string fullClassName;

    while (!classStack.empty())
    {
        fullClassName += classStack.top();
        classStack.pop();
        if (!classStack.empty())
        {
            fullClassName += W("+");
        }
    }

    CQuickBytes quickBytes;
    portable_wide_string signatureString(PrettyPrintSigWorker(sig, sigLength, EMPTYSTRING, &quickBytes, metaDataImport));
    signatureString.resize(signatureString.length() + 1); // we want the null terminator when we serialize

    if (!finished)
    {
        this->eventQueue->push(LogEvent<JITCompilationStartedEvent>(functionId, moduleId, methodToken, methodILSize, fullClassName, methodName, signatureString, this->clrInstanceId));
    }
    else
    {
        ULONG32 codeInfosCount;

        IfFailRet(this->corProfilerInfo->GetCodeInfo2(functionId, 0, &codeInfosCount, nullptr));
        std::vector<COR_PRF_CODE_INFO> codeInfos(codeInfosCount);
        IfFailRet(this->corProfilerInfo->GetCodeInfo2(functionId, codeInfosCount, &codeInfosCount, codeInfos.data()));

        int64_t methodStartAddress = 0;
        int32_t methodSize = 0;

        if (codeInfosCount > 0)
        {
            methodStartAddress = codeInfos[0].startAddress;
            methodSize = static_cast<int32_t>(codeInfos[0].size);
        }

        if (unload)
        {
            this->eventQueue->push(LogEvent<MethodUnloadEvent>(functionId, moduleId, methodStartAddress, methodSize, methodToken, 1, fullClassName, methodName, signatureString, this->clrInstanceId, 0));

            {
                std::lock_guard<std::mutex> lock(this->methodMutex);

                auto iter = this->methodInfoMap.find(functionId);
                if (iter != this->methodInfoMap.end())
                {
                    this->methodInfoMap.erase(iter);
                    this->methodRundown.Remove(iter->second);
                }
            }

            {
                std::lock_guard<std::mutex> lock(this->ilNativeMutex);

                auto iter2 = this->ilNativeInfoMap.find(functionId);
                if (iter2 != this->ilNativeInfoMap.end())
                {
                    this->ilNativeInfoMap.erase(iter2);
                    this->ilNativeMapRundown.Remove(iter2->second);
                }
            }
        }
        else
        {
            auto temp = this->methodRundown.Add(LogEvent2<JITCompilationFinishedEvent>(this->eventQueue, functionId, moduleId, methodStartAddress, methodSize, methodToken, 1, fullClassName, methodName, signatureString, this->clrInstanceId, 0));

            {
                std::lock_guard<std::mutex> lock(this->methodMutex);
                this->methodInfoMap[functionId] = temp;
            }
        }
    }

    return S_OK;
}

HRESULT BPerfProfiler::DynamicMethodJitStartFinishData(FunctionID functionId, ULONG cbILHeader, bool finished)
{
    HRESULT hr;
    ModuleID moduleId;
    ULONG methodNameLength = 0;
    ULONG sigLength = 0;
    PCCOR_SIGNATURE sig;

    IfFailRet(this->corProfilerInfo->GetDynamicFunctionInfo(functionId, &moduleId, &sig, &sigLength, 0, &methodNameLength, nullptr));
    portable_wide_string methodName(methodNameLength, ZEROSTRING);
    IfFailRet(this->corProfilerInfo->GetDynamicFunctionInfo(functionId, nullptr, &sig, &sigLength, methodNameLength, nullptr, addr(methodName)));

    CComPtr<IMetaDataImport> metaDataImport;
    IfFailRet(this->corProfilerInfo->GetModuleMetaData(moduleId, ofRead, IID_IMetaDataImport, reinterpret_cast<IUnknown **>(&metaDataImport)));

    CQuickBytes quickBytes;
    portable_wide_string signatureString(PrettyPrintSigWorker(sig, sigLength, EMPTYSTRING, &quickBytes, metaDataImport));
    signatureString.resize(signatureString.length() + 1); // we want the null terminator when we serialize

    if (!finished)
    {
        this->eventQueue->push(LogEvent<JITCompilationStartedEvent>(functionId, moduleId, 0, cbILHeader, W("dynamicClass"), methodName, signatureString, this->clrInstanceId));
    }
    else
    {
        ULONG32 codeInfosCount;

        IfFailRet(this->corProfilerInfo->GetCodeInfo2(functionId, 0, &codeInfosCount, nullptr));
        std::vector<COR_PRF_CODE_INFO> codeInfos(codeInfosCount);
        IfFailRet(this->corProfilerInfo->GetCodeInfo2(functionId, codeInfosCount, &codeInfosCount, codeInfos.data()));

        int64_t methodStartAddress = 0;
        int32_t methodSize = 0;

        if (codeInfosCount > 0)
        {
            methodStartAddress = codeInfos[0].startAddress;
            methodSize = static_cast<int32_t>(codeInfos[0].size);
        }

        auto temp = this->methodRundown.Add(LogEvent2<JITCompilationFinishedEvent>(this->eventQueue, functionId, moduleId, methodStartAddress, methodSize, 0, 1, W("dynamicClass"), methodName, signatureString, this->clrInstanceId, 0));
        {
            std::lock_guard<std::mutex> lock(this->methodMutex);
            this->methodInfoMap[functionId] = temp;
        }
    }

    return S_OK;
}