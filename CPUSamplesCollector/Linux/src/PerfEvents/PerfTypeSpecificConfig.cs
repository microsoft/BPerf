// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    internal enum PerfTypeSpecificConfig : ulong
    {
        /* perf_sw_ids */

        PERF_COUNT_SW_CPU_CLOCK = 0,
        PERF_COUNT_SW_TASK_CLOCK = 1,
        PERF_COUNT_SW_PAGE_FAULTS = 2,
        PERF_COUNT_SW_CONTEXT_SWITCHES = 3,
        PERF_COUNT_SW_CPU_MIGRATIONS = 4,
        PERF_COUNT_SW_PAGE_FAULTS_MIN = 5,
        PERF_COUNT_SW_PAGE_FAULTS_MAJ = 6,
        PERF_COUNT_SW_ALIGNMENT_FAULTS = 7,
        PERF_COUNT_SW_EMULATION_FAULTS = 8,
        PERF_COUNT_SW_DUMMY = 9,
        PERF_COUNT_SW_BPF_OUTPUT = 10,

        /* perf_hw_id */

        PERF_COUNT_HW_CPU_CYCLES = 0,
        PERF_COUNT_HW_INSTRUCTIONS = 1,
        PERF_COUNT_HW_CACHE_REFERENCES = 2,
        PERF_COUNT_HW_CACHE_MISSES = 3,
        PERF_COUNT_HW_BRANCH_INSTRUCTIONS = 4,
        PERF_COUNT_HW_BRANCH_MISSES = 5,
        PERF_COUNT_HW_BUS_CYCLES = 6,
        PERF_COUNT_HW_STALLED_CYCLES_FRONTEND = 7,
        PERF_COUNT_HW_STALLED_CYCLES_BACKEND = 8,
        PERF_COUNT_HW_REF_CPU_CYCLES = 9,

        /* perf_hw_cache_id */

        PERF_COUNT_HW_CACHE_L1D = 0,
        PERF_COUNT_HW_CACHE_L1I = 1,
        PERF_COUNT_HW_CACHE_LL = 2,
        PERF_COUNT_HW_CACHE_DTLB = 3,
        PERF_COUNT_HW_CACHE_ITLB = 4,
        PERF_COUNT_HW_CACHE_BPU = 5,
        PERF_COUNT_HW_CACHE_NODE = 6,

        /* perf_hw_cache_op_id */

        PERF_COUNT_HW_CACHE_OP_READ = 0,
        PERF_COUNT_HW_CACHE_OP_WRITE = 1,
        PERF_COUNT_HW_CACHE_OP_PREFETCH = 2,

        /* perf_hw_cache_op_result_id */

        PERF_COUNT_HW_CACHE_RESULT_ACCESS = 0,
        PERF_COUNT_HW_CACHE_RESULT_MISS = 1,
    }
}
