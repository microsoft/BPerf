// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;

    internal readonly struct RingBuffer
    {
        public readonly IntPtr Address;

        public readonly uint Size;

        public readonly int FileDescriptor;

        public RingBuffer(IntPtr address, uint size, int fd)
        {
            this.Address = address;
            this.Size = size;
            this.FileDescriptor = fd;
        }
    }
}
