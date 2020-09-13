// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;

    [Flags]
    internal enum PerfBranchSampleType : ulong
    {
        PERF_SAMPLE_BRANCH_USER = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_USER_SHIFT,
        PERF_SAMPLE_BRANCH_KERNEL = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_KERNEL_SHIFT,
        PERF_SAMPLE_BRANCH_HV = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_HV_SHIFT,
        PERF_SAMPLE_BRANCH_ANY = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_ANY_SHIFT,
        PERF_SAMPLE_BRANCH_ANY_CALL = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_ANY_CALL_SHIFT,
        PERF_SAMPLE_BRANCH_ANY_RETURN = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_ANY_RETURN_SHIFT,
        PERF_SAMPLE_BRANCH_IND_CALL = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_IND_CALL_SHIFT,
        PERF_SAMPLE_BRANCH_ABORT_TX = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_ABORT_TX_SHIFT,
        PERF_SAMPLE_BRANCH_IN_TX = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_IN_TX_SHIFT,
        PERF_SAMPLE_BRANCH_NO_TX = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_NO_TX_SHIFT,
        PERF_SAMPLE_BRANCH_COND = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_COND_SHIFT,
        PERF_SAMPLE_BRANCH_CALL_STACK = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_CALL_STACK_SHIFT,
        PERF_SAMPLE_BRANCH_IND_JUMP = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_IND_JUMP_SHIFT,
        PERF_SAMPLE_BRANCH_CALL = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_CALL_SHIFT,
        PERF_SAMPLE_BRANCH_NO_FLAGS = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_NO_FLAGS_SHIFT,
        PERF_SAMPLE_BRANCH_NO_CYCLES = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_NO_CYCLES_SHIFT,
        PERF_SAMPLE_BRANCH_TYPE_SAVE = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_TYPE_SAVE_SHIFT,
        PERF_SAMPLE_BRANCH_MAX = 1U << PerfBranchSampleTypeShift.PERF_SAMPLE_BRANCH_MAX_SHIFT,
    }
}
