namespace Microsoft.BPerf
{
    using System;
    using System.Runtime.InteropServices;

    public static class ProfilerInterop
    {
        private const string dllName = "Microsoft.BPerf.CoreCLRProfiler.dll";

        [DllImport(dllName)]
        public static extern IntPtr GetNumberOfExceptionsThrown();

        [DllImport(dllName)]
        public static extern IntPtr GetNumberOfFiltersExecuted();

        [DllImport(dllName)]
        public static extern IntPtr GetNumberOfFinallysExecuted();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalILBytesJittedForMetadataMethods();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfMetadataMethodsJitted();

        [DllImport(dllName)]
        public static extern IntPtr GetCurrentNumberOfMetadataMethodsJitted();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalMachineCodeBytesGeneratedForMetadataMethods();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalILBytesJittedForDynamicMethods();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalMachineCodeBytesGeneratedForDynamicMethods();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfDynamicMethods();

        [DllImport(dllName)]
        public static extern IntPtr GetCurrentNumberOfDynamicMethods();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalCachedMethodsSearched();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalCachedMethodsRestored();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalCachedMethodsMachineCodeBytesRestored();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfRuntimeSuspsensions();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfRuntimeSuspensionsForGC();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfRuntimeSuspensionsForGCPrep();

        [DllImport(dllName)]
        public static extern IntPtr GetCurrentNumberOfClassesLoaded();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfClassesLoaded();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfClassLoadFailures();

        [DllImport(dllName)]
        public static extern IntPtr GetCurrentNumberOfAssembliesLoaded();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfAssembliesLoaded();

        [DllImport(dllName)]
        public static extern IntPtr GetCurrentNumberOfModulesLoaded();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfModulesLoaded();

        [DllImport(dllName)]
        public static extern IntPtr GetCurrentNumberOfLogicalThreads();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfInducedGarbageCollections();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfGen0Collections();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfGen1Collections();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfGen2Collections();

        [DllImport(dllName)]
        public static extern IntPtr GetTotalNumberOfBytesInAllHeaps();

        [DllImport(dllName)]
        public static extern IntPtr GetGen0HeapSize();

        [DllImport(dllName)]
        public static extern IntPtr GetGen1HeapSize();

        [DllImport(dllName)]
        public static extern IntPtr GetGen2HeapSize();

        [DllImport(dllName)]
        public static extern IntPtr GetGen3HeapSize();

        [DllImport(dllName)]
        public static extern IntPtr GetFrozenHeapSize();

        [DllImport(dllName)]
        public static extern IntPtr GetNumberOfGCSegments();

        [DllImport(dllName)]
        public static extern IntPtr GetNumberOfFrozenSegments();
    }
}