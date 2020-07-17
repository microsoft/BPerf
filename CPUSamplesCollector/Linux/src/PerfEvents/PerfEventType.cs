// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    internal enum PerfEventType : ulong
    {
        PERF_TYPE_HARDWARE = 0,
        PERF_TYPE_SOFTWARE = 1,
        PERF_TYPE_TRACEPOINT = 2,
        PERF_TYPE_HW_CACHE = 3,
        PERF_TYPE_RAW = 4,
        PERF_TYPE_BREAKPOINT = 5,
    }
}
