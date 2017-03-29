// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*
/modules
/assemblies
/appdomains
/jittedMethods
/nativeMethods
/threads

/rejit?moduleId=...&methodDefToken=...&il=...
/gc


/enableELTMonitoring
/enableNewObjMonitoring
/enableCPUSamplingUsingELTOrThreadSuspend

BPERF_ENABLE_CALL_COUNT

/showDisassembly/methodId
/EIPToFunctionID
/exceptions

ModuleId,ModuleName
0x4334343,ModuleName
*/

// TODO:
// (1) Build a web service that can take an EIP and find FunctionID
// (2) A Symbolization service /Eip+Time
// (3) Source Line info too
// (4) Exceptions
// (5) Querying the Shadow Stack
// (6) Supply capapbility to change web server url (and how much info we cache, etc).
// (7) General debugging service (given functionID, show me name)
// (8) Given some function names (Strings) tell me if they're hot how much time they're spending
// (9) Object allocations (tell me if they allocated objects), etc.
// (10) memory leaks
// (11) Request REJIT from function strings
// (12) Loaded Module List
// (13) Enumerate JITTED functions ---> EnumJITedFunctions2
// (14) EnumThreads
// (15) GetReJITIDs
// (16) ForceGC
// (17) Flight Recorder Mode
// (18) call count data

// Open Questions:
// (1) Can GetFunctionFromIP2 be used to get FunctionIDs? Should be, how does one know from EIP if it's a rejitted function or not

#include "profiler_pal.h"
#include "BPerfProfiler.h"
#include "corhlpr.h"
#include "JITCompilationStartedEvent.h"
#include "AppDomainUnloadEvent.h"
#include "RuntimeInformationEvent.h"
#include "ShutdownEvent.h"
#include "SQLiteEventLogger.h"
#include "StartupEvent.h"
#include "RundownList.h"
#include "EtwLogger.h"
#include "ILRewriter.h"
#include "CComPtr.h"
#include "MethodEnterEvent.h"

thread_local std::vector<FunctionID> ShadowStack;

static void STDMETHODCALLTYPE Enter(FunctionID functionId)
{
    ShadowStack.push_back(functionId);
    ProfilerInstance->eventQueue->push(LogEvent<MethodEnterEvent>(functionId, &ShadowStack));
}

static void STDMETHODCALLTYPE Leave(FunctionID functionId)
{
    ShadowStack.pop_back();
}

void(STDMETHODCALLTYPE *EnterMethodAddress)(FunctionID) = &Enter;
void(STDMETHODCALLTYPE *LeaveMethodAddress)(FunctionID) = &Leave;
COR_SIGNATURE enterLeaveMethodSignature[] = { IMAGE_CEE_CS_CALLCONV_STDCALL, 0x01, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I };

BPerfProfiler::BPerfProfiler() : eventQueue(std::make_shared<ThreadSafeQueue<std::unique_ptr<EtwEvent>>>()), clrInstanceId(0), refCount(0), corProfilerInfo(nullptr), shouldMaintainShadowStack(false)
{
    if (IsEnvironmentVariableEnabled("BPERF_DELAYED_STARTUP"))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }

    this->eventQueue->push(LogEvent<StartupEvent>(this->clrInstanceId));

    this->eventLoggerThread = std::thread([this] {

        const std::string bPerfLogsPath = [&]() {
            const char *ptr = std::getenv("BPERF_LOG_PATH");
            return ptr != nullptr ? ptr : "BPerfLog.db"; // default file name, in the current working directory
        }();

        if (IsEnvironmentVariableEnabled("BPERF_OVERWRITE_LOGS"))
        {
            portable_wide_string wideString;
            int wlen = MultiByteToWideChar(CP_UTF8, 0, bPerfLogsPath.c_str(), -1, nullptr, 0);
            if (wlen > 0)
            {
                wideString.resize(wlen - 1);
                MultiByteToWideChar(CP_UTF8, 0, bPerfLogsPath.c_str(), -1, addr(wideString), wlen);
                DeleteFile(wideString.c_str());
            }
        }

        SQLiteEventLogger logger(this->db, this->eventQueue, bPerfLogsPath);
        logger.RunEventLoop();
    });
}

BPerfProfiler::~BPerfProfiler()
{
    if (this->corProfilerInfo != nullptr)
    {
        this->corProfilerInfo->Release();
        this->corProfilerInfo = nullptr;
    }
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::Initialize(IUnknown *pICorProfilerInfoUnk)
{
    HRESULT queryInterfaceResult = pICorProfilerInfoUnk->QueryInterface(__uuidof(ICorProfilerInfo7), reinterpret_cast<void **>(&this->corProfilerInfo));

    if (FAILED(queryInterfaceResult))
    {
        return E_FAIL;
    }

    DWORD eventMask = COR_PRF_DISABLE_TRANSPARENCY_CHECKS_UNDER_FULL_TRUST |
                      COR_PRF_MONITOR_THREADS |
                      COR_PRF_MONITOR_SUSPENDS |
                      COR_PRF_MONITOR_MODULE_LOADS |
                      COR_PRF_MONITOR_JIT_COMPILATION |
                      COR_PRF_MONITOR_FUNCTION_UNLOADS |
                      COR_PRF_MONITOR_EXCEPTIONS |
                      COR_PRF_MONITOR_CLR_EXCEPTIONS |
                      COR_PRF_MONITOR_CLASS_LOADS |
                      COR_PRF_MONITOR_ASSEMBLY_LOADS |
                      COR_PRF_MONITOR_APPDOMAIN_LOADS;

    if (IsEnvironmentVariableEnabled("BPERF_MONITOR_ALLOCATIONS"))
    {
        eventMask |= COR_PRF_MONITOR_OBJECT_ALLOCATED;
    }

    if (IsEnvironmentVariableEnabled("BPERF_MONITOR_ENTERLEAVE"))
    {
        this->shouldMaintainShadowStack = true;
    }

    COR_PRF_RUNTIME_TYPE sku;
    USHORT majorVersion, minorVersion, buildNumber, qfeVersion;
    this->corProfilerInfo->GetRuntimeInformation(&this->clrInstanceId, &sku, &majorVersion, &minorVersion, &buildNumber, &qfeVersion, 0, nullptr, nullptr);
    this->eventQueue->push(LogEvent<RuntimeInformationEvent>(this->clrInstanceId, sku, majorVersion, minorVersion, buildNumber, qfeVersion));

    ProfilerInstance = this;

#ifdef _WINDOWS
    EventRegisterMicrosoft_Windows_DotNETRuntimeRundown();
#endif

    return this->corProfilerInfo->SetEventMask(eventMask);
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::Shutdown()
{
#ifdef _WINDOWS
    EventUnregisterMicrosoft_Windows_DotNETRuntimeRundown();
#endif

    std::lock_guard<std::mutex> lock(RundownMutex);

    ProfilerInstance = nullptr;

    this->eventQueue->push(LogEvent<ShutdownEvent>(this->clrInstanceId));
    this->eventLoggerThread.join();

    if (this->corProfilerInfo != nullptr)
    {
        this->corProfilerInfo->Release();
        this->corProfilerInfo = nullptr;
    }

    return S_OK;
}

void STDMETHODCALLTYPE BPerfProfiler::Rundown()
{
#ifdef _WINDOWS
    this->appDomainRundown.Iterate([](auto data) { EtwLogger::WriteEvent(data); });
    this->assemblyRundown.Iterate([](auto data) { EtwLogger::WriteEvent(data); });
    this->moduleRundown.Iterate([](auto data) { EtwLogger::WriteEvent(data); });
    this->methodRundown.Iterate([](auto data) { EtwLogger::WriteEvent(data); });
    this->ilNativeMapRundown.Iterate([](auto data) { EtwLogger::WriteEvent(data); });
#endif
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::AppDomainCreationStarted(AppDomainID appDomainId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::AppDomainCreationFinished(AppDomainID appDomainId, HRESULT hrStatus)
{
    HRESULT hr;
    IfFailRet(hrStatus);

    ULONG appDomainNameLength = 0;

    this->corProfilerInfo->GetAppDomainInfo(appDomainId, 0, &appDomainNameLength, nullptr, nullptr);
    portable_wide_string appDomainName(appDomainNameLength, ZEROSTRING);
    IfFailRet(this->corProfilerInfo->GetAppDomainInfo(appDomainId, appDomainNameLength, &appDomainNameLength, addr(appDomainName), nullptr));

    auto temp = this->appDomainRundown.Add(LogEvent2<AppDomainLoadEvent>(this->eventQueue, appDomainId, 0, appDomainName, 0, this->clrInstanceId));

    {
        std::lock_guard<std::mutex> lock(this->appDomainMutex);
        this->appDomainInfoMap[appDomainId] = temp;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::AppDomainShutdownStarted(AppDomainID appDomainId)
{
    HRESULT hr;
    ULONG appDomainNameLength = 0;

    this->corProfilerInfo->GetAppDomainInfo(appDomainId, 0, &appDomainNameLength, nullptr, nullptr);

    portable_wide_string appDomainName(appDomainNameLength, ZEROSTRING);
    IfFailRet(this->corProfilerInfo->GetAppDomainInfo(appDomainId, appDomainNameLength, &appDomainNameLength, addr(appDomainName), nullptr));

    this->eventQueue->push(LogEvent<AppDomainUnloadEvent>(appDomainId, 0, appDomainName, 0, this->clrInstanceId));

    {
        std::lock_guard<std::mutex> lock(this->appDomainMutex);

        auto iter = this->appDomainInfoMap.find(appDomainId);
        if (iter != this->appDomainInfoMap.end())
        {
            this->appDomainInfoMap.erase(iter);
            this->appDomainRundown.Remove(iter->second);
        }
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::AppDomainShutdownFinished(AppDomainID appDomainId, HRESULT hrStatus)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::AssemblyLoadStarted(AssemblyID assemblyId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::AssemblyLoadFinished(AssemblyID assemblyId, HRESULT hrStatus)
{
    HRESULT hr;
    IfFailRet(hrStatus);

    hr = this->AssemblyLoadUnloadData(assemblyId, true);
    return hr;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::AssemblyUnloadStarted(AssemblyID assemblyId)
{
    return this->AssemblyLoadUnloadData(assemblyId, false);
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::AssemblyUnloadFinished(AssemblyID assemblyId, HRESULT hrStatus)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ModuleLoadStarted(ModuleID moduleId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ModuleLoadFinished(ModuleID moduleId, HRESULT hrStatus)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ModuleUnloadStarted(ModuleID moduleId)
{
    return this->ModuleLoadUnloadData(moduleId, false);
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ModuleUnloadFinished(ModuleID moduleId, HRESULT hrStatus)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ModuleAttachedToAssembly(ModuleID moduleId, AssemblyID assemblyId)
{
    return this->ModuleLoadUnloadData(moduleId, true);
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ClassLoadStarted(ClassID classId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ClassLoadFinished(ClassID classId, HRESULT hrStatus)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ClassUnloadStarted(ClassID classId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ClassUnloadFinished(ClassID classId, HRESULT hrStatus)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::FunctionUnloadStarted(FunctionID functionId)
{
    return this->MethodJitStartFinishData(functionId, /* finished */ true, /* unload */ true);
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::JITCompilationStarted(FunctionID functionId, BOOL fIsSafeToBlock)
{
    HRESULT hr;

    if (this->shouldMaintainShadowStack)
    {
        mdToken token;
        ClassID classId;
        ModuleID moduleId;

        IfFailRet(this->corProfilerInfo->GetFunctionInfo(functionId, &classId, &moduleId, &token));

        CComPtr<IMetaDataImport> metadataImport;
        IfFailRet(this->corProfilerInfo->GetModuleMetaData(moduleId, ofRead | ofWrite, IID_IMetaDataImport, reinterpret_cast<IUnknown **>(&metadataImport)));

        CComPtr<IMetaDataEmit> metadataEmit;
        IfFailRet(metadataImport->QueryInterface(IID_IMetaDataEmit, reinterpret_cast<void **>(&metadataEmit)));

        mdSignature enterLeaveMethodSignatureToken;
        metadataEmit->GetTokenFromSig(enterLeaveMethodSignature, sizeof(enterLeaveMethodSignature), &enterLeaveMethodSignatureToken);

        IfFailRet(RewriteIL(this->corProfilerInfo, nullptr, moduleId, token, functionId, reinterpret_cast<ULONGLONG>(EnterMethodAddress), reinterpret_cast<ULONGLONG>(LeaveMethodAddress), enterLeaveMethodSignatureToken));
    }

    return this->MethodJitStartFinishData(functionId, /* finished */ false, /* unload */ false);
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::JITCompilationFinished(FunctionID functionId, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    HRESULT hr;
    IfFailRet(hrStatus);

    this->MethodJitStartFinishData(functionId, /* finished */ true, /* unload */ false);

    ULONG32 nativeMapsCount;

    IfFailRet(this->corProfilerInfo->GetILToNativeMapping(functionId, 0, &nativeMapsCount, nullptr));
    std::vector<COR_DEBUG_IL_TO_NATIVE_MAP> nativeMaps(nativeMapsCount);
    IfFailRet(this->corProfilerInfo->GetILToNativeMapping(functionId, nativeMapsCount, &nativeMapsCount, nativeMaps.data()));

    std::vector<uint32_t> ilOffsets, nativeOffsets;
    ilOffsets.resize(nativeMapsCount);
    nativeOffsets.resize(nativeMapsCount);

    for (auto const &e : nativeMaps)
    {
        ilOffsets.emplace_back(e.ilOffset);
        nativeOffsets.emplace_back(e.nativeStartOffset);
    }

    auto temp = this->ilNativeMapRundown.Add(LogEvent2<MethodILToNativeMapEvent>(this->eventQueue, functionId, 0, 0, nativeMapsCount, ilOffsets, nativeOffsets, this->clrInstanceId));

    {
        std::lock_guard<std::mutex> lock(this->ilNativeMutex);
        this->ilNativeInfoMap[functionId] = temp;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::JITCachedFunctionSearchStarted(FunctionID functionId, BOOL *pbUseCachedFunction)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::JITCachedFunctionSearchFinished(FunctionID functionId, COR_PRF_JIT_CACHE result)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::JITFunctionPitched(FunctionID functionId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::JITInlining(FunctionID callerId, FunctionID calleeId, BOOL *pfShouldInline)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ThreadCreated(ThreadID threadId)
{
	ShadowStack.reserve(512); // reserve to accomodate 512 frames without allocation
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ThreadDestroyed(ThreadID threadId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ThreadAssignedToOSThread(ThreadID managedThreadId, DWORD osThreadId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RemotingClientInvocationStarted()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RemotingClientSendingMessage(GUID *pCookie, BOOL fIsAsync)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RemotingClientReceivingReply(GUID *pCookie, BOOL fIsAsync)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RemotingClientInvocationFinished()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RemotingServerReceivingMessage(GUID *pCookie, BOOL fIsAsync)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RemotingServerInvocationStarted()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RemotingServerInvocationReturned()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RemotingServerSendingReply(GUID *pCookie, BOOL fIsAsync)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::UnmanagedToManagedTransition(FunctionID functionId, COR_PRF_TRANSITION_REASON reason)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ManagedToUnmanagedTransition(FunctionID functionId, COR_PRF_TRANSITION_REASON reason)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RuntimeSuspendStarted(COR_PRF_SUSPEND_REASON suspendReason)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RuntimeSuspendFinished()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RuntimeSuspendAborted()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RuntimeResumeStarted()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RuntimeResumeFinished()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RuntimeThreadSuspended(ThreadID threadId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RuntimeThreadResumed(ThreadID threadId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::MovedReferences(ULONG cMovedObjectIDRanges, ObjectID oldObjectIDRangeStart[], ObjectID newObjectIDRangeStart[], ULONG cObjectIDRangeLength[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ObjectAllocated(ObjectID objectId, ClassID classId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ObjectsAllocatedByClass(ULONG cClassCount, ClassID classIds[], ULONG cObjects[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ObjectReferences(ObjectID objectId, ClassID classId, ULONG cObjectRefs, ObjectID objectRefIds[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RootReferences(ULONG cRootRefs, ObjectID rootRefIds[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionThrown(ObjectID thrownObjectId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionSearchFunctionEnter(FunctionID functionId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionSearchFunctionLeave()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionSearchFilterEnter(FunctionID functionId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionSearchFilterLeave()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionSearchCatcherFound(FunctionID functionId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionOSHandlerEnter(UINT_PTR __unused)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionOSHandlerLeave(UINT_PTR __unused)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionUnwindFunctionEnter(FunctionID functionId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionUnwindFunctionLeave()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionUnwindFinallyEnter(FunctionID functionId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionUnwindFinallyLeave()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionCatcherEnter(FunctionID functionId, ObjectID objectId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionCatcherLeave()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::COMClassicVTableCreated(ClassID wrappedClassId, REFGUID implementedIID, void *pVTable, ULONG cSlots)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::COMClassicVTableDestroyed(ClassID wrappedClassId, REFGUID implementedIID, void *pVTable)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionCLRCatcherFound()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ExceptionCLRCatcherExecute()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ThreadNameChanged(ThreadID threadId, ULONG cchName, WCHAR name[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::GarbageCollectionStarted(int cGenerations, BOOL generationCollected[], COR_PRF_GC_REASON reason)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::SurvivingReferences(ULONG cSurvivingObjectIDRanges, ObjectID objectIDRangeStart[], ULONG cObjectIDRangeLength[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::GarbageCollectionFinished()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::FinalizeableObjectQueued(DWORD finalizerFlags, ObjectID objectID)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::RootReferences2(ULONG cRootRefs, ObjectID rootRefIds[], COR_PRF_GC_ROOT_KIND rootKinds[], COR_PRF_GC_ROOT_FLAGS rootFlags[], UINT_PTR rootIds[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::HandleCreated(GCHandleID handleId, ObjectID initialObjectId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::HandleDestroyed(GCHandleID handleId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::InitializeForAttach(IUnknown *pCorProfilerInfoUnk, void *pvClientData, UINT cbClientData)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ProfilerAttachComplete()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ProfilerDetachSucceeded()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ReJITCompilationStarted(FunctionID functionId, ReJITID rejitId, BOOL fIsSafeToBlock)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ReJITCompilationFinished(FunctionID functionId, ReJITID rejitId, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    HRESULT hr;
    ULONG32 codeInfosCount;

    IfFailRet(this->corProfilerInfo->GetCodeInfo3(functionId, rejitId, 0, &codeInfosCount, nullptr));
    std::vector<COR_PRF_CODE_INFO> codeInfos(codeInfosCount);
    IfFailRet(this->corProfilerInfo->GetCodeInfo3(functionId, rejitId, codeInfosCount, nullptr, codeInfos.data()));

    ULONG32 nativeMapsCount;

    IfFailRet(this->corProfilerInfo->GetILToNativeMapping2(functionId, rejitId, 0, &nativeMapsCount, nullptr));
    std::vector<COR_DEBUG_IL_TO_NATIVE_MAP> nativeMaps(nativeMapsCount);
    IfFailRet(this->corProfilerInfo->GetILToNativeMapping2(functionId, rejitId, nativeMapsCount, nullptr, nativeMaps.data()));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ReJITError(ModuleID moduleId, mdMethodDef methodId, FunctionID functionId, HRESULT hrStatus)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::MovedReferences2(ULONG cMovedObjectIDRanges, ObjectID oldObjectIDRangeStart[], ObjectID newObjectIDRangeStart[], SIZE_T cObjectIDRangeLength[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::SurvivingReferences2(ULONG cSurvivingObjectIDRanges, ObjectID objectIDRangeStart[], SIZE_T cObjectIDRangeLength[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ConditionalWeakTableElementReferences(ULONG cRootRefs, ObjectID keyRefIds[], ObjectID valueRefIds[], GCHandleID rootIds[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::GetAssemblyReferences(const WCHAR *wszAssemblyPath, ICorProfilerAssemblyReferenceProvider *pAsmRefProvider)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::ModuleInMemorySymbolsUpdated(ModuleID moduleId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::DynamicMethodJITCompilationStarted(FunctionID functionId, BOOL fIsSafeToBlock, LPCBYTE ilHeader, ULONG cbILHeader)
{
    return this->DynamicMethodJitStartFinishData(functionId, cbILHeader, /* finished */ false);
}

HRESULT STDMETHODCALLTYPE BPerfProfiler::DynamicMethodJITCompilationFinished(FunctionID functionId, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    return this->DynamicMethodJitStartFinishData(functionId, 0 /* not needed */, /* finished */ true);
}