#include "BPerfProfilerCallback.h"
#include <vector>
#include "corhlpr.h"

#ifdef _WINDOWS
#include "ETWWindows.h"
#endif

BPerfProfilerCallback::BPerfProfilerCallback()
{
    this->refCount = 0;
    this->corProfilerInfo = nullptr;

    /* Exception Counters */
    this->numberOfExceptionsThrown = 0;
    this->numberOfFiltersExecuted = 0;
    this->numberOfFinallysExecuted = 0;

    /* Metadata Methods Jitting Counters */
    this->totalNumberOfMethodsJitted = 0;
    this->currentNumberOfMethodsJitted = 0;
    this->totalILBytesJitted = 0;
    this->totalMachineCodeBytesGenerated = 0;

    /* Dynamic Methods Jitting Counters */
    this->totalILBytesJittedForDynamicMethods = 0;
    this->totalMachineCodeBytesGeneratedForDynamicMethods = 0;
    this->totalNumberOfDynamicMethods = 0;
    this->currentNumberOfDynamicMethods = 0;

    /* Precompiled Methods Counters */
    this->totalCachedMethodsSearched = 0;
    this->totalCachedMethodsRestored = 0;
    this->totalCachedMethodsMachineCodeBytesRestored = 0;

    /* Runtime Suspension Counters */
    this->totalNumberOfRuntimeSuspensions = 0;
    this->totalNumberOfRuntimeSuspensionsForGC = 0;
    this->totalNumberOfRuntimeSuspensionsForGCPrep = 0;

    /* Type Loader Counters */
    this->currentNumberOfClassesLoaded = 0;
    this->totalNumberOfClassesLoaded = 0;
    this->totalNumberOfClassLoadFailures = 0;

    /* Assembly Loader Counters */
    this->currentNumberOfAssembliesLoaded = 0;
    this->totalNumberOfAssembliesLoaded = 0;
    this->currentNumberOfModulesLoaded = 0;
    this->totalNumberOfModulesLoaded = 0;

    /* Thread Counters */
    this->currentNumberOfLogicalThreads = 0;

    /* GC Counters */
    this->totalNumberOfInducedGarbageCollections = 0;
    this->totalNumberOfGen0Collections = 0;
    this->totalNumberOfGen1Collections = 0;
    this->totalNumberOfGen2Collections = 0;

    this->totalNumberOfAllBytesAllocatedSinceLastGC = 0;
    this->totalNumberOfBytesInAllHeaps = 0;

    this->gen0HeapSize = 0;
    this->gen1HeapSize = 0;
    this->gen2HeapSize = 0;
    this->gen3HeapSize = 0; // LOH
    this->pinnedObjectHeapSize = 0;
    this->frozenHeapSize = 0;

    this->numberOfGCSegments = 0;
    this->numberOfFrozenSegments = 0;
}

BPerfProfilerCallback::~BPerfProfilerCallback() = default;

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::Initialize(IUnknown *pICorProfilerInfoUnk)
{
    const HRESULT queryInterfaceResult = pICorProfilerInfoUnk->QueryInterface(__uuidof(ICorProfilerInfo12), reinterpret_cast<void **>(&this->corProfilerInfo));

    if (FAILED(queryInterfaceResult))
    {
        return E_FAIL;
    }

    DWORD eventsMask = COR_PRF_MONITOR_THREADS          |
                       COR_PRF_MONITOR_SUSPENDS         |
                       COR_PRF_MONITOR_MODULE_LOADS     |
                       COR_PRF_MONITOR_CACHE_SEARCHES   |
                       COR_PRF_MONITOR_JIT_COMPILATION  |
                       COR_PRF_MONITOR_EXCEPTIONS       |
                       COR_PRF_MONITOR_CLASS_LOADS      |
                       COR_PRF_ENABLE_STACK_SNAPSHOT    |
                       COR_PRF_MONITOR_ASSEMBLY_LOADS   ;

    const DWORD eventsHigh = COR_PRF_HIGH_MONITOR_LARGEOBJECT_ALLOCATED    |
                             COR_PRF_HIGH_MONITOR_DYNAMIC_FUNCTION_UNLOADS |
                             COR_PRF_HIGH_BASIC_GC                         |
                             COR_PRF_HIGH_MONITOR_EVENT_PIPE               ;

    const char *monitorObjectAllocated = std::getenv("BPERF_MONITOR_OBJECT_ALLOCATED");
    if (monitorObjectAllocated != nullptr && std::string(monitorObjectAllocated) == std::string("1"))
    {
        eventsMask |= COR_PRF_MONITOR_OBJECT_ALLOCATED;
    }

#ifdef _WINDOWS
    RegisterToETW();
#endif

    return this->corProfilerInfo->SetEventMask2(eventsMask, eventsHigh);
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::Shutdown()
{
#ifdef _WINDOWS
    UnregisterFromETW();
#endif

    ICorProfilerInfo12 *tmp = this->corProfilerInfo;

    if (tmp != nullptr)
    {
        tmp->Release();
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
    ++this->currentNumberOfAssembliesLoaded;
    ++this->totalNumberOfAssembliesLoaded;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::AssemblyUnloadStarted(AssemblyID assemblyId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::AssemblyUnloadFinished(AssemblyID assemblyId, HRESULT hrStatus)
{
    --this->currentNumberOfAssembliesLoaded;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ModuleLoadStarted(ModuleID moduleId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ModuleLoadFinished(ModuleID moduleId, HRESULT hrStatus)
{
    ++this->currentNumberOfModulesLoaded;
    ++this->totalNumberOfModulesLoaded;

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
    if (hrStatus == S_OK)
    {
        ++this->currentNumberOfClassesLoaded;
        ++this->totalNumberOfClassesLoaded;
    }
    else
    {
        ++this->totalNumberOfClassLoadFailures;
    }

    return S_OK;
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
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::JITCompilationFinished(FunctionID functionId, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    ++this->totalNumberOfMethodsJitted;
    ++this->currentNumberOfMethodsJitted;

    ModuleID moduleId;
    mdToken token;

    ICorProfilerInfo12 *tmp = this->corProfilerInfo;
    IfFailRet(tmp->GetFunctionInfo(functionId, nullptr, &moduleId, &token));

    ULONG cbMethodSize;

    IfFailRet(tmp->GetILFunctionBody(moduleId, token, nullptr, &cbMethodSize));
    this->totalILBytesJitted += cbMethodSize;

    ULONG32 cCodeInfos;
    IfFailRet(tmp->GetCodeInfo2(functionId, 0, &cCodeInfos, nullptr));
    std::vector<COR_PRF_CODE_INFO> codeInfos(cCodeInfos);
    IfFailRet(tmp->GetCodeInfo2(functionId, cCodeInfos, &cCodeInfos, codeInfos.data()));

    size_t codeSize = 0;
    for (auto &s : codeInfos)
    {
        codeSize += s.size;
    }

    this->totalMachineCodeBytesGenerated += codeSize;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::JITCachedFunctionSearchStarted(FunctionID functionId, BOOL *pbUseCachedFunction)
{
    ++this->totalCachedMethodsSearched;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::JITCachedFunctionSearchFinished(FunctionID functionId, COR_PRF_JIT_CACHE result)
{
    ICorProfilerInfo12 *tmp = this->corProfilerInfo;

    // Bail out early, so you don't end up causing an SO if for whatever reason this->corProfilerInfo is null.
    // CoreCLR's filter wants to throw an AccessViolation exception, which is not yet restored, so it tries to restore that
    // which in turn calls this function, so on and so forth ...
    if (tmp == nullptr)
    {
        return S_OK;
    }

    if (result == COR_PRF_CACHED_FUNCTION_FOUND)
    {
        ++this->totalCachedMethodsRestored;

        ULONG32 cCodeInfos;

        IfFailRet(tmp->GetCodeInfo2(functionId, 0, &cCodeInfos, nullptr));
        std::vector<COR_PRF_CODE_INFO> codeInfos(cCodeInfos);
        IfFailRet(tmp->GetCodeInfo2(functionId, cCodeInfos, &cCodeInfos, codeInfos.data()));

        size_t codeSize = 0;
        for (auto &s : codeInfos)
        {
            codeSize += s.size;
        }

        this->totalCachedMethodsMachineCodeBytesRestored += codeSize;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::JITFunctionPitched(FunctionID functionId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::JITInlining(FunctionID callerId, FunctionID calleeId, BOOL *pfShouldInline)
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

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RemotingClientSendingMessage(GUID *pCookie, BOOL fIsAsync)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RemotingClientReceivingReply(GUID *pCookie, BOOL fIsAsync)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RemotingClientInvocationFinished()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RemotingServerReceivingMessage(GUID *pCookie, BOOL fIsAsync)
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

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::RemotingServerSendingReply(GUID *pCookie, BOOL fIsAsync)
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
    ++this->totalNumberOfRuntimeSuspensions;

    switch (suspendReason)
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
#ifdef _WINDOWS
    ICorProfilerInfo12 *tmp = this->corProfilerInfo;
    ModuleID moduleId;
    mdTypeDef typeDef;
    SIZE_T size;

    IfFailRet(tmp->GetClassIDInfo2(classId, &moduleId, &typeDef, nullptr, 0, nullptr, nullptr));
    IfFailRet(tmp->GetObjectSize2(objectId, &size));
    ReportObjectAllocation(moduleId, typeDef, size);
#endif
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

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::COMClassicVTableCreated(ClassID wrappedClassId, REFGUID implementedIID, void *pVTable, ULONG cSlots)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::COMClassicVTableDestroyed(ClassID wrappedClassId, REFGUID implementedIID, void *pVTable)
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
            default:
                break;
            }
        }
    }

    ULONG cObjectRanges = 0;

    ICorProfilerInfo12 *tmp = this->corProfilerInfo;
    IfFailRet(tmp->GetGenerationBounds(0, &cObjectRanges, nullptr));
    std::vector<COR_PRF_GC_GENERATION_RANGE> objectRanges(cObjectRanges);
    IfFailRet(tmp->GetGenerationBounds(cObjectRanges, &cObjectRanges, objectRanges.data()));

    size_t sum = 0;
    for (auto &s : objectRanges)
    {
        sum += s.rangeLength;
    }

    this->totalNumberOfAllBytesAllocatedSinceLastGC = sum - this->totalNumberOfBytesInAllHeaps;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::SurvivingReferences(ULONG cSurvivingObjectIDRanges, ObjectID objectIDRangeStart[], ULONG cObjectIDRangeLength[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::GarbageCollectionFinished()
{
    size_t generationSizes[5] = {0, 0, 0, 0, 0};
    size_t frozenSegmentsSize = 0;

    ULONG cObjectRanges = 0;

    ICorProfilerInfo12 *tmp = this->corProfilerInfo;
    IfFailRet(tmp->GetGenerationBounds(0, &cObjectRanges, nullptr));
    std::vector<COR_PRF_GC_GENERATION_RANGE> objectRanges(cObjectRanges);
    IfFailRet(tmp->GetGenerationBounds(cObjectRanges, &cObjectRanges, objectRanges.data()));

    size_t frozenSegmentCount = 0;
    for (auto &s : objectRanges)
    {
        BOOL bFrozen = FALSE;
        tmp->IsFrozenObject(s.rangeStart, &bFrozen);

        if (bFrozen)
        {
            frozenSegmentsSize += s.rangeLength;
            ++frozenSegmentCount;
        }
        else
        {
            generationSizes[s.generation] += s.rangeLength;
        }
    }

    this->numberOfGCSegments = cObjectRanges;
    this->numberOfFrozenSegments = frozenSegmentCount;

    this->gen0HeapSize = generationSizes[COR_PRF_GC_GEN_0];
    this->gen1HeapSize = generationSizes[COR_PRF_GC_GEN_1];
    this->gen2HeapSize = generationSizes[COR_PRF_GC_GEN_2];
    this->gen3HeapSize = generationSizes[COR_PRF_GC_LARGE_OBJECT_HEAP];
    this->pinnedObjectHeapSize = generationSizes[COR_PRF_GC_PINNED_OBJECT_HEAP];
    this->frozenHeapSize = frozenSegmentsSize;

    this->totalNumberOfBytesInAllHeaps = generationSizes[COR_PRF_GC_GEN_0] + generationSizes[COR_PRF_GC_GEN_1] + generationSizes[COR_PRF_GC_GEN_2] + generationSizes[COR_PRF_GC_LARGE_OBJECT_HEAP] + generationSizes[COR_PRF_GC_PINNED_OBJECT_HEAP] + frozenSegmentsSize;

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

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::InitializeForAttach(IUnknown *pCorProfilerInfoUnk, void *pvClientData, UINT cbClientData)
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

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl)
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

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::GetAssemblyReferences(const WCHAR *wszAssemblyPath, ICorProfilerAssemblyReferenceProvider *pAsmRefProvider)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::ModuleInMemorySymbolsUpdated(ModuleID moduleId)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::DynamicMethodJITCompilationStarted(FunctionID functionId, BOOL fIsSafeToBlock, LPCBYTE ilHeader, ULONG cbILHeader)
{
    this->totalILBytesJittedForDynamicMethods += cbILHeader;

    ++this->totalNumberOfDynamicMethods;
    ++this->currentNumberOfDynamicMethods;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::DynamicMethodJITCompilationFinished(FunctionID functionId, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    ULONG32 cCodeInfos;

    ICorProfilerInfo12 *tmp = this->corProfilerInfo;
    IfFailRet(tmp->GetCodeInfo2(functionId, 0, &cCodeInfos, nullptr));
    std::vector<COR_PRF_CODE_INFO> codeInfos(cCodeInfos);
    IfFailRet(tmp->GetCodeInfo2(functionId, cCodeInfos, &cCodeInfos, codeInfos.data()));

    size_t codeSize = 0;
    for (auto &s : codeInfos)
    {
        codeSize += s.size;
    }

    this->totalMachineCodeBytesGeneratedForDynamicMethods += codeSize;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::DynamicMethodUnloaded(FunctionID functionId)
{
    --this->currentNumberOfDynamicMethods;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::EventPipeEventDelivered(EVENTPIPE_PROVIDER provider, DWORD eventId, DWORD eventVersion, ULONG cbMetadataBlob, LPCBYTE metadataBlob, ULONG cbEventData, LPCBYTE eventData, LPCGUID pActivityId, LPCGUID pRelatedActivityId, ThreadID eventThread, ULONG numStackFrames, UINT_PTR stackFrames[])
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BPerfProfilerCallback::EventPipeProviderCreated(EVENTPIPE_PROVIDER provider)
{
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

size_t BPerfProfilerCallback::GetTotalMachineCodeBytesGeneratedForDynamicMethods() const
{
    return this->totalMachineCodeBytesGeneratedForDynamicMethods;
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

size_t BPerfProfilerCallback::GetTotalCachedMethodsMachineCodeBytesRestored() const
{
    return this->totalCachedMethodsMachineCodeBytesRestored;
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

size_t BPerfProfilerCallback::GetTotalNumberOfBytesAllocatedSinceLastGC() const
{
    return this->totalNumberOfAllBytesAllocatedSinceLastGC;
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

size_t BPerfProfilerCallback::GetPinnedObjectHeapSize() const
{
    return this->pinnedObjectHeapSize;
}

size_t BPerfProfilerCallback::GetFrozenHeapSize() const
{
    return this->frozenHeapSize;
}

size_t BPerfProfilerCallback::GetNumberOfGCSegments() const
{
    return this->numberOfGCSegments;
}

size_t BPerfProfilerCallback::GetNumberOfFrozenSegments() const
{
    return this->numberOfFrozenSegments;
}

bool BPerfProfilerCallback::EnableObjectAllocationMonitoring() const
{
    DWORD eventsLow, eventsHigh;

    ICorProfilerInfo12 *tmp = this->corProfilerInfo;
    tmp->GetEventMask2(&eventsLow, &eventsHigh);
    eventsLow |= (COR_PRF_ENABLE_OBJECT_ALLOCATED);
    return tmp->SetEventMask2(eventsLow, eventsHigh) == S_OK;
}

bool BPerfProfilerCallback::DisableObjectAllocationMonitoring() const
{
    DWORD eventsLow, eventsHigh;

    ICorProfilerInfo12 *tmp = this->corProfilerInfo;
    tmp->GetEventMask2(&eventsLow, &eventsHigh);
    eventsLow &= ~(COR_PRF_ENABLE_OBJECT_ALLOCATED);
    return tmp->SetEventMask2(eventsLow, eventsHigh) == S_OK;
}
