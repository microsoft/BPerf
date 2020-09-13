// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;

    // perf_event_sample_format
    [Flags]
    internal enum PerfEventSampleFormat : ulong
    {
        PERF_SAMPLE_IP = 1U << 0,
        PERF_SAMPLE_TID = 1U << 1,
        PERF_SAMPLE_TIME = 1U << 2,
        PERF_SAMPLE_ADDR = 1U << 3,
        PERF_SAMPLE_READ = 1U << 4,
        PERF_SAMPLE_CALLCHAIN = 1U << 5,
        PERF_SAMPLE_ID = 1U << 6,
        PERF_SAMPLE_CPU = 1U << 7,
        PERF_SAMPLE_PERIOD = 1U << 8,
        PERF_SAMPLE_STREAM_ID = 1U << 9,
        PERF_SAMPLE_RAW = 1U << 10,
        PERF_SAMPLE_BRANCH_STACK = 1U << 11,
        PERF_SAMPLE_REGS_USER = 1U << 12,
        PERF_SAMPLE_STACK_USER = 1U << 13,
        PERF_SAMPLE_WEIGHT = 1U << 14,
        PERF_SAMPLE_DATA_SRC = 1U << 15,
        PERF_SAMPLE_IDENTIFIER = 1U << 16,
        PERF_SAMPLE_TRANSACTION = 1U << 17,
        PERF_SAMPLE_REGS_INTR = 1U << 18,
        PERF_SAMPLE_PHYS_ADDR = 1U << 19,
        PERF_SAMPLE_AUX = 1U << 20, // 5.5
        PERF_SAMPLE_CGROUP = 1U << 21, // 5.7
        PERF_SAMPLE_MAX = 1U << 22, /* non-ABI */
    }
}
