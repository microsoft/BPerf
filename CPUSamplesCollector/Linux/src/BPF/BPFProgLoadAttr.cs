// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Explicit)]
    internal readonly struct BPFProgLoadAttr
    {
        [FieldOffset(0)]
        private readonly BPFProgType progType;

        [FieldOffset(4)]
        private readonly int instructionCount;

        [FieldOffset(8)]
        private readonly ulong instructions;

        [FieldOffset(16)]
        private readonly ulong license;

        [FieldOffset(24)]
        private readonly int logLevel;

        [FieldOffset(28)]
        private readonly int logSize;

        [FieldOffset(32)]
        private readonly ulong logBuffer;

        [FieldOffset(40)]
        private readonly int kernVersion; /* Checked when ProgType=KProbe */

        [FieldOffset(44)]
        private readonly int progFlags;

        [FieldOffset(48)]
        private readonly ulong progName1;

        [FieldOffset(56)]
        private readonly ulong progName2;

        public unsafe BPFProgLoadAttr(BPFProgType progType, int instructionCount, void* instructions, void* license, int logLevel, int logSize, void* logBuffer, int kernVersion)
        {
            this.progType = progType;
            this.instructionCount = instructionCount;
            this.instructions = (ulong)instructions;
            this.license = (ulong)license;
            this.logLevel = logLevel;
            this.logSize = logSize;
            this.logBuffer = (ulong)logBuffer;
            this.kernVersion = kernVersion;
            this.progFlags = 0;
            this.progName1 = 0;
            this.progName2 = 0;
        }
    }
}
