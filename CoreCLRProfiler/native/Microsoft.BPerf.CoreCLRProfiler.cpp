// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "BPerfProfilerCallback.h"

BPerfProfilerCallback* ProfilerInstance;

/* Exception Counters */
extern "C" size_t GetNumberOfExceptionsThrown()
{
    return ProfilerInstance->GetNumberOfExceptionsThrown();
}

extern "C" size_t GetNumberOfFiltersExecuted()
{
    return ProfilerInstance->GetNumberOfFiltersExecuted();
}

extern "C" size_t GetNumberOfFinallysExecuted()
{
    return ProfilerInstance->GetNumberOfFinallysExecuted();
}

/* Metadata Method Counters */
extern "C" size_t GetTotalILBytesJittedForMetadataMethods()
{
    return ProfilerInstance->GetTotalILBytesJittedForMetadataMethods();
}

extern "C" size_t GetCurrentNumberOfMetadataMethodsJitted()
{
    return ProfilerInstance->GetCurrentNumberOfMetadataMethodsJitted();
}

extern "C" size_t GetTotalNumberOfMetadataMethodsJitted()
{
    return ProfilerInstance->GetTotalNumberOfMetadataMethodsJitted();
}

extern "C" size_t GetTotalMachineCodeBytesGeneratedForMetadataMethods()
{
    return ProfilerInstance->GetTotalMachineCodeBytesGeneratedForMetadataMethods();
}

/* Dynamic Method Counters */
extern "C" size_t GetTotalILBytesJittedForDynamicMethods()
{
    return ProfilerInstance->GetTotalILBytesJittedForDynamicMethods();
}

extern "C" size_t GetTotalMachineCodeBytesGeneratedForDynamicMethods()
{
    return ProfilerInstance->GetTotalMachineCodeBytesGeneratedForDynamicMethods();
}

extern "C" size_t GetTotalNumberOfDynamicMethods()
{
    return ProfilerInstance->GetTotalNumberOfDynamicMethods();
}

extern "C" size_t GetCurrentNumberOfDynamicMethods()
{
    return ProfilerInstance->GetCurrentNumberOfDynamicMethods();
}

/* Precompiled Method Counters */
extern "C" size_t GetTotalCachedMethodsSearched()
{
    return ProfilerInstance->GetTotalCachedMethodsSearched();
}

extern "C" size_t GetTotalCachedMethodsRestored()
{
    return ProfilerInstance->GetTotalCachedMethodsRestored();
}

extern "C" size_t GetTotalCachedMethodsMachineCodeBytesRestored()
{
    return ProfilerInstance->GetTotalCachedMethodsMachineCodeBytesRestored();
}

/* Runtime Suspension Counters */
extern "C" size_t GetTotalNumberOfRuntimeSuspsensions()
{
    return ProfilerInstance->GetTotalNumberOfRuntimeSuspsensions();
}

extern "C" size_t GetTotalNumberOfRuntimeSuspensionsForGC()
{
    return ProfilerInstance->GetTotalNumberOfRuntimeSuspensionsForGC();
}

extern "C" size_t GetTotalNumberOfRuntimeSuspensionsForGCPrep()
{
    return ProfilerInstance->GetTotalNumberOfRuntimeSuspensionsForGCPrep();
}

/* Type Loader Counters */
extern "C" size_t GetCurrentNumberOfClassesLoaded()
{
    return ProfilerInstance->GetCurrentNumberOfClassesLoaded();
}

extern "C" size_t GetTotalNumberOfClassesLoaded()
{
    return ProfilerInstance->GetTotalNumberOfClassesLoaded();
}

extern "C" size_t GetTotalNumberOfClassLoadFailures()
{
    return ProfilerInstance->GetTotalNumberOfClassLoadFailures();
}

/* Assembly Loader Counters */
extern "C" size_t GetCurrentNumberOfAssembliesLoaded()
{
    return ProfilerInstance->GetCurrentNumberOfAssembliesLoaded();
}

extern "C" size_t GetTotalNumberOfAssembliesLoaded()
{
    return ProfilerInstance->GetTotalNumberOfAssembliesLoaded();
}

extern "C" size_t GetCurrentNumberOfModulesLoaded()
{
    return ProfilerInstance->GetCurrentNumberOfModulesLoaded();
}

extern "C" size_t GetTotalNumberOfModulesLoaded()
{
    return ProfilerInstance->GetTotalNumberOfModulesLoaded();
}

/* Thread Counters */
extern "C" size_t GetCurrentNumberOfLogicalThreads()
{
    return ProfilerInstance->GetCurrentNumberOfLogicalThreads();
}

/* GC Counters */
extern "C" size_t GetTotalNumberOfInducedGarbageCollections()
{
    return ProfilerInstance->GetTotalNumberOfInducedGarbageCollections();
}

extern "C" size_t GetTotalNumberOfGen0Collections()
{
    return ProfilerInstance->GetTotalNumberOfGen0Collections();
}

extern "C" size_t GetTotalNumberOfGen1Collections()
{
    return ProfilerInstance->GetTotalNumberOfGen1Collections();
}

extern "C" size_t GetTotalNumberOfGen2Collections()
{
    return ProfilerInstance->GetTotalNumberOfGen2Collections();
}

extern "C" size_t GetTotalNumberOfBytesInAllHeaps()
{
    return ProfilerInstance->GetTotalNumberOfBytesInAllHeaps();
}

extern "C" size_t GetGen0HeapSize()
{
    return ProfilerInstance->GetGen0HeapSize();
}

extern "C" size_t GetGen1HeapSize()
{
    return ProfilerInstance->GetGen1HeapSize();
}

extern "C" size_t GetGen2HeapSize()
{
    return ProfilerInstance->GetGen2HeapSize();
}

extern "C" size_t GetGen3HeapSize()
{
    return ProfilerInstance->GetGen3HeapSize();
}

extern "C" size_t GetFrozenHeapSize()
{
    return ProfilerInstance->GetFrozenHeapSize();
}

extern "C" size_t GetNumberOfGCSegments()
{
    return ProfilerInstance->GetNumberOfGCSegments();
}

extern "C" size_t GetNumberOfFrozenSegments()
{
    return ProfilerInstance->GetNumberOfFrozenSegments();
}

extern "C" size_t GetTotalNumberOfBytesAllocatedSinceLastGC()
{
    return ProfilerInstance->GetTotalNumberOfBytesAllocatedSinceLastGC();
}

class ClassFactory : public IClassFactory
{
private:
    std::atomic<int> refCount;

public:
    ClassFactory();
    virtual ~ClassFactory();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) override;
    HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) override;
};

ClassFactory::ClassFactory() : refCount(0)
{
}

ClassFactory::~ClassFactory() = default;

HRESULT STDMETHODCALLTYPE ClassFactory::QueryInterface(REFIID riid, void** ppvObject)
{
    if (riid == IID_IUnknown || riid == IID_IClassFactory)
    {
        *ppvObject = this;
        this->AddRef();
        return S_OK;
    }

    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE ClassFactory::AddRef()
{
    return std::atomic_fetch_add(&this->refCount, 1) + 1;
}

ULONG STDMETHODCALLTYPE ClassFactory::Release()
{
    const auto count = std::atomic_fetch_sub(&this->refCount, 1) - 1;
    if (count <= 0)
    {
        delete this;
    }

    return count;
}

HRESULT STDMETHODCALLTYPE ClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
{
    if (pUnkOuter != nullptr)
    {
        *ppvObject = nullptr;
        return CLASS_E_NOAGGREGATION;
    }

    ProfilerInstance = new BPerfProfilerCallback();
    if (ProfilerInstance == nullptr)
    {
        return E_FAIL;
    }

    return ProfilerInstance->QueryInterface(riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE ClassFactory::LockServer(BOOL fLock)
{
    return S_OK;
}

const IID IID_NULL = { 0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };

const IID IID_IUnknown = { 0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };

const IID IID_IClassFactory = { 0x00000001, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };

BOOL STDMETHODCALLTYPE DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

extern "C" HRESULT STDMETHODCALLTYPE DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    // {D46E5565-D624-4C4D-89CC-7A82887D3626}
    const GUID CLSID_CorProfiler = { 0xD46E5565, 0xD624, 0x4C4D, {0x89, 0xCC, 0x7A, 0x82, 0x88, 0x7D, 0x36, 0x26} };

    if (ppv == nullptr || rclsid != CLSID_CorProfiler)
    {
        return E_FAIL;
    }

    auto factory = new ClassFactory;
    if (factory == nullptr)
    {
        return E_FAIL;
    }

    return factory->QueryInterface(riid, ppv);
}

extern "C" HRESULT STDMETHODCALLTYPE DllCanUnloadNow()
{
    return S_OK;
}
