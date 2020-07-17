// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using static NativeMethods;

    internal readonly ref struct LinuxFileDescriptor
    {
        public LinuxFileDescriptor(int fd)
        {
            this.FileDescriptor = fd;
        }

        public int FileDescriptor { get; }

        public void Dispose()
        {
            CloseFileDescriptor(this.FileDescriptor);
        }
    }
}
