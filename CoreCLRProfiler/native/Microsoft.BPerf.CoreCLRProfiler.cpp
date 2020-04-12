// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "BPerfProfilerCallback.h"

BPerfProfilerCallback* ProfilerInstance;

/* Exception Counters */
extern "C" size_t GetNumberOfExceptionsThrown()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetNumberOfExceptionsThrown();
}

extern "C" size_t GetNumberOfFiltersExecuted()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetNumberOfFiltersExecuted();
}

extern "C" size_t GetNumberOfFinallysExecuted()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetNumberOfFinallysExecuted();
}

/* Metadata Method Counters */
extern "C" size_t GetTotalILBytesJittedForMetadataMethods()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalILBytesJittedForMetadataMethods();
}

extern "C" size_t GetCurrentNumberOfMetadataMethodsJitted()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetCurrentNumberOfMetadataMethodsJitted();
}

extern "C" size_t GetTotalNumberOfMetadataMethodsJitted()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfMetadataMethodsJitted();
}

extern "C" size_t GetTotalMachineCodeBytesGeneratedForMetadataMethods()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalMachineCodeBytesGeneratedForMetadataMethods();
}

/* Dynamic Method Counters */
extern "C" size_t GetTotalILBytesJittedForDynamicMethods()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalILBytesJittedForDynamicMethods();
}

extern "C" size_t GetTotalMachineCodeBytesGeneratedForDynamicMethods()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalMachineCodeBytesGeneratedForDynamicMethods();
}

extern "C" size_t GetTotalNumberOfDynamicMethods()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfDynamicMethods();
}

extern "C" size_t GetCurrentNumberOfDynamicMethods()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetCurrentNumberOfDynamicMethods();
}

/* Precompiled Method Counters */
extern "C" size_t GetTotalCachedMethodsSearched()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalCachedMethodsSearched();
}

extern "C" size_t GetTotalCachedMethodsRestored()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalCachedMethodsRestored();
}

extern "C" size_t GetTotalCachedMethodsMachineCodeBytesRestored()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalCachedMethodsMachineCodeBytesRestored();
}

/* Runtime Suspension Counters */
extern "C" size_t GetTotalNumberOfRuntimeSuspsensions()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfRuntimeSuspsensions();
}

extern "C" size_t GetTotalNumberOfRuntimeSuspensionsForGC()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfRuntimeSuspensionsForGC();
}

extern "C" size_t GetTotalNumberOfRuntimeSuspensionsForGCPrep()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfRuntimeSuspensionsForGCPrep();
}

/* Type Loader Counters */
extern "C" size_t GetCurrentNumberOfClassesLoaded()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetCurrentNumberOfClassesLoaded();
}

extern "C" size_t GetTotalNumberOfClassesLoaded()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfClassesLoaded();
}

extern "C" size_t GetTotalNumberOfClassLoadFailures()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfClassLoadFailures();
}

/* Assembly Loader Counters */
extern "C" size_t GetCurrentNumberOfAssembliesLoaded()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetCurrentNumberOfAssembliesLoaded();
}

extern "C" size_t GetTotalNumberOfAssembliesLoaded()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfAssembliesLoaded();
}

extern "C" size_t GetCurrentNumberOfModulesLoaded()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetCurrentNumberOfModulesLoaded();
}

extern "C" size_t GetTotalNumberOfModulesLoaded()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfModulesLoaded();
}

/* Thread Counters */
extern "C" size_t GetCurrentNumberOfLogicalThreads()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetCurrentNumberOfLogicalThreads();
}

/* GC Counters */
extern "C" size_t GetTotalNumberOfInducedGarbageCollections()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfInducedGarbageCollections();
}

extern "C" size_t GetTotalNumberOfGen0Collections()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfGen0Collections();
}

extern "C" size_t GetTotalNumberOfGen1Collections()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfGen1Collections();
}

extern "C" size_t GetTotalNumberOfGen2Collections()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfGen2Collections();
}

extern "C" size_t GetTotalNumberOfBytesInAllHeaps()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfBytesInAllHeaps();
}

extern "C" size_t GetGen0HeapSize()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetGen0HeapSize();
}

extern "C" size_t GetGen1HeapSize()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetGen1HeapSize();
}

extern "C" size_t GetGen2HeapSize()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetGen2HeapSize();
}

extern "C" size_t GetGen3HeapSize()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetGen3HeapSize();
}

extern "C" size_t GetFrozenHeapSize()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetFrozenHeapSize();
}

extern "C" size_t GetNumberOfGCSegments()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetNumberOfGCSegments();
}

extern "C" size_t GetNumberOfFrozenSegments()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetNumberOfFrozenSegments();
}

extern "C" size_t GetTotalNumberOfBytesAllocatedSinceLastGC()
{
    if (ProfilerInstance == nullptr)
    {
        return 0;
    }

    return ProfilerInstance->GetTotalNumberOfBytesAllocatedSinceLastGC();
}

extern "C" bool EnableObjectAllocationMonitoring()
{
    if (ProfilerInstance == nullptr)
    {
        return false;
    }

    return ProfilerInstance->EnableObjectAllocationMonitoring();
}

extern "C" bool DisableObjectAllocationMonitoring()
{
    if (ProfilerInstance == nullptr)
    {
        return false;
    }

    return ProfilerInstance->DisableObjectAllocationMonitoring();
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
