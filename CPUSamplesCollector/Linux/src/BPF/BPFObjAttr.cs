// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal readonly struct BPFObjAttr
    {
        private readonly ulong pathName;

        private readonly int fileDescriptor;

        private readonly int fileFlags;

        public unsafe BPFObjAttr(void* pathName, int fd, int flags)
        {
            this.pathName = (ulong)pathName;
            this.fileDescriptor = fd;
            this.fileFlags = flags;
        }
    }
}
