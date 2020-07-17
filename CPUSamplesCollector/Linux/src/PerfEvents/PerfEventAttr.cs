// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Explicit)]
    internal struct PerfEventAttr
    {
        /// <summary>
        /// Type of event
        /// </summary>
        [FieldOffset(0)]
        public PerfEventType Type;

        /// <summary>
        /// Size of attribute structure
        /// </summary>
        [FieldOffset(4)]
        public int Size;

        /// <summary>
        /// Type-specific configuration.
        /// </summary>
        [FieldOffset(8)]
        public PerfTypeSpecificConfig Config;

        /// <summary>
        /// Period of sampling
        /// </summary>
        [FieldOffset(16)]
        public ulong SamplePeriod;

        /// <summary>
        /// Frequency of sampling
        /// </summary>
        [FieldOffset(16)]
        public ulong SampleFreq;

        /// <summary>
        /// Specifies values included in sample
        /// </summary>
        [FieldOffset(24)]
        public PerfEventSampleFormat SampleType;

        /// <summary>
        /// Specifies values returned in read
        /// </summary>
        [FieldOffset(32)]
        public PerfEventReadFormat ReadFormat;

        /// <summary>
        /// Flags
        /// </summary>
        [FieldOffset(40)]
        public PerfEventAttrFlags Flags;

        /// <summary>
        /// wakeup every n events
        /// </summary>
        [FieldOffset(48)]
        public uint WakeupEvents;

        /// <summary>
        /// bytes before wakeup
        /// </summary>
        [FieldOffset(48)]
        public uint WakeupWatermark;

        /// <summary>
        /// breakpoint type
        /// </summary>
        [FieldOffset(52)]
        public PerfEventBreakpointType BPType;

        /// <summary>
        /// breakpoint address
        /// </summary>
        [FieldOffset(56)]
        public ulong BPAddr;

        /// <summary>
        /// for perf_kprobe
        /// </summary>
        [FieldOffset(56)]
        public ulong KProbeFunc;

        /// <summary>
        /// for perf_uprobe
        /// </summary>
        [FieldOffset(56)]
        public ulong UProbePath;

        /// <summary>
        /// extension of config
        /// </summary>
        [FieldOffset(56)]
        public ulong Config1;

        /// <summary>
        /// breakpoint length
        /// </summary>
        [FieldOffset(64)]
        public ulong BPLen;

        /// <summary>
        /// with kprobe_func == NULL
        /// </summary>
        [FieldOffset(64)]
        public ulong KProbeAddr;

        /// <summary>
        /// for perf_[k,u]probe
        /// </summary>
        [FieldOffset(64)]
        public ulong ProbeOffset;

        /// <summary>
        /// extension of config1
        /// </summary>
        [FieldOffset(64)]
        public ulong Config2;

        /// <summary>
        /// enum perf_branch_sample_type
        /// </summary>
        [FieldOffset(72)]
        public PerfBranchSampleType BranchSampleType;

        /// <summary>
        /// user regs to dump on samples
        /// </summary>
        [FieldOffset(80)]
        public ulong SampleRegsUser;

        /// <summary>
        /// size of stack to dump on samples
        /// </summary>
        [FieldOffset(88)]
        public uint SampleStackUser;

        /// <summary>
        /// clock to use for time fields
        /// </summary>
        [FieldOffset(92)]
        public ClockConstants ClockId;

        /// <summary>
        /// regs to dump on samples
        /// </summary>
        [FieldOffset(96)]
        public ulong SampleRegsIntr;

        /// <summary>
        /// aux bytes before wakeup
        /// </summary>
        [FieldOffset(104)]
        public uint AuxWatermark;

        /// <summary>
        /// max frames in callchain
        /// </summary>
        [FieldOffset(108)]
        public ushort SampleMaxStack;

        /// <summary>
        /// align to u64
        /// </summary>
        [FieldOffset(110)]
        public short Reserved2;

        /// <summary>
        /// max sample size
        /// </summary>
        [FieldOffset(112)]
        public uint AuxSampleSize;

        /// <summary>
        /// align to u64
        /// </summary>
        [FieldOffset(116)]
        public int Reserved3;
    }
}
