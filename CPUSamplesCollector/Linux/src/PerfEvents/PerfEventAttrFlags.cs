// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;

    [Flags]
    internal enum PerfEventAttrFlags : ulong
    {
        Disabled = 1,                      /* off by default        */
        Inherit = 1 << 1,                  /* children inherit it   */
        Pinned = 1 << 2,                   /* must always be on PMU */
        Exclusive = 1 << 3,                /* only group on PMU     */
        ExcludeUser = 1 << 4,              /* don't count user      */
        ExcludeKernel = 1 << 5,            /* ditto kernel          */
        ExcludeHv = 1 << 6,                /* ditto hypervisor      */
        ExcludeIdle = 1 << 7,              /* don't count when idle */
        MMap = 1 << 8,                     /* include mmap data     */
        Comm = 1 << 9,                     /* include comm data     */
        Freq = 1 << 10,                    /* use freq, not period  */
        InheritStat = 1 << 11,             /* per task counts       */
        EnableOnExec = 1 << 12,            /* next exec enables     */
        Task = 1 << 13,                    /* trace fork/exit       */
        Watermark = 1 << 14,               /* wakeup_watermark      */

        /*
         * precise_ip:
         *
         *  0 - SAMPLE_IP can have arbitrary skid
         *  1 - SAMPLE_IP must have constant skid
         *  2 - SAMPLE_IP requested to have 0 skid
         *  3 - SAMPLE_IP must have 0 skid
         */

        PreciseIP1 = 1 << 15,
        PreciseIP2 = 1 << 16,
        MMapData = 1 << 17,                 /* non-exec mmap data    */
        SampleIdAll = 1 << 18,              /* sample_type all events */
        ExcludeHost = 1 << 19,              /* don't count in host   */
        ExcludeGuest = 1 << 20,             /* don't count in guest  */
        ExcludeCallChainKernel = 1 << 21,   /* exclude kernel callchains */
        ExcludeCallChainUser = 1 << 22,     /* exclude user callchains */
        MMap2 = 1 << 23,                    /* include mmap with inode data     */
        CommExec = 1 << 24,                 /* flag comm events that are due to an exec */
        UseClockId = 1 << 25,               /* use @clockid for time fields */
        ContextSwitch = 1 << 26,            /* context switch data */
        WriteBackward = 1 << 27,            /* Write ring buffer from end to beginning */
        Namespaces = 1 << 28,               /* include namespaces data */
        KSymbol = 1 << 29,                  /* context switch data */
        BPF_Event = 1 << 30,                /* include bpf events */
    }
}
