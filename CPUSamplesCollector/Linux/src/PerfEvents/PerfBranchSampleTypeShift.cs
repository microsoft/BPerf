// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    // perf_branch_sample_type_shift
    internal enum PerfBranchSampleTypeShift
    {
        PERF_SAMPLE_BRANCH_USER_SHIFT = 0, /* user branches */
        PERF_SAMPLE_BRANCH_KERNEL_SHIFT = 1, /* kernel branches */
        PERF_SAMPLE_BRANCH_HV_SHIFT = 2, /* hypervisor branches */
        PERF_SAMPLE_BRANCH_ANY_SHIFT = 3, /* any branch types */
        PERF_SAMPLE_BRANCH_ANY_CALL_SHIFT = 4, /* any call branch */
        PERF_SAMPLE_BRANCH_ANY_RETURN_SHIFT = 5, /* any return branch */
        PERF_SAMPLE_BRANCH_IND_CALL_SHIFT = 6, /* indirect calls */
        PERF_SAMPLE_BRANCH_ABORT_TX_SHIFT = 7, /* transaction aborts */
        PERF_SAMPLE_BRANCH_IN_TX_SHIFT = 8, /* in transaction */
        PERF_SAMPLE_BRANCH_NO_TX_SHIFT = 9, /* not in transaction */
        PERF_SAMPLE_BRANCH_COND_SHIFT = 10, /* conditional branches */
        PERF_SAMPLE_BRANCH_CALL_STACK_SHIFT = 11, /* call/ret stack */
        PERF_SAMPLE_BRANCH_IND_JUMP_SHIFT = 12, /* indirect jumps */
        PERF_SAMPLE_BRANCH_CALL_SHIFT = 13, /* direct call */
        PERF_SAMPLE_BRANCH_NO_FLAGS_SHIFT = 14, /* no flags */
        PERF_SAMPLE_BRANCH_NO_CYCLES_SHIFT = 15, /* no cycles */
        PERF_SAMPLE_BRANCH_TYPE_SAVE_SHIFT = 16, /* save branch type */
        PERF_SAMPLE_BRANCH_MAX_SHIFT, /* non-ABI */
    }
}
