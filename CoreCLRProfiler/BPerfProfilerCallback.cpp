#include "BPerfProfilerCallback.h"
#include <vector>
#include "openum.h"

static void CopyData(std::vector<BYTE> &vec, PBYTE data, int size)
{
    for (int i = 0; i < size; ++i)
    {
        vec.push_back(*data);
        data++;
    }
}

static void STDMETHODCALLTYPE WriteEventCore(GUID* providerId, int eventId, int eventDataCount, EVENT_DATA_DESCRIPTOR* data)
{
    printf("WriteEventCore : EventId: %d, EventDataCount: %d\n", eventId, eventDataCount);
}

static void STDMETHODCALLTYPE WriteEventWithRelatedActivityIdCore(GUID* providerId, int eventId, GUID* childActivityID, int eventDataCount, EVENT_DATA_DESCRIPTOR* data)
{
    printf("WriteEventWithRelatedActivityIdCore : EventId: %d, EventDataCount: %d\n", eventId, eventDataCount);
}

static HRESULT HijackWriteEventWithRelatedActivityIdCore(ICorProfilerInfo10* corProfilerInfo, IMetaDataEmit* metadataEmit, ModuleID moduleId, mdToken methodDef, mdFieldDef mGuidFieldDef)
{
    mdSignature metadataSignature;
    constexpr COR_SIGNATURE signature[] = { IMAGE_CEE_CS_CALLCONV_STDCALL, 0x05, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I, ELEMENT_TYPE_I4, ELEMENT_TYPE_I, ELEMENT_TYPE_I4, ELEMENT_TYPE_I };
    IfFailRet(metadataEmit->GetTokenFromSig(&signature[0], sizeof(signature), &metadataSignature));

    constexpr COR_SIGNATURE localVargSig[] = { IMAGE_CEE_CS_CALLCONV_LOCAL_SIG, 0x01, ELEMENT_TYPE_PINNED, ELEMENT_TYPE_BYREF, ELEMENT_TYPE_U1 };
    mdSignature localVarSigTok;
    IfFailRet(metadataEmit->GetTokenFromSig(&localVargSig[0], sizeof(localVargSig), &localVarSigTok));

    std::vector<BYTE> data;

    IMAGE_COR_ILMETHOD_FAT header;
    header.Flags = CorILMethod_FatFormat;
    header.Size = 3; // constant
    header.MaxStack = 7;
    header.CodeSize = 25 + sizeof(size_t);
    header.LocalVarSigTok = localVarSigTok;

    CopyData(data, reinterpret_cast<PBYTE>(&header), sizeof(IMAGE_COR_ILMETHOD_FAT));
    data.push_back(CEE_LDARG_0);
    data.push_back(CEE_LDFLDA);
    CopyData(data, reinterpret_cast<PBYTE>(&mGuidFieldDef), sizeof(mdFieldDef));
    data.push_back(CEE_STLOC_0);
    data.push_back(CEE_LDLOC_0);
    data.push_back(CEE_CONV_U);
    data.push_back(CEE_LDARG_1);
    data.push_back(CEE_LDARG_2);
    data.push_back(CEE_LDARG_3);
    data.push_back(CEE_LDARG_S);
    data.push_back(0x4);
    sizeof(size_t) == 8 ? data.push_back(CEE_LDC_I8) : sizeof(size_t) == 4 ? data.push_back(CEE_LDC_I4) : throw std::logic_error("size_t must be defined as 8 or 4");
    auto addr = reinterpret_cast<size_t>(&WriteEventWithRelatedActivityIdCore);
    CopyData(data, reinterpret_cast<PBYTE>(&addr), sizeof(size_t));
    data.push_back(CEE_CONV_I);
    data.push_back(CEE_CALLI);
    CopyData(data, reinterpret_cast<PBYTE>(&metadataSignature), sizeof(mdSignature));
    data.push_back(CEE_LDC_I4_0);
    data.push_back(CEE_CONV_U);
    data.push_back(CEE_STLOC_0);
    data.push_back(CEE_RET);

    CComPtr<IMethodMalloc> methodMalloc;
    IfFailRet(corProfilerInfo->GetILFunctionBodyAllocator(moduleId, reinterpret_cast<IMethodMalloc * *>(&methodMalloc)));
    const auto ilBytes = static_cast<PBYTE>(methodMalloc->Alloc(static_cast<ULONG>(data.size())));
    CopyMemory(&ilBytes[0], data.data(), data.size());

    IfFailRet(corProfilerInfo->SetILFunctionBody(moduleId, methodDef, &ilBytes[0]));

    return S_OK;
}

static HRESULT HijackWriteEventCore(ICorProfilerInfo10* corProfilerInfo, IMetaDataEmit* metadataEmit, ModuleID moduleId, mdToken methodDef, mdFieldDef mGuidFieldDef)
{
    mdSignature metadataSignature;
    constexpr COR_SIGNATURE signature[] = { IMAGE_CEE_CS_CALLCONV_STDCALL, 0x04, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I, ELEMENT_TYPE_I4, ELEMENT_TYPE_I4, ELEMENT_TYPE_I };
    IfFailRet(metadataEmit->GetTokenFromSig(&signature[0], sizeof(signature), &metadataSignature));

    constexpr COR_SIGNATURE localVargSig[] = { IMAGE_CEE_CS_CALLCONV_LOCAL_SIG, 0x01, ELEMENT_TYPE_PINNED, ELEMENT_TYPE_BYREF, ELEMENT_TYPE_U1 };
    mdSignature localVarSigTok;
    IfFailRet(metadataEmit->GetTokenFromSig(&localVargSig[0], sizeof(localVargSig), &localVarSigTok));

    std::vector<BYTE> data;

    IMAGE_COR_ILMETHOD_FAT header;
    header.Flags = CorILMethod_FatFormat;
    header.Size = 3; // constant
    header.MaxStack = 6;
    header.CodeSize = 23 + sizeof(size_t);
    header.LocalVarSigTok = localVarSigTok;

    CopyData(data, reinterpret_cast<PBYTE>(&header), sizeof(IMAGE_COR_ILMETHOD_FAT));
    data.push_back(CEE_LDARG_0);
    data.push_back(CEE_LDFLDA);
    CopyData(data, reinterpret_cast<PBYTE>(&mGuidFieldDef), sizeof(mdFieldDef));
    data.push_back(CEE_STLOC_0);
    data.push_back(CEE_LDLOC_0);
    data.push_back(CEE_CONV_U);
    data.push_back(CEE_LDARG_1);
    data.push_back(CEE_LDARG_2);
    data.push_back(CEE_LDARG_3);
    sizeof(size_t) == 8 ? data.push_back(CEE_LDC_I8) : sizeof(size_t) == 4 ? data.push_back(CEE_LDC_I4) : throw std::logic_error("size_t must be defined as 8 or 4");
    auto addr = reinterpret_cast<size_t>(&WriteEventCore);
    CopyData(data, reinterpret_cast<PBYTE>(&addr), sizeof(size_t));
    data.push_back(CEE_CONV_I);
    data.push_back(CEE_CALLI);
    CopyData(data, reinterpret_cast<PBYTE>(&metadataSignature), sizeof(mdSignature));
    data.push_back(CEE_LDC_I4_0);
    data.push_back(CEE_CONV_U);
    data.push_back(CEE_STLOC_0);
    data.push_back(CEE_RET);

    CComPtr<IMethodMalloc> methodMalloc;
    IfFailRet(corProfilerInfo->GetILFunctionBodyAllocator(moduleId, reinterpret_cast<IMethodMalloc **>(&methodMalloc)));
    const auto ilBytes = static_cast<PBYTE>(methodMalloc->Alloc(static_cast<ULONG>(data.size())));
    CopyMemory(&ilBytes[0], data.data(), data.size());

    IfFailRet(corProfilerInfo->SetILFunctionBody(moduleId, methodDef, &ilBytes[0]));

    return S_OK;
}

static void InitClrEventRecord(ICorProfilerInfo10* corProfilerInfo, PEVENT_RECORD eventRecord, USHORT eventId, UCHAR version)
{
    const auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    ThreadID threadId;
    corProfilerInfo->GetCurrentThreadID(&threadId);
    DWORD win32ThreadId;
    corProfilerInfo->GetThreadInfo(threadId, &win32ThreadId);

    eventRecord->EventHeader.EventProperty = EVENT_HEADER_PROPERTY_XML;
    eventRecord->EventHeader.Flags = sizeof(size_t) == 8 ? EVENT_HEADER_FLAG_64_BIT_HEADER : sizeof(size_t) == 4 ? EVENT_HEADER_FLAG_32_BIT_HEADER : throw std::logic_error("size_t must be defined as 8 or 4");
    eventRecord->EventHeader.ProcessId = 0;
    eventRecord->EventHeader.ThreadId = win32ThreadId;
    eventRecord->EventHeader.TimeStamp.QuadPart = ts;
    eventRecord->EventHeader.ProviderId = { 0xE13C0D23, 0xCCBC, 0x4E12, {0x93, 0x1B, 0xD9, 0xCC, 0x2E, 0xEE, 0x27, 0xE4} };

    eventRecord->EventHeader.EventDescriptor.Id = eventId;
    eventRecord->EventHeader.EventDescriptor.Version = version;
    eventRecord->EventHeader.EventDescriptor.Opcode = 0;
}

static HRESULT IdentifyEventSourceType(ICorProfilerInfo10* corProfilerInfo, ClassID classId, ModuleID *eventSourceTypeModuleId, mdTypeDef *writeEventCoreMethodDef, mdTypeDef *writeEventWithRelatedActivityIdCoreMethodDef, mdFieldDef *mGuidFieldDef)
{
    ModuleID moduleId;
    mdTypeDef typeDef;
    IfFailRet(corProfilerInfo->GetClassIDInfo(classId, &moduleId, &typeDef));

    CComPtr<IMetaDataImport> metadataImport;
    IfFailRet(corProfilerInfo->GetModuleMetaData(moduleId, ofRead, IID_IMetaDataImport, reinterpret_cast<IUnknown * *>(&metadataImport)));

    ULONG cchTypeDef;
    IfFailRet(metadataImport->GetTypeDefProps(typeDef, nullptr, 0, &cchTypeDef, nullptr, nullptr));
    std::vector<WCHAR> typeDefName(cchTypeDef);
    IfFailRet(metadataImport->GetTypeDefProps(typeDef, typeDefName.data(), cchTypeDef, &cchTypeDef, nullptr, nullptr));

    const WCHAR eventSourceClassName[] = W("System.Diagnostics.Tracing.EventSource");
    const size_t size = typeDefName.size() * sizeof(WCHAR);

    if (size == sizeof(eventSourceClassName) && memcmp(typeDefName.data(), &eventSourceClassName[0], MIN(size, sizeof(eventSourceClassName))) == 0)
    {
        *eventSourceTypeModuleId = moduleId;
        HCORENUM hcorenum = nullptr;
        ULONG cTokens;
        IfFailRet(metadataImport->EnumMethodsWithName(&hcorenum, typeDef, W("WriteEventCore"), writeEventCoreMethodDef, 1, &cTokens));
        hcorenum = nullptr;
        IfFailRet(metadataImport->EnumMethodsWithName(&hcorenum, typeDef, W("WriteEventWithRelatedActivityIdCore"), writeEventWithRelatedActivityIdCoreMethodDef, 1, &cTokens));
        hcorenum = nullptr;
        IfFailRet(metadataImport->EnumFieldsWithName(&hcorenum, typeDef, W("m_guid"), mGuidFieldDef, 1, &cTokens));
    }

    return S_OK;
}

BPerfProfilerCallback::BPerfProfilerCallback()
{
    this->corProfilerInfo = nullptr;
    this->refCount = 0;
    this->numberOfExceptionsThrown = 0;
    this->numberOfFiltersExecuted = 0;
    this->numberOfFinallysExecuted = 0;
    this->totalILBytesJitted = 0;
    this->totalNumberOfMethodsJitted = 0;
    this->currentNumberOfMethodsJitted = 0;
    this->totalMachineCodeBytesGenerated = 0;
    this->totalILBytesJittedForDynamicMethods = 0;
    this->totalNumberOfDynamicMethods = 0;
    this->currentNumberOfDynamicMethods = 0;
    this->totalCachedMethodsSearched = 0;
    this->totalCachedMethodsRestored = 0;
    this->totalNumberOfRuntimeSuspensions = 0;
    this->totalNumberOfRuntimeSuspensionsForGC = 0;
    this->totalNumberOfRuntimeSuspensionsForGCPrep = 0;
    this->currentNumberOfClassesLoaded = 0;
    this->totalNumberOfClassesLoaded = 0;
    this->totalNumberOfClassLoadFailures = 0;
    this->currentNumberOfAssembliesLoaded = 0;
    this->totalNumberOfAssembliesLoaded = 0;
    this->currentNumberOfModulesLoaded = 0;
    this->totalNumberOfModulesLoaded = 0;
    this->currentNumberOfLogicalThreads = 0;
    this->totalNumberOfInducedGarbageCollections = 0;
    this->totalNumberOfGen0Collections = 0;
    this->totalNumberOfGen1Collections = 0;
    this->totalNumberOfGen2Collections = 0;
    this->totalNumberOfGen3Collections = 0;
    this->totalNumberOfAllBytesAllocatedSinceLastGC = 0;
    this->totalNumberOfBytesInAllHeaps = 0;
    this->gen0HeapSize = 0;
    this->gen1HeapSize = 0;
    this->gen2HeapSize = 0;
    this->gen3HeapSize = 0;
    this->numberOfGCSegments = 0;
    this->numberOfFrozenSegments = 0;
    this->totalHeapSizeAtGCStart = 0;
    this->totalPromotedBytes = 0;

    this->moduleTableFileOffset = 0;
    this->classTableFileOffset = 0;
    this->methodTableFileOffset = 0;
    this->assemblyTableFileOffset = 0;
    this->dataFileOffset = 0;

    this->eventSourceTypeModuleId = 0;
    this->mGuidFieldDef = 0;
    this->writeEventCoreMethodDef = 0;
    this->writeEventWithRelatedActivityIdCoreMethodDef = 0;
}

BPerfProfilerCallback::~BPerfProfilerCallback()
{
    if (this->corProfilerInfo != nullptr)
    {
        this->corProfilerInfo->Release();
        this->corProfilerInfo = nullptr;
    }
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::Initialize(IUnknown* pICorProfilerInfoUnk)
{
    const HRESULT queryInterfaceResult = pICorProfilerInfoUnk->QueryInterface(__uuidof(ICorProfilerInfo10), reinterpret_cast<void**>(&this->corProfilerInfo));

    if (FAILED(queryInterfaceResult))
    {
        return E_FAIL;
    }

    const DWORD eventsMask = COR_PRF_MONITOR_THREADS          |
                             COR_PRF_MONITOR_SUSPENDS         |
                             COR_PRF_MONITOR_MODULE_LOADS     |
                             COR_PRF_MONITOR_CACHE_SEARCHES   |
                             COR_PRF_MONITOR_JIT_COMPILATION  |
                             COR_PRF_MONITOR_FUNCTION_UNLOADS |
                             COR_PRF_MONITOR_EXCEPTIONS       |
                             COR_PRF_MONITOR_CLR_EXCEPTIONS   |
                             COR_PRF_MONITOR_CLASS_LOADS      |
                             COR_PRF_ENABLE_STACK_SNAPSHOT    |
                             COR_PRF_MONITOR_ASSEMBLY_LOADS   ;

    const DWORD eventsHigh = COR_PRF_HIGH_MONITOR_DYNAMIC_FUNCTION_UNLOADS |
                             COR_PRF_HIGH_BASIC_GC                         |
                             COR_PRF_HIGH_MONITOR_LARGEOBJECT_ALLOCATED    ;

    return this->corProfilerInfo->SetEventMask2(eventsMask, eventsHigh);
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::Shutdown()
{
    if (this->corProfilerInfo != nullptr)
    {
        this->corProfilerInfo->Release();
        this->corProfilerInfo = nullptr;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::AppDomainCreationStarted(AppDomainID appDomainId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::AppDomainCreationFinished(AppDomainID appDomainId, HRESULT hrStatus)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::AppDomainShutdownStarted(AppDomainID appDomainId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::AppDomainShutdownFinished(AppDomainID appDomainId, HRESULT hrStatus)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::AssemblyLoadStarted(AssemblyID assemblyId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::AssemblyLoadFinished(AssemblyID assemblyId, HRESULT hrStatus)
{
    ++this->totalNumberOfAssembliesLoaded;
    ++this->currentNumberOfAssembliesLoaded;

    // Log with callstack, timestamp

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::AssemblyUnloadStarted(AssemblyID assemblyId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::AssemblyUnloadFinished(AssemblyID assemblyId, HRESULT hrStatus)
{
    --this->currentNumberOfAssembliesLoaded;

    // Log with callstack, timestamp

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ModuleLoadStarted(ModuleID moduleId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ModuleLoadFinished(ModuleID moduleId, HRESULT hrStatus)
{
    ++this->totalNumberOfModulesLoaded;
    ++this->currentNumberOfModulesLoaded;

    // Log with callstack, timestamp

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ModuleUnloadStarted(ModuleID moduleId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ModuleUnloadFinished(ModuleID moduleId, HRESULT hrStatus)
{
    --this->currentNumberOfModulesLoaded;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ModuleAttachedToAssembly(ModuleID moduleId, AssemblyID assemblyId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ClassLoadStarted(ClassID classId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ClassLoadFinished(ClassID classId, HRESULT hrStatus)
{
    EVENT_RECORD eventRecord;
    InitClrEventRecord(this->corProfilerInfo, &eventRecord, 1, 1);

    if (hrStatus == S_OK)
    {
        ++this->totalNumberOfClassesLoaded;
        ++this->currentNumberOfClassesLoaded;
    }
    else
    {
        ++this->totalNumberOfClassLoadFailures;
    }

    // TODO: Add Class Load Debug Logging
    return IdentifyEventSourceType(this->corProfilerInfo, classId, &this->eventSourceTypeModuleId, &this->writeEventCoreMethodDef, &this->writeEventWithRelatedActivityIdCoreMethodDef, &this->mGuidFieldDef);
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ClassUnloadStarted(ClassID classId)
{
    --this->currentNumberOfClassesLoaded;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ClassUnloadFinished(ClassID classId, HRESULT hrStatus)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::FunctionUnloadStarted(FunctionID functionId)
{
    --this->currentNumberOfMethodsJitted;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::JITCompilationStarted(FunctionID functionId, BOOL fIsSafeToBlock)
{
    ModuleID moduleId;
    mdToken methodDef;
    IfFailRet(corProfilerInfo->GetFunctionInfo(functionId, nullptr, &moduleId, &methodDef));

    if (moduleId == this->eventSourceTypeModuleId && (methodDef == this->writeEventCoreMethodDef || methodDef == this->writeEventWithRelatedActivityIdCoreMethodDef))
    {
        CComPtr<IMetaDataImport> metadataImport;
        IfFailRet(corProfilerInfo->GetModuleMetaData(moduleId, ofRead, IID_IMetaDataImport, reinterpret_cast<IUnknown **>(&metadataImport)));

        CComPtr<IMetaDataEmit> metadataEmit;
        IfFailRet(metadataImport->QueryInterface(IID_IMetaDataEmit, reinterpret_cast<void**>(&metadataEmit)));

        if (methodDef == this->writeEventCoreMethodDef)
        {
            HijackWriteEventCore(corProfilerInfo, metadataEmit, moduleId, methodDef, this->mGuidFieldDef);
        }
        else if (methodDef == this->writeEventWithRelatedActivityIdCoreMethodDef)
        {
            HijackWriteEventWithRelatedActivityIdCore(corProfilerInfo, metadataEmit, moduleId, methodDef, this->mGuidFieldDef);
        }
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::JITCompilationFinished(FunctionID functionId, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    ++this->totalNumberOfMethodsJitted;
    ++this->currentNumberOfMethodsJitted;

    ModuleID moduleId;
    mdToken token;
    IfFailRet(this->corProfilerInfo->GetFunctionInfo(functionId, nullptr, &moduleId, &token));

    ULONG cbMethodSize;
    IfFailRet(this->corProfilerInfo->GetILFunctionBody(moduleId, token, nullptr, &cbMethodSize));
    this->totalILBytesJitted += cbMethodSize;

    ULONG32 cCodeInfos;
    IfFailRet(this->corProfilerInfo->GetCodeInfo2(functionId, 0, &cCodeInfos, nullptr));
    std::vector<COR_PRF_CODE_INFO> codeInfos(cCodeInfos);
    IfFailRet(this->corProfilerInfo->GetCodeInfo2(functionId, 0, &cCodeInfos, codeInfos.data()));

    SIZE_T codeSize = 0;
    for (auto &s : codeInfos)
    {
        codeSize += s.size;
    }

    this->totalMachineCodeBytesGenerated += codeSize;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::JITCachedFunctionSearchStarted(FunctionID functionId, BOOL* pbUseCachedFunction)
{
    ++this->totalCachedMethodsSearched;

    ModuleID moduleId;
    mdToken methodToken;
    IfFailRet(this->corProfilerInfo->GetFunctionInfo(functionId, nullptr, &moduleId, &methodToken));

    if (methodToken == this->writeEventCoreMethodDef || methodToken == this->writeEventWithRelatedActivityIdCoreMethodDef)
    {
        *pbUseCachedFunction = FALSE;
    }
    else
    {
        *pbUseCachedFunction = TRUE;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::JITCachedFunctionSearchFinished(FunctionID functionId, COR_PRF_JIT_CACHE result)
{
    if (result == COR_PRF_CACHED_FUNCTION_FOUND)
    {
        ++this->totalCachedMethodsRestored;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::JITFunctionPitched(FunctionID functionId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::JITInlining(FunctionID callerId, FunctionID calleeId, BOOL* pfShouldInline)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ThreadCreated(ThreadID threadId)
{
    ++this->currentNumberOfLogicalThreads;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ThreadDestroyed(ThreadID threadId)
{
    --this->currentNumberOfLogicalThreads;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ThreadAssignedToOSThread(ThreadID managedThreadId, DWORD osThreadId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RemotingClientInvocationStarted()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RemotingClientSendingMessage(GUID* pCookie, BOOL fIsAsync)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RemotingClientReceivingReply(GUID* pCookie, BOOL fIsAsync)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RemotingClientInvocationFinished()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RemotingServerReceivingMessage(GUID* pCookie, BOOL fIsAsync)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RemotingServerInvocationStarted()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RemotingServerInvocationReturned()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RemotingServerSendingReply(GUID* pCookie, BOOL fIsAsync)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::UnmanagedToManagedTransition(FunctionID functionId, COR_PRF_TRANSITION_REASON reason)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ManagedToUnmanagedTransition(FunctionID functionId, COR_PRF_TRANSITION_REASON reason)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RuntimeSuspendStarted(COR_PRF_SUSPEND_REASON suspendReason)
{
    switch(suspendReason)
    {
    case COR_PRF_SUSPEND_FOR_GC:
        ++this->totalNumberOfRuntimeSuspensionsForGC;
        break;
    case COR_PRF_SUSPEND_FOR_GC_PREP:
        ++this->totalNumberOfRuntimeSuspensionsForGCPrep;
        break;
    default:
        break;
    }

    ++this->totalNumberOfRuntimeSuspensions;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RuntimeSuspendFinished()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RuntimeSuspendAborted()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RuntimeResumeStarted()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RuntimeResumeFinished()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RuntimeThreadSuspended(ThreadID threadId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RuntimeThreadResumed(ThreadID threadId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::MovedReferences(ULONG cMovedObjectIDRanges, ObjectID oldObjectIDRangeStart[], ObjectID newObjectIDRangeStart[], ULONG cObjectIDRangeLength[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ObjectAllocated(ObjectID objectId, ClassID classId)
{
    // this->corProfilerInfo->DoStackSnapshot(0, nullptr, 0, nullptr, nullptr, 0);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ObjectsAllocatedByClass(ULONG cClassCount, ClassID classIds[], ULONG cObjects[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ObjectReferences(ObjectID objectId, ClassID classId, ULONG cObjectRefs, ObjectID objectRefIds[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RootReferences(ULONG cRootRefs, ObjectID rootRefIds[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionThrown(ObjectID thrownObjectId)
{
    ++this->numberOfExceptionsThrown;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionSearchFunctionEnter(FunctionID functionId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionSearchFunctionLeave()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionSearchFilterEnter(FunctionID functionId)
{
    ++this->numberOfFiltersExecuted;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionSearchFilterLeave()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionSearchCatcherFound(FunctionID functionId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionOSHandlerEnter(UINT_PTR __unused)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionOSHandlerLeave(UINT_PTR __unused)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionUnwindFunctionEnter(FunctionID functionId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionUnwindFunctionLeave()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionUnwindFinallyEnter(FunctionID functionId)
{
    ++this->numberOfFinallysExecuted;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionUnwindFinallyLeave()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionCatcherEnter(FunctionID functionId, ObjectID objectId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionCatcherLeave()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::COMClassicVTableCreated(ClassID wrappedClassId, REFGUID implementedIID, void* pVTable, ULONG cSlots)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::COMClassicVTableDestroyed(ClassID wrappedClassId, REFGUID implementedIID, void* pVTable)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionCLRCatcherFound()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ExceptionCLRCatcherExecute()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ThreadNameChanged(ThreadID threadId, ULONG cchName, WCHAR name[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::GarbageCollectionStarted(int cGenerations, BOOL generationCollected[], COR_PRF_GC_REASON reason)
{
    if (reason == COR_PRF_GC_INDUCED)
    {
        ++this->totalNumberOfInducedGarbageCollections;
    }

    for (int i = 0; i < cGenerations; ++i)
    {
        if (generationCollected[i])
        {
            switch (i)
            {
            case COR_PRF_GC_GEN_0:
                ++this->totalNumberOfGen0Collections;
                break;
            case COR_PRF_GC_GEN_1:
                ++this->totalNumberOfGen1Collections;
                break;
            case COR_PRF_GC_GEN_2:
                ++this->totalNumberOfGen2Collections;
                break;
            case COR_PRF_GC_LARGE_OBJECT_HEAP:
                ++this->totalNumberOfGen3Collections;
                break;
            default:
                break;
            }
        }
    }

    ULONG cObjectRanges = 0;
    IfFailRet(this->corProfilerInfo->GetGenerationBounds(0, &cObjectRanges, nullptr));
    std::vector<COR_PRF_GC_GENERATION_RANGE> objectRanges(cObjectRanges);
    IfFailRet(this->corProfilerInfo->GetGenerationBounds(cObjectRanges, &cObjectRanges, objectRanges.data()));

    SIZE_T sum = 0;
    for (auto& s : objectRanges)
    {
        sum += s.rangeLength;
    }

    this->totalNumberOfAllBytesAllocatedSinceLastGC = sum - this->totalNumberOfBytesInAllHeaps;
    this->totalHeapSizeAtGCStart = sum;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::SurvivingReferences(ULONG cSurvivingObjectIDRanges, ObjectID objectIDRangeStart[], ULONG cObjectIDRangeLength[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::GarbageCollectionFinished()
{
    SIZE_T generationSizes[4] = { 0, 0, 0, 0 };

    ULONG cObjectRanges = 0;
    IfFailRet(this->corProfilerInfo->GetGenerationBounds(0, &cObjectRanges, nullptr));
    std::vector<COR_PRF_GC_GENERATION_RANGE> objectRanges(cObjectRanges);
    IfFailRet(this->corProfilerInfo->GetGenerationBounds(cObjectRanges, &cObjectRanges, objectRanges.data()));

    SIZE_T frozenSegmentCount = 0;
    for (auto &s : objectRanges)
    {
        generationSizes[s.generation] += s.rangeLength;

        BOOL bFrozen = FALSE;
        this->corProfilerInfo->IsFrozenObject(s.rangeStart, &bFrozen);

        if (bFrozen)
        {
            ++frozenSegmentCount;
        }
    }

    this->numberOfGCSegments = cObjectRanges;
    this->numberOfFrozenSegments = frozenSegmentCount;

    this->gen0HeapSize = generationSizes[COR_PRF_GC_GEN_0];
    this->gen1HeapSize = generationSizes[COR_PRF_GC_GEN_1];
    this->gen2HeapSize = generationSizes[COR_PRF_GC_GEN_2];
    this->gen3HeapSize = generationSizes[COR_PRF_GC_LARGE_OBJECT_HEAP];

    const auto sum = generationSizes[COR_PRF_GC_GEN_0] + generationSizes[COR_PRF_GC_GEN_1] + generationSizes[COR_PRF_GC_GEN_2] + generationSizes[COR_PRF_GC_LARGE_OBJECT_HEAP];

    this->totalNumberOfBytesInAllHeaps = sum;
    this->totalPromotedBytes = sum - this->totalHeapSizeAtGCStart;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::FinalizeableObjectQueued(DWORD finalizerFlags, ObjectID objectID)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RootReferences2(ULONG cRootRefs, ObjectID rootRefIds[], COR_PRF_GC_ROOT_KIND rootKinds[], COR_PRF_GC_ROOT_FLAGS rootFlags[], UINT_PTR rootIds[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::HandleCreated(GCHandleID handleId, ObjectID initialObjectId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::HandleDestroyed(GCHandleID handleId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::InitializeForAttach(IUnknown* pCorProfilerInfoUnk, void* pvClientData, UINT cbClientData)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ProfilerAttachComplete()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ProfilerDetachSucceeded()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ReJITCompilationStarted(FunctionID functionId, ReJITID rejitId, BOOL fIsSafeToBlock)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl* pFunctionControl)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ReJITCompilationFinished(FunctionID functionId, ReJITID rejitId, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ReJITError(ModuleID moduleId, mdMethodDef methodId, FunctionID functionId, HRESULT hrStatus)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::MovedReferences2(ULONG cMovedObjectIDRanges, ObjectID oldObjectIDRangeStart[], ObjectID newObjectIDRangeStart[], SIZE_T cObjectIDRangeLength[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::SurvivingReferences2(ULONG cSurvivingObjectIDRanges, ObjectID objectIDRangeStart[], SIZE_T cObjectIDRangeLength[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ConditionalWeakTableElementReferences(ULONG cRootRefs, ObjectID keyRefIds[], ObjectID valueRefIds[], GCHandleID rootIds[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::GetAssemblyReferences(const WCHAR* wszAssemblyPath, ICorProfilerAssemblyReferenceProvider* pAsmRefProvider)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ModuleInMemorySymbolsUpdated(ModuleID moduleId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::DynamicMethodJITCompilationStarted(FunctionID functionId, BOOL fIsSafeToBlock, LPCBYTE ilHeader, ULONG cbILHeader)
{
    ++this->totalNumberOfDynamicMethods;
    ++this->currentNumberOfDynamicMethods;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::DynamicMethodJITCompilationFinished(FunctionID functionId, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::DynamicMethodUnloaded(FunctionID functionId)
{
    --this->currentNumberOfDynamicMethods;

    return S_OK;
}

/* Exception Counters */
size_t BPerfProfilerCallback::GetNumberOfExceptionsThrown() const
{
    return this->numberOfExceptionsThrown;
}

size_t BPerfProfilerCallback::GetNumberOfFiltersExecuted() const
{
    return this->numberOfFiltersExecuted;
}

size_t BPerfProfilerCallback::GetNumberOfFinallysExecuted() const
{
    return this->numberOfFinallysExecuted;
}

/* Metadata Method Counters */
size_t BPerfProfilerCallback::GetTotalILBytesJittedForMetadataMethods() const
{
    return this->totalILBytesJitted;
}

size_t BPerfProfilerCallback::GetTotalNumberOfMetadataMethodsJitted() const
{
    return this->totalNumberOfMethodsJitted;
}

size_t BPerfProfilerCallback::GetCurrentNumberOfMetadataMethodsJitted() const
{
    return this->currentNumberOfMethodsJitted;
}

size_t BPerfProfilerCallback::GetTotalMachineCodeBytesGeneratedForMetadataMethods() const
{
    return this->totalMachineCodeBytesGenerated;
}

/* Dynamic Method Counters */
size_t BPerfProfilerCallback::GetTotalILBytesJittedForDynamicMethods() const
{
    return this->totalILBytesJittedForDynamicMethods;
}

size_t BPerfProfilerCallback::GetTotalNumberOfDynamicMethods() const
{
    return this->totalNumberOfDynamicMethods;
}

size_t BPerfProfilerCallback::GetCurrentNumberOfDynamicMethods() const
{
    return this->currentNumberOfDynamicMethods;
}

/* Precompiled Method Counters */
size_t BPerfProfilerCallback::GetTotalCachedMethodsSearched() const
{
    return this->totalCachedMethodsSearched;
}

size_t BPerfProfilerCallback::GetTotalCachedMethodsRestored() const
{
    return this->totalCachedMethodsRestored;
}

/* Runtime Suspension Counters */
size_t BPerfProfilerCallback::GetTotalNumberOfRuntimeSuspsensions() const
{
    return this->totalNumberOfRuntimeSuspensions;
}

size_t BPerfProfilerCallback::GetTotalNumberOfRuntimeSuspensionsForGC() const
{
    return this->totalNumberOfRuntimeSuspensionsForGC;
}

size_t BPerfProfilerCallback::GetTotalNumberOfRuntimeSuspensionsForGCPrep() const
{
    return this->totalNumberOfRuntimeSuspensionsForGCPrep;
}

/* Type Loader Counters */
size_t BPerfProfilerCallback::GetCurrentNumberOfClassesLoaded() const
{
    return this->currentNumberOfClassesLoaded;
}

size_t BPerfProfilerCallback::GetTotalNumberOfClassesLoaded() const
{
    return this->totalNumberOfClassesLoaded;
}

size_t BPerfProfilerCallback::GetTotalNumberOfClassLoadFailures() const
{
    return this->totalNumberOfClassLoadFailures;
}

size_t BPerfProfilerCallback::GetCurrentNumberOfAssembliesLoaded() const
{
    return this->currentNumberOfAssembliesLoaded;
}

size_t BPerfProfilerCallback::GetTotalNumberOfAssembliesLoaded() const
{
    return this->totalNumberOfAssembliesLoaded;
}

size_t BPerfProfilerCallback::GetCurrentNumberOfModulesLoaded() const
{
    return this->currentNumberOfModulesLoaded;
}

size_t BPerfProfilerCallback::GetTotalNumberOfModulesLoaded() const
{
    return this->totalNumberOfModulesLoaded;
}

/* Thread Counters */
size_t BPerfProfilerCallback::GetCurrentNumberOfLogicalThreads() const
{
    return this->currentNumberOfLogicalThreads;
}

/* GC Counters */
size_t BPerfProfilerCallback::GetTotalNumberOfInducedGarbageCollections() const
{
    return this->totalNumberOfInducedGarbageCollections;
}

size_t BPerfProfilerCallback::GetTotalNumberOfGen0Collections() const
{
    return this->totalNumberOfGen0Collections;
}

size_t BPerfProfilerCallback::GetTotalNumberOfGen1Collections() const
{
    return this->totalNumberOfGen1Collections;
}

size_t BPerfProfilerCallback::GetTotalNumberOfGen2Collections() const
{
    return this->totalNumberOfGen2Collections;
}

size_t BPerfProfilerCallback::GetTotalNumberOfGen3Collections() const
{
    return this->totalNumberOfGen3Collections;
}

size_t BPerfProfilerCallback::GetTotalNumberOfBytesAllocatedSinceLastGC() const
{
    return this->totalNumberOfAllBytesAllocatedSinceLastGC;
}

size_t BPerfProfilerCallback::GetTotalNumberOfPromotedBytes() const
{
    return this->totalPromotedBytes;
}

size_t BPerfProfilerCallback::GetTotalNumberOfBytesInAllHeaps() const
{
    return this->totalNumberOfBytesInAllHeaps;
}

size_t BPerfProfilerCallback::GetGen0HeapSize() const
{
    return this->gen0HeapSize;
}

size_t BPerfProfilerCallback::GetGen1HeapSize() const
{
    return this->gen1HeapSize;
}

size_t BPerfProfilerCallback::GetGen2HeapSize() const
{
    return this->gen2HeapSize;
}

size_t BPerfProfilerCallback::GetGen3HeapSize() const
{
    return this->gen3HeapSize;
}

size_t BPerfProfilerCallback::GetNumberOfGCSegments() const
{
    return this->numberOfGCSegments;
}

size_t BPerfProfilerCallback::GetNumberOfFrozenSegments() const
{
    return this->numberOfFrozenSegments;
}
