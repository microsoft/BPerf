// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;
    using static NativeMethods;

    internal readonly ref struct RingBuffers
    {
        private readonly ReadOnlySpan<RingBuffer> data;

        public RingBuffers(Span<RingBuffer> space)
        {
            var attr = new PerfEventAttr
            {
                SampleType = PerfEventSampleFormat.PERF_SAMPLE_RAW,
                Type = PerfEventType.PERF_TYPE_SOFTWARE,
                Config = PerfTypeSpecificConfig.PERF_COUNT_SW_BPF_OUTPUT,
                SamplePeriod = 1,
                WakeupEvents = 1000,
            };

            for (var i = 0; i < space.Length; ++i)
            {
                var pmufd = PerfEventOpen(in attr, i);
                if (pmufd == -1)
                {
                    this.data = default;
                    return;
                }

                const int ReadWrite = 3;
                const int Shared = 1;

                var length = Program.RingBufferSize + GetMemoryPageSize();

                IntPtr m = MemoryMap(IntPtr.Zero, length, ReadWrite, Shared, pmufd, 0);
                if (m == new IntPtr(-1))
                {
                    this.data = default;
                    return;
                }

                space[i] = new RingBuffer(m, length, pmufd);
            }

            this.data = space;
        }

        public int Length => this.data.Length;

        public ref readonly RingBuffer this[int index]
        {
            get
            {
                return ref this.data[index];
            }
        }

        public void Dispose()
        {
            for (var i = 0; i < this.data.Length; ++i)
            {
                ref readonly RingBuffer r = ref this.data[i];
                MemoryUnmap(r.Address, r.Size);
                CloseFileDescriptor(r.FileDescriptor);
            }
        }
    }
}
