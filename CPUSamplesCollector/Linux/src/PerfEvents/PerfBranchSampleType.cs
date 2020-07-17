// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;

    [Flags]
    internal enum PerfBranchSampleType : ulong
    {
        PERF_SAMPLE_BRANCH_USER = 1U << 0,
        PERF_SAMPLE_BRANCH_KERNEL = 1U << 1,
        PERF_SAMPLE_BRANCH_HV = 1U << 2,
        PERF_SAMPLE_BRANCH_ANY = 1U << 3,
        PERF_SAMPLE_BRANCH_ANY_CALL = 1U << 4,
        PERF_SAMPLE_BRANCH_ANY_RETURN = 1U << 5,
        PERF_SAMPLE_BRANCH_IND_CALL = 1U << 6,
        PERF_SAMPLE_BRANCH_MAX = 1U << 7,
    }
}
