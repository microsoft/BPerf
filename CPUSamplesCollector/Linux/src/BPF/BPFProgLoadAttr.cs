// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal readonly struct BPFProgLoadAttr
    {
        private readonly BPFProgType progType;

        private readonly int instructionCount;

        private readonly unsafe void* instructions;

        private readonly unsafe void* license;

        private readonly int logLevel;

        private readonly int logSize;

        private readonly unsafe void* logBuffer;

        private readonly int kernVersion; /* Checked when ProgType=KProbe */

        public unsafe BPFProgLoadAttr(BPFProgType progType, int instructionCount, void* instructions, void* license, int logLevel, int logSize, void* logBuffer, int kernVersion)
        {
            this.progType = progType;
            this.instructionCount = instructionCount;
            this.instructions = instructions;
            this.license = license;
            this.logLevel = logLevel;
            this.logSize = logSize;
            this.logBuffer = logBuffer;
            this.kernVersion = kernVersion;
        }
    }
}
