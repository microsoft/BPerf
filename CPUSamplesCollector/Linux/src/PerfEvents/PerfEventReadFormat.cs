// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;

    [Flags]
    internal enum PerfEventReadFormat : ulong
    {
        PERF_FORMAT_TOTAL_TIME_ENABLED = 1U << 0,
        PERF_FORMAT_TOTAL_TIME_RUNNING = 1U << 1,
        PERF_FORMAT_ID = 1U << 2,
        PERF_FORMAT_GROUP = 1U << 3,
        PERF_FORMAT_MAX = 1U << 4,
    }
}
