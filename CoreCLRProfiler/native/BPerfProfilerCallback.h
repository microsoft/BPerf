// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "profiler_pal.h"

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#undef IfFailRet
#define IfFailRet(EXPR) do { HRESULT hr = (EXPR); if(FAILED(hr)) { return (hr); } } while (0)

#include <cstddef>
#include <atomic>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <fstream>
#include "cor.h"
#include "corprof.h"

class BPerfProfilerCallback final : public ICorProfilerCallback9
{
public:
    BPerfProfilerCallback();
    virtual ~BPerfProfilerCallback();
    HRESULT STDMETHODCALLTYPE Initialize(IUnknown* pICorProfilerInfoUnk) override;
    HRESULT STDMETHODCALLTYPE Shutdown() override;
    HRESULT STDMETHODCALLTYPE AppDomainCreationStarted(AppDomainID appDomainId) override;
    HRESULT STDMETHODCALLTYPE AppDomainCreationFinished(AppDomainID appDomainId, HRESULT hrStatus) override;
    HRESULT STDMETHODCALLTYPE AppDomainShutdownStarted(AppDomainID appDomainId) override;
    HRESULT STDMETHODCALLTYPE AppDomainShutdownFinished(AppDomainID appDomainId, HRESULT hrStatus) override;
    HRESULT STDMETHODCALLTYPE AssemblyLoadStarted(AssemblyID assemblyId) override;
    HRESULT STDMETHODCALLTYPE AssemblyLoadFinished(AssemblyID assemblyId, HRESULT hrStatus) override;
    HRESULT STDMETHODCALLTYPE AssemblyUnloadStarted(AssemblyID assemblyId) override;
    HRESULT STDMETHODCALLTYPE AssemblyUnloadFinished(AssemblyID assemblyId, HRESULT hrStatus) override;
    HRESULT STDMETHODCALLTYPE ModuleLoadStarted(ModuleID moduleId) override;
    HRESULT STDMETHODCALLTYPE ModuleLoadFinished(ModuleID moduleId, HRESULT hrStatus) override;
    HRESULT STDMETHODCALLTYPE ModuleUnloadStarted(ModuleID moduleId) override;
    HRESULT STDMETHODCALLTYPE ModuleUnloadFinished(ModuleID moduleId, HRESULT hrStatus) override;
    HRESULT STDMETHODCALLTYPE ModuleAttachedToAssembly(ModuleID moduleId, AssemblyID AssemblyId) override;
    HRESULT STDMETHODCALLTYPE ClassLoadStarted(ClassID classId) override;
    HRESULT STDMETHODCALLTYPE ClassLoadFinished(ClassID classId, HRESULT hrStatus) override;
    HRESULT STDMETHODCALLTYPE ClassUnloadStarted(ClassID classId) override;
    HRESULT STDMETHODCALLTYPE ClassUnloadFinished(ClassID classId, HRESULT hrStatus) override;
    HRESULT STDMETHODCALLTYPE FunctionUnloadStarted(FunctionID functionId) override;
    HRESULT STDMETHODCALLTYPE JITCompilationStarted(FunctionID functionId, BOOL fIsSafeToBlock) override;
    HRESULT STDMETHODCALLTYPE JITCompilationFinished(FunctionID functionId, HRESULT hrStatus, BOOL fIsSafeToBlock) override;
    HRESULT STDMETHODCALLTYPE JITCachedFunctionSearchStarted(FunctionID functionId, BOOL* pbUseCachedFunction) override;
    HRESULT STDMETHODCALLTYPE JITCachedFunctionSearchFinished(FunctionID functionId, COR_PRF_JIT_CACHE result) override;
    HRESULT STDMETHODCALLTYPE JITFunctionPitched(FunctionID functionId) override;
    HRESULT STDMETHODCALLTYPE JITInlining(FunctionID callerId, FunctionID calleeId, BOOL* pfShouldInline) override;
    HRESULT STDMETHODCALLTYPE ThreadCreated(ThreadID threadId) override;
    HRESULT STDMETHODCALLTYPE ThreadDestroyed(ThreadID threadId) override;
    HRESULT STDMETHODCALLTYPE ThreadAssignedToOSThread(ThreadID managedThreadId, DWORD osThreadId) override;
    HRESULT STDMETHODCALLTYPE RemotingClientInvocationStarted() override;
    HRESULT STDMETHODCALLTYPE RemotingClientSendingMessage(GUID* pCookie, BOOL fIsAsync) override;
    HRESULT STDMETHODCALLTYPE RemotingClientReceivingReply(GUID* pCookie, BOOL fIsAsync) override;
    HRESULT STDMETHODCALLTYPE RemotingClientInvocationFinished() override;
    HRESULT STDMETHODCALLTYPE RemotingServerReceivingMessage(GUID* pCookie, BOOL fIsAsync) override;
    HRESULT STDMETHODCALLTYPE RemotingServerInvocationStarted() override;
    HRESULT STDMETHODCALLTYPE RemotingServerInvocationReturned() override;
    HRESULT STDMETHODCALLTYPE RemotingServerSendingReply(GUID* pCookie, BOOL fIsAsync) override;
    HRESULT STDMETHODCALLTYPE UnmanagedToManagedTransition(FunctionID functionId, COR_PRF_TRANSITION_REASON reason) override;
    HRESULT STDMETHODCALLTYPE ManagedToUnmanagedTransition(FunctionID functionId, COR_PRF_TRANSITION_REASON reason) override;
    HRESULT STDMETHODCALLTYPE RuntimeSuspendStarted(COR_PRF_SUSPEND_REASON suspendReason) override;
    HRESULT STDMETHODCALLTYPE RuntimeSuspendFinished() override;
    HRESULT STDMETHODCALLTYPE RuntimeSuspendAborted() override;
    HRESULT STDMETHODCALLTYPE RuntimeResumeStarted() override;
    HRESULT STDMETHODCALLTYPE RuntimeResumeFinished() override;
    HRESULT STDMETHODCALLTYPE RuntimeThreadSuspended(ThreadID threadId) override;
    HRESULT STDMETHODCALLTYPE RuntimeThreadResumed(ThreadID threadId) override;
    HRESULT STDMETHODCALLTYPE MovedReferences(ULONG cMovedObjectIDRanges, ObjectID oldObjectIDRangeStart[], ObjectID newObjectIDRangeStart[], ULONG cObjectIDRangeLength[]) override;
    HRESULT STDMETHODCALLTYPE ObjectAllocated(ObjectID objectId, ClassID classId) override;
    HRESULT STDMETHODCALLTYPE ObjectsAllocatedByClass(ULONG cClassCount, ClassID classIds[], ULONG cObjects[]) override;
    HRESULT STDMETHODCALLTYPE ObjectReferences(ObjectID objectId, ClassID classId, ULONG cObjectRefs, ObjectID objectRefIds[]) override;
    HRESULT STDMETHODCALLTYPE RootReferences(ULONG cRootRefs, ObjectID rootRefIds[]) override;
    HRESULT STDMETHODCALLTYPE ExceptionThrown(ObjectID thrownObjectId) override;
    HRESULT STDMETHODCALLTYPE ExceptionSearchFunctionEnter(FunctionID functionId) override;
    HRESULT STDMETHODCALLTYPE ExceptionSearchFunctionLeave() override;
    HRESULT STDMETHODCALLTYPE ExceptionSearchFilterEnter(FunctionID functionId) override;
    HRESULT STDMETHODCALLTYPE ExceptionSearchFilterLeave() override;
    HRESULT STDMETHODCALLTYPE ExceptionSearchCatcherFound(FunctionID functionId) override;
    HRESULT STDMETHODCALLTYPE ExceptionOSHandlerEnter(UINT_PTR __unused) override;
    HRESULT STDMETHODCALLTYPE ExceptionOSHandlerLeave(UINT_PTR __unused) override;
    HRESULT STDMETHODCALLTYPE ExceptionUnwindFunctionEnter(FunctionID functionId) override;
    HRESULT STDMETHODCALLTYPE ExceptionUnwindFunctionLeave() override;
    HRESULT STDMETHODCALLTYPE ExceptionUnwindFinallyEnter(FunctionID functionId) override;
    HRESULT STDMETHODCALLTYPE ExceptionUnwindFinallyLeave() override;
    HRESULT STDMETHODCALLTYPE ExceptionCatcherEnter(FunctionID functionId, ObjectID objectId) override;
    HRESULT STDMETHODCALLTYPE ExceptionCatcherLeave() override;
    HRESULT STDMETHODCALLTYPE COMClassicVTableCreated(ClassID wrappedClassId, REFGUID implementedIID, void* pVTable, ULONG cSlots) override;
    HRESULT STDMETHODCALLTYPE COMClassicVTableDestroyed(ClassID wrappedClassId, REFGUID implementedIID, void* pVTable) override;
    HRESULT STDMETHODCALLTYPE ExceptionCLRCatcherFound() override;
    HRESULT STDMETHODCALLTYPE ExceptionCLRCatcherExecute() override;
    HRESULT STDMETHODCALLTYPE ThreadNameChanged(ThreadID threadId, ULONG cchName, WCHAR name[]) override;
    HRESULT STDMETHODCALLTYPE GarbageCollectionStarted(int cGenerations, BOOL generationCollected[], COR_PRF_GC_REASON reason) override;
    HRESULT STDMETHODCALLTYPE SurvivingReferences(ULONG cSurvivingObjectIDRanges, ObjectID objectIDRangeStart[], ULONG cObjectIDRangeLength[]) override;
    HRESULT STDMETHODCALLTYPE GarbageCollectionFinished() override;
    HRESULT STDMETHODCALLTYPE FinalizeableObjectQueued(DWORD finalizerFlags, ObjectID objectID) override;
    HRESULT STDMETHODCALLTYPE RootReferences2(ULONG cRootRefs, ObjectID rootRefIds[], COR_PRF_GC_ROOT_KIND rootKinds[], COR_PRF_GC_ROOT_FLAGS rootFlags[], UINT_PTR rootIds[]) override;
    HRESULT STDMETHODCALLTYPE HandleCreated(GCHandleID handleId, ObjectID initialObjectId) override;
    HRESULT STDMETHODCALLTYPE HandleDestroyed(GCHandleID handleId) override;
    HRESULT STDMETHODCALLTYPE InitializeForAttach(IUnknown* pCorProfilerInfoUnk, void* pvClientData, UINT cbClientData) override;
    HRESULT STDMETHODCALLTYPE ProfilerAttachComplete() override;
    HRESULT STDMETHODCALLTYPE ProfilerDetachSucceeded() override;
    HRESULT STDMETHODCALLTYPE ReJITCompilationStarted(FunctionID functionId, ReJITID rejitId, BOOL fIsSafeToBlock) override;
    HRESULT STDMETHODCALLTYPE GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl* pFunctionControl) override;
    HRESULT STDMETHODCALLTYPE ReJITCompilationFinished(FunctionID functionId, ReJITID rejitId, HRESULT hrStatus, BOOL fIsSafeToBlock) override;
    HRESULT STDMETHODCALLTYPE ReJITError(ModuleID moduleId, mdMethodDef methodId, FunctionID functionId, HRESULT hrStatus) override;
    HRESULT STDMETHODCALLTYPE MovedReferences2(ULONG cMovedObjectIDRanges, ObjectID oldObjectIDRangeStart[], ObjectID newObjectIDRangeStart[], SIZE_T cObjectIDRangeLength[]) override;
    HRESULT STDMETHODCALLTYPE SurvivingReferences2(ULONG cSurvivingObjectIDRanges, ObjectID objectIDRangeStart[], SIZE_T cObjectIDRangeLength[]) override;
    HRESULT STDMETHODCALLTYPE ConditionalWeakTableElementReferences(ULONG cRootRefs, ObjectID keyRefIds[], ObjectID valueRefIds[], GCHandleID rootIds[]) override;
    HRESULT STDMETHODCALLTYPE GetAssemblyReferences(const WCHAR* wszAssemblyPath, ICorProfilerAssemblyReferenceProvider* pAsmRefProvider) override;
    HRESULT STDMETHODCALLTYPE ModuleInMemorySymbolsUpdated(ModuleID moduleId) override;
    HRESULT STDMETHODCALLTYPE DynamicMethodJITCompilationStarted(FunctionID functionId, BOOL fIsSafeToBlock, LPCBYTE ilHeader, ULONG cbILHeader) override;
    HRESULT STDMETHODCALLTYPE DynamicMethodJITCompilationFinished(FunctionID functionId, HRESULT hrStatus, BOOL fIsSafeToBlock) override;
    HRESULT STDMETHODCALLTYPE DynamicMethodUnloaded(FunctionID functionId) override;

    /* Exception Counters */
    size_t GetNumberOfExceptionsThrown() const;
    size_t GetNumberOfFiltersExecuted() const;
    size_t GetNumberOfFinallysExecuted() const;

    /* Metadata Method Counters */
    size_t GetTotalILBytesJittedForMetadataMethods() const;
    size_t GetTotalNumberOfMetadataMethodsJitted() const;
    size_t GetCurrentNumberOfMetadataMethodsJitted() const;
    size_t GetTotalMachineCodeBytesGeneratedForMetadataMethods() const;

    /* Dynamic Method Counters */
    size_t GetTotalILBytesJittedForDynamicMethods() const;
    size_t GetTotalMachineCodeBytesGeneratedForDynamicMethods() const;
    size_t GetTotalNumberOfDynamicMethods() const;
    size_t GetCurrentNumberOfDynamicMethods() const;

    /* Precompiled Method Counters */
    size_t GetTotalCachedMethodsSearched() const;
    size_t GetTotalCachedMethodsRestored() const;

    /* Runtime Suspension Counters */
    size_t GetTotalNumberOfRuntimeSuspsensions() const;
    size_t GetTotalNumberOfRuntimeSuspensionsForGC() const;
    size_t GetTotalNumberOfRuntimeSuspensionsForGCPrep() const;

    /* Type Loader Counters */
    size_t GetCurrentNumberOfClassesLoaded() const;
    size_t GetTotalNumberOfClassesLoaded() const;

    /* Assembly Loader Counters */
    size_t GetTotalNumberOfClassLoadFailures() const;
    size_t GetCurrentNumberOfAssembliesLoaded() const;
    size_t GetTotalNumberOfAssembliesLoaded() const;
    size_t GetCurrentNumberOfModulesLoaded() const;
    size_t GetTotalNumberOfModulesLoaded() const;

    /* Thread Counters */
    size_t GetCurrentNumberOfLogicalThreads() const;

    /* GC Counters */
    size_t GetTotalNumberOfInducedGarbageCollections() const;
    size_t GetTotalNumberOfGen0Collections() const;
    size_t GetTotalNumberOfGen1Collections() const;
    size_t GetTotalNumberOfGen2Collections() const;
    size_t GetTotalNumberOfBytesAllocatedSinceLastGC() const;
    size_t GetTotalNumberOfBytesInAllHeaps() const;
    size_t GetGen0HeapSize() const;
    size_t GetGen1HeapSize() const;
    size_t GetGen2HeapSize() const;
    size_t GetGen3HeapSize() const;
    size_t GetNumberOfGCSegments() const;
    size_t GetNumberOfFrozenSegments() const;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (riid == __uuidof(ICorProfilerCallback9) ||
            riid == __uuidof(ICorProfilerCallback8) ||
            riid == __uuidof(ICorProfilerCallback7) ||
            riid == __uuidof(ICorProfilerCallback6) ||
            riid == __uuidof(ICorProfilerCallback5) ||
            riid == __uuidof(ICorProfilerCallback4) ||
            riid == __uuidof(ICorProfilerCallback3) ||
            riid == __uuidof(ICorProfilerCallback2) ||
            riid == __uuidof(ICorProfilerCallback) ||
            riid == IID_IUnknown)
        {
            *ppvObject = this;
            this->AddRef();
            return S_OK;
        }

        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return std::atomic_fetch_add(&this->refCount, 1) + 1;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const auto count = std::atomic_fetch_sub(&this->refCount, 1) - 1;

        if (count <= 0)
        {
            delete this;
        }

        return count;
    }

private:

    std::atomic<int> refCount;
    ICorProfilerInfo10* corProfilerInfo;

    /* Exception Counters */
    std::atomic<size_t> numberOfExceptionsThrown;
    std::atomic<size_t> numberOfFiltersExecuted;
    std::atomic<size_t> numberOfFinallysExecuted;

    /* Metadata Methods Jitting Counters */
    std::atomic<size_t> totalNumberOfMethodsJitted;
    std::atomic<size_t> currentNumberOfMethodsJitted;
    std::atomic<size_t> totalILBytesJitted;
    std::atomic<size_t> totalMachineCodeBytesGenerated;

    /* Dynamic Methods Jitting Counters */
    std::atomic<size_t> totalILBytesJittedForDynamicMethods;
    std::atomic<size_t> totalMachineCodeBytesGeneratedForDynamicMethods;
    std::atomic<size_t> totalNumberOfDynamicMethods;
    std::atomic<size_t> currentNumberOfDynamicMethods;

    /* Precompiled Methods Counters */
    std::atomic<size_t> totalCachedMethodsSearched;
    std::atomic<size_t> totalCachedMethodsRestored;

    /* Runtime Suspension Counters */
    std::atomic<size_t> totalNumberOfRuntimeSuspensions;
    std::atomic<size_t> totalNumberOfRuntimeSuspensionsForGC;
    std::atomic<size_t> totalNumberOfRuntimeSuspensionsForGCPrep;

    /* Type Loader Counters */
    std::atomic<size_t> currentNumberOfClassesLoaded;
    std::atomic<size_t> totalNumberOfClassesLoaded;
    std::atomic<size_t> totalNumberOfClassLoadFailures;

    /* Assembly Loader Counters */
    std::atomic<size_t> currentNumberOfAssembliesLoaded;
    std::atomic<size_t> totalNumberOfAssembliesLoaded;
    std::atomic<size_t> currentNumberOfModulesLoaded;
    std::atomic<size_t> totalNumberOfModulesLoaded;

    /* Thread Counters */
    std::atomic<size_t> currentNumberOfLogicalThreads;

    /* GC Counters */
    std::atomic<size_t> totalNumberOfInducedGarbageCollections;
    std::atomic<size_t> totalNumberOfGen0Collections;
    std::atomic<size_t> totalNumberOfGen1Collections;
    std::atomic<size_t> totalNumberOfGen2Collections;

    std::atomic<size_t> totalNumberOfAllBytesAllocatedSinceLastGC;
    std::atomic<size_t> totalNumberOfBytesInAllHeaps;

    std::atomic<size_t> gen0HeapSize;
    std::atomic<size_t> gen1HeapSize;
    std::atomic<size_t> gen2HeapSize;
    std::atomic<size_t> gen3HeapSize; // LOH

    std::atomic<size_t> numberOfGCSegments;
    std::atomic<size_t> numberOfFrozenSegments;
};

template <class TInterface>
class CComPtr
{
private:
    TInterface* pointer;

public:
    CComPtr(const CComPtr&) = delete;            // Copy constructor
    CComPtr& operator=(const CComPtr&) = delete; // Copy assignment
    CComPtr(CComPtr&&) = delete;                 // Move constructor
    CComPtr& operator=(CComPtr&&) = delete;      // Move assignment

    void* operator new(std::size_t) = delete;
    void* operator new[](std::size_t) = delete;

    void operator delete(void* ptr) = delete;
    void operator delete[](void* ptr) = delete;

    CComPtr()
    {
        this->pointer = nullptr;
    }

    ~CComPtr()
    {
        if (this->pointer)
        {
            this->pointer->Release();
            this->pointer = nullptr;
        }
    }

    operator TInterface* ()
    {
        return this->pointer;
    }

    operator TInterface* () const = delete;

    TInterface& operator*() = delete;

    TInterface& operator*() const = delete;

    TInterface** operator&()
    {
        return &this->pointer;
    }

    TInterface** operator&() const = delete;

    TInterface* operator->()
    {
        return this->pointer;
    }

    TInterface* operator->() const = delete;
};
