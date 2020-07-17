// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;
    using static NativeMethods;

    internal readonly ref struct PerfEventDescriptors
    {
        private readonly ReadOnlySpan<int> data;

        public PerfEventDescriptors(in PerfEventAttr attr, int bpfprogfd, Span<int> space)
        {
            this.data = space;

            for (var i = 0; i < space.Length; ++i)
            {
                ref var perffd = ref space[i];

                perffd = PerfEventOpen(in attr, i);

                PerfEventAssociateBPFProgram(perffd, bpfprogfd);
                PerfEventEnable(perffd);
            }
        }

        public void Dispose()
        {
            for (var i = 0; i < this.data.Length; ++i)
            {
                CloseFileDescriptor(this.data[i]);
            }
        }
    }
}
