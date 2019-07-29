// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*
 *
 *Frozen Objects -- implement method thingie
.NET Counters
PerfView/BPerf
large/huge pages
make serialization/deserialization itnernal call
regex compile to assembly
implement multi process serp using ipc mechanism and demo with Kestrel and asp.net core httpcontext
utf-8 razor, calli + integer based rms/razor stuff
DI code generation
bond parser
protobuf code-gen
string deduping
how can frozen segments be in range?
 *
 * number of gc segments
number of r2r assemblies
number of assembiles loaded from disk vs. dynamic assemblies
number of modules loaded from disk. vs dynamic
number of types loaded ... dynamic vs not
number of generic types loaded ... 
number of value types vs number of reference types
number of type with finalizers

number of interface dispatches generated or called?
histogram of methods that are huge/small/tiny/etc
number of types loaded as parameters for Dictionary<T, K>
  ... more formally, types whose gethashcode and equals is called or used
types that are involved in locks / objects involved in locks
areas of code where reference equality is performed
types that override gethashcode
methods that use gethashcode and are not override GetHashCode or implementing it as IEquatable
Tree browser of how to do perf analysis starting at assembly, finding all types loaded, then which methods, etc.
Propose profiler apis for getting locals info
number of methods with try/catch in them
number of methods with high local variables
stack walk for loh allocations
GetLOHObjectSizeThreshold
GetDynamicFunctionInfo
lock GCStart/GCEnd so we can expose an API to get all segments

 */

#pragma once

#include "profiler_pal.h"

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#include <cstddef>
#include <atomic>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include "cor.h"
#include "corprof.h"
#include "ETWHeaders.h"

#undef IfFailRet
#define IfFailRet(EXPR) do { HRESULT hr = (EXPR); if(FAILED(hr)) { return (hr); } } while (0)

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
    size_t GetTotalNumberOfGen3Collections() const;
    size_t GetTotalNumberOfBytesAllocatedSinceLastGC() const;
    size_t GetTotalNumberOfPromotedBytes() const;
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

    ModuleID eventSourceTypeModuleId;
    mdMethodDef writeEventCoreMethodDef;
    mdMethodDef writeEventWithRelatedActivityIdCoreMethodDef;
    mdFieldDef mGuidFieldDef;

    std::mutex methodTableFileMapLock;
    std::mutex classTableFileMapLock;
    std::mutex moduleTableFileMapLock;
    std::mutex assemblyTableFileMapLock;

    std::unordered_map<ThreadID, std::ofstream> methodTableFileMap;
    std::unordered_map<ThreadID, std::ofstream> classTableFileMap;
    std::unordered_map<ThreadID, std::ofstream> moduleTableFileMap;
    std::unordered_map<ThreadID, std::ofstream> assemblyTableFileMap;

    std::ofstream methodTableFile;
    std::ofstream classTableFile;
    std::ofstream moduleTableFile;
    std::ofstream assemblyTableFile;

    std::atomic<size_t> methodTableFileOffset;
    std::atomic<size_t> classTableFileOffset;
    std::atomic<size_t> moduleTableFileOffset;
    std::atomic<size_t> assemblyTableFileOffset;
    std::atomic<size_t> dataFileOffset;

    /* Exception Counters */
    std::atomic<SIZE_T> numberOfExceptionsThrown;
    std::atomic<SIZE_T> numberOfFiltersExecuted;
    std::atomic<SIZE_T> numberOfFinallysExecuted;

    /* Metadata Methods Jitting Counters */
    std::atomic<SIZE_T> totalILBytesJitted;
    std::atomic<SIZE_T> totalNumberOfMethodsJitted;
    std::atomic<SIZE_T> currentNumberOfMethodsJitted;
    std::atomic<SIZE_T> totalMachineCodeBytesGenerated;

    /* Dynamic Methods Jitting Counters */
    std::atomic<SIZE_T> totalILBytesJittedForDynamicMethods;
    std::atomic<SIZE_T> totalNumberOfDynamicMethods;
    std::atomic<SIZE_T> currentNumberOfDynamicMethods;

    /* Precompiled Methods Counters */
    std::atomic<SIZE_T> totalCachedMethodsSearched;
    std::atomic<SIZE_T> totalCachedMethodsRestored;

    /* Runtime Suspension Counters */
    std::atomic<SIZE_T> totalNumberOfRuntimeSuspensions;
    std::atomic<SIZE_T> totalNumberOfRuntimeSuspensionsForGC;
    std::atomic<SIZE_T> totalNumberOfRuntimeSuspensionsForGCPrep;

    /* Type Loader Counters */
    std::atomic<SIZE_T> currentNumberOfClassesLoaded;
    std::atomic<SIZE_T> totalNumberOfClassesLoaded;
    std::atomic<SIZE_T> totalNumberOfClassLoadFailures;

    /* Assembly Loader Counters */
    std::atomic<SIZE_T> currentNumberOfAssembliesLoaded;
    std::atomic<SIZE_T> totalNumberOfAssembliesLoaded;
    std::atomic<SIZE_T> currentNumberOfModulesLoaded;
    std::atomic<SIZE_T> totalNumberOfModulesLoaded;

    /* Thread Counters */
    std::atomic<SIZE_T> currentNumberOfLogicalThreads;

    /* GC Counters */
    std::atomic<SIZE_T> totalNumberOfInducedGarbageCollections;
    std::atomic<SIZE_T> totalNumberOfGen0Collections;
    std::atomic<SIZE_T> totalNumberOfGen1Collections;
    std::atomic<SIZE_T> totalNumberOfGen2Collections;
    std::atomic<SIZE_T> totalNumberOfGen3Collections; // LOH

    std::atomic<SIZE_T> totalNumberOfAllBytesAllocatedSinceLastGC;
    std::atomic<SIZE_T> totalNumberOfBytesInAllHeaps;

    std::atomic<SIZE_T> gen0HeapSize;
    std::atomic<SIZE_T> gen1HeapSize;
    std::atomic<SIZE_T> gen2HeapSize;
    std::atomic<SIZE_T> gen3HeapSize; // LOH

    std::atomic<SIZE_T> numberOfGCSegments;
    std::atomic<SIZE_T> numberOfFrozenSegments;

    std::atomic<SIZE_T> totalHeapSizeAtGCStart;
    std::atomic<SIZE_T> totalPromotedBytes;
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

#define CLASS_TABLE_ROW_SIZE 16
#define CLASS_INFO_DATA_SIZE_BASE (sizeof(ModuleID) + sizeof(ClassID))
