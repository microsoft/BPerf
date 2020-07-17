// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;
    using System.Runtime.InteropServices;
    using static NativeMethods;

    [StructLayout(LayoutKind.Sequential)]
    internal ref struct ReadOnlyLinuxFile
    {
        private readonly IntPtr addr;

        private readonly int size;

        private readonly int fd;

        public ReadOnlyLinuxFile(in byte filename)
        {
            const int O_RDONLY = 0;
            this.fd = OpenFileDescriptor(in filename, O_RDONLY);

            this.size = GetFileSize(in filename);

            const int MAP_PRIVATE = 2;
            const int PROT_READ = 1;
            const int PROT_WRITE = 2;

            this.addr = MemoryMap(IntPtr.Zero, this.size, PROT_READ | PROT_WRITE, MAP_PRIVATE, this.fd, 0);
        }

        public unsafe Span<byte> Contents => new Span<byte>((void*)this.addr, this.size);

        public void Dispose()
        {
            MemoryUnmap(this.addr, this.size);
            CloseFileDescriptor(this.fd);
        }
    }
}
