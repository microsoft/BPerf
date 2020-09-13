// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    // perf_event_type
    internal enum PerfEventType : uint
    {
        PERF_RECORD_MMAP = 1,
        PERF_RECORD_LOST = 2,
        PERF_RECORD_COMM = 3,
        PERF_RECORD_EXIT = 4,
        PERF_RECORD_THROTTLE = 5,
        PERF_RECORD_UNTHROTTLE = 6,
        PERF_RECORD_FORK = 7,
        PERF_RECORD_READ = 8,
        PERF_RECORD_SAMPLE = 9,
        PERF_RECORD_MMAP2 = 10,
        PERF_RECORD_AUX = 11,
        PERF_RECORD_ITRACE_START = 12,
        PERF_RECORD_LOST_SAMPLES = 13,
        PERF_RECORD_SWITCH = 14,
        PERF_RECORD_SWITCH_CPU_WIDE = 15,
        PERF_RECORD_NAMESPACES = 16,
        PERF_RECORD_KSYMBOL = 17, // 5.1
        PERF_RECORD_BPF_EVENT = 18, // 5.1
        PERF_RECORD_CGROUP = 19, // 5.7
        PERF_RECORD_TEXT_POKE = 20, // 5.9
        PERF_RECORD_MAX,
    }
}
