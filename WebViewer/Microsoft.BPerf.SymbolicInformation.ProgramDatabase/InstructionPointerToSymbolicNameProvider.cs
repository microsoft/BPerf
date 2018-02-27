// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolicInformation.ProgramDatabase
{
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.BPerf.ModuleInformation.Abstractions;
    using Microsoft.BPerf.PdbSymbolReader.Interfaces;
    using Microsoft.BPerf.SymbolicInformation.Interfaces;

    public sealed class InstructionPointerToSymbolicNameProvider : IInstructionPointerToSymbolicNameProvider
    {
        private readonly Dictionary<ulong, ManagedMethodILToNativeMapping> methodILNativeMap;

        private readonly Dictionary<uint, List<ManagedMethodInfo>> methodLoadMap;

        private readonly Dictionary<uint, List<ImageInfo>> imageLoadMap;

        private readonly Dictionary<ProcessModuleId, ManagedModuleInfo> managedModuleInfoMap;

        private readonly Dictionary<ProcessImageId, DbgId> imageToDebugInfoMap;

        private readonly ITracePdbSymbolReaderProvider tracePdbSymbolReaderProvider;

        public InstructionPointerToSymbolicNameProvider(IImageInformationProvider symbolicInformationProvider, ITracePdbSymbolReaderProvider tracePdbSymbolReaderProvider)
        {
            this.methodLoadMap = symbolicInformationProvider.MethodLoadMap;
            this.methodILNativeMap = symbolicInformationProvider.MethodILNativeMap;
            this.imageLoadMap = symbolicInformationProvider.ImageLoadMap;
            this.managedModuleInfoMap = symbolicInformationProvider.ManagedModuleInfoMap;
            this.imageToDebugInfoMap = symbolicInformationProvider.ImageToDebugInfoMap;
            this.tracePdbSymbolReaderProvider = tracePdbSymbolReaderProvider;
        }

        public string GetSymbolicName(uint pid, ulong eip)
        {
            if (this.methodLoadMap.TryGetValue(pid, out var methodList))
            {
                var result = RangeBinarySearch(methodList, eip);
                if (result >= 0)
                {
                    var managedMethodInfo = methodList[result];

                    string managedModuleName = "ManagedModule!";

                    if (this.managedModuleInfoMap.TryGetValue(new ProcessModuleId(pid, managedMethodInfo.ModuleID), out var moduleInfo))
                    {
                        managedModuleName = moduleInfo.ModuleNamePrefix;
                    }

                    return managedModuleName + managedMethodInfo.MethodName;
                }
            }

            var retVal = this.NativeFrameToString(pid, eip);

            if (retVal != null)
            {
                return retVal;
            }

            // this is done because instruction pointers not resolved
            // maybe in the common idle process.
            retVal = this.NativeFrameToString(0, eip);
            if (retVal != null)
            {
                return retVal;
            }

            // most likely jitted code
            return "?!0x" + eip.ToString("x");
        }

        public async ValueTask<SourceLocation> GetSourceLocation(uint pid, ulong eip)
        {
            if (this.methodLoadMap.TryGetValue(pid, out var methodList))
            {
                var result = RangeBinarySearch(methodList, eip);
                if (result >= 0)
                {
                    var managedMethod = methodList[result];

                    if (this.managedModuleInfoMap.TryGetValue(new ProcessModuleId(pid, managedMethod.ModuleID), out var moduleInfo))
                    {
                        var managedPdbSignature = moduleInfo.ManagedPdbSignature;
                        var managedPdbAge = moduleInfo.ManagedPdbAge;
                        var managedPdbPath = moduleInfo.ManagedPdbPath;
                        var moduleILPath = moduleInfo.ModuleILPath;

                        var pdbSymbolReader = await this.tracePdbSymbolReaderProvider.GetSymbolReader(moduleILPath, managedPdbSignature, managedPdbAge, managedPdbPath);

                        int iloffset = 0;
                        if (this.methodILNativeMap.TryGetValue(managedMethod.MethodID, out var mapping))
                        {
                            int nativeOffset = (int)(eip - managedMethod.Begin); // yes, we're limited to 4GB
                            iloffset = NativeToILOffset(mapping.ILOffsets, mapping.NativeOffsets, nativeOffset);
                        }

                        return pdbSymbolReader?.FindSourceLocation(managedMethod.MethodToken, iloffset);
                    }
                }
            }

            if (this.imageLoadMap.TryGetValue(pid, out var imageList))
            {
                var result = RangeBinarySearch(imageList, eip);
                if (result >= 0)
                {
                    var nativeImageInfo = imageList[result];
                    return this.GetPdbSymbolReader(pid, nativeImageInfo)?.FindSourceLocation((uint)(eip - nativeImageInfo.Begin));
                }
            }

            return null;
        }

        private static int NativeToILOffset(int[] iloffsets, int[] nativeoffsets, int nativeLookupOffset)
        {
            var count = iloffsets.Length;
            var map = new Dictionary<int, int>(count);

            for (int i = 0; i < count; ++i)
            {
                map.Add(i, iloffsets[i]);
            }

            return map.TryGetValue(LinearInterpolationSearch(nativeoffsets, nativeLookupOffset), out int retVal) ? retVal : -1;
        }

        private static int LinearInterpolationSearch(int[] arr, int key)
        {
            int low = 0;
            int high = arr.Length - 1;

            while (arr[high] != arr[low] && key >= arr[low] && key <= arr[high])
            {
                var mid = low + ((key - arr[low]) * (high - low) / (arr[high] - arr[low]));

                if (arr[mid] < key)
                {
                    low = mid + 1;
                }
                else if (key < arr[mid])
                {
                    high = mid - 1;
                }
                else
                {
                    return mid;
                }
            }

            if (key == arr[low])
            {
                return low;
            }

            return -1;
        }

        private static int RangeBinarySearch<T>(List<T> sortedArray, ulong frame)
            where T : SearchableInfo
        {
            return RangeBinarySearch(sortedArray, 0, sortedArray.Count - 1, frame);
        }

        private static int RangeBinarySearch<T>(List<T> sortedArray, int first, int last, ulong frame)
            where T : SearchableInfo
        {
            if (first <= last)
            {
                int mid = (first + last) / 2;

                // if the frame is imageBase or within its size
                if (frame >= sortedArray[mid].Begin && frame <= sortedArray[mid].End)
                {
                    return mid;
                }

                if (frame < sortedArray[mid].Begin)
                {
                    return RangeBinarySearch(sortedArray, first, mid - 1, frame);
                }

                return RangeBinarySearch(sortedArray, mid + 1, last, frame);
            }

            return -1;
        }

        private string NativeFrameToString(uint pid, ulong eip)
        {
            if (this.imageLoadMap.TryGetValue(pid, out var imageList))
            {
                var result = RangeBinarySearch(imageList, eip);
                if (result >= 0)
                {
                    var nativeImageInfo = imageList[result];
                    var pdbSymbolReader = this.GetPdbSymbolReader(pid, nativeImageInfo);

                    string retVal;
                    if (pdbSymbolReader != null)
                    {
                        pdbSymbolReader.AddInstructionPointersForProcess(pid, nativeImageInfo.Begin, nativeImageInfo.InstructionPointers);
                        retVal = nativeImageInfo.ImageName + "!" + pdbSymbolReader.FindNameForRVA((uint)(eip - nativeImageInfo.Begin));
                    }
                    else
                    {
                        retVal = nativeImageInfo.ImageName + "!0x" + eip.ToString("x");
                    }

                    return retVal;
                }
            }

            return null;
        }

        private IPdbSymbolReader GetPdbSymbolReader(uint pid, ImageInfo nativeImageInfo)
        {
            if (this.imageToDebugInfoMap.TryGetValue(new ProcessImageId(pid, nativeImageInfo.Begin), out var dbgId))
            {
                return this.tracePdbSymbolReaderProvider.GetSymbolReader(dbgId.Signature, dbgId.Age, dbgId.Filename);
            }

            return null;
        }
    }
}
