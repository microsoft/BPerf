// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolicInformation.ProgramDatabase
{
    using System;
    using System.Runtime.InteropServices;

    internal delegate IntPtr Allocator(IntPtr size);

    internal sealed class NativePdbReader : IDisposable
    {
        private const string LibraryPath = @"C:\Microsoft.BPerf.WindowsPdbReader.Native\x64\Debug\Microsoft.BPerf.WindowsPdbReader.Native.dll";

        private static readonly IntPtr AllocationPointer = Marshal.GetFunctionPointerForDelegate<Allocator>(Marshal.AllocHGlobal);

        private readonly IntPtr reader;

        private readonly bool signatureAndAgeValid;

        public NativePdbReader(string filename, Guid incomingGuid, uint incomingAge)
        {
            this.reader = CreatePdbSymbolReader(filename);
            this.signatureAndAgeValid = ValidateSignature(this.reader, ref incomingGuid, incomingAge);
        }

        public NativePdbReader(string filename)
            : this(filename, Guid.Empty, 0)
        {
            this.signatureAndAgeValid = true;
        }

        ~NativePdbReader()
        {
            this.ReleaseUnmanagedResources();
        }

        public void Dispose()
        {
            this.ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        public bool IsValid()
        {
            return this.signatureAndAgeValid && IsReaderValid(this.reader);
        }

        public bool FindNameForRVA(uint rva, out string name)
        {
            var retVal = FindNameForRVA(this.reader, rva, AllocationPointer, out var namePtr, out uint size);

            try
            {
                name = namePtr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringAnsi(namePtr, (int)size);
                return retVal;
            }
            finally
            {
                if (namePtr != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(namePtr);
                }
            }
        }

        public bool FindLineNumberForManagedMethod(uint methodToken, uint iloffset, out LineColumnInformation lineColumnInformation)
        {
            var retVal = FindLineNumberForManagedMethod(this.reader, methodToken, iloffset, AllocationPointer, out var namePtr, out var size, out var lineStart, out var lineEnd, out var columnStart, out var columnEnd);
            try
            {
                var name = namePtr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringAnsi(namePtr, (int)size);
                lineColumnInformation = new LineColumnInformation(name, lineStart, lineEnd, columnStart, columnEnd);
                return retVal;
            }
            finally
            {
                if (namePtr != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(namePtr);
                }
            }
        }

        public bool FindLineNumberForNativeMethod(uint rva, out LineColumnInformation lineColumnInformation)
        {
            var retVal = FindLineNumberForNativeMethod(this.reader, rva, AllocationPointer, out var namePtr, out var size, out var lineStart, out var lineEnd, out var columnStart, out var columnEnd);
            try
            {
                var name = namePtr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringAnsi(namePtr, (int)size);
                lineColumnInformation = new LineColumnInformation(name, lineStart, lineEnd, columnStart, columnEnd);
                return retVal;
            }
            finally
            {
                if (namePtr != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(namePtr);
                }
            }
        }

        public bool GetSourceLinkData(out string sourcelink)
        {
            var retVal = GetSourceLinkData(this.reader, out var dataPtr, out var size);
            try
            {
                sourcelink = dataPtr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringAnsi(dataPtr, (int)size);
                return retVal;
            }
            finally
            {
                if (dataPtr != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(dataPtr);
                }
            }
        }

        public bool GetSrcSrvData(out string srcsrv)
        {
            var retVal = GetSrcSrvData(this.reader, out var dataPtr, out var size);
            try
            {
                srcsrv = dataPtr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringAnsi(dataPtr, (int)size);
                return retVal;
            }
            finally
            {
                if (dataPtr != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(dataPtr);
                }
            }
        }

        [DllImport(LibraryPath)]
        private static extern IntPtr CreatePdbSymbolReader(string filename);

        [DllImport(LibraryPath)]
        private static extern void DeletePdbSymbolReader(IntPtr reader);

        [DllImport(LibraryPath)]
        private static extern bool IsReaderValid(IntPtr reader);

        [DllImport(LibraryPath)]
        private static extern bool ValidateSignature(IntPtr reader, ref Guid signature, uint age);

        [DllImport(LibraryPath)]
        private static extern bool FindNameForRVA(IntPtr reader, uint rva, IntPtr allocator, out IntPtr outPtr, out uint size);

        [DllImport(LibraryPath)]
        private static extern bool FindLineNumberForManagedMethod(IntPtr reader, uint methodToken, uint iloffset, IntPtr allocator, out IntPtr namePtr, out uint size, out uint lineStart, out uint lineEnd, out ushort columnStart, out ushort columnEnd);

        [DllImport(LibraryPath)]
        private static extern bool FindLineNumberForNativeMethod(IntPtr reader, uint rva, IntPtr allocator, out IntPtr namePtr, out uint size, out uint lineStart, out uint lineEnd, out ushort columnStart, out ushort columnEnd);

        [DllImport(LibraryPath)]
        private static extern bool GetSrcSrvData(IntPtr reader, out IntPtr data, out uint size);

        [DllImport(LibraryPath)]
        private static extern bool GetSourceLinkData(IntPtr reader, out IntPtr data, out uint size);

        private void ReleaseUnmanagedResources()
        {
            if (this.reader != IntPtr.Zero)
            {
                DeletePdbSymbolReader(this.reader);
            }
        }
    }
}
