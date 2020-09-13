// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    // perf_event_mmap_page
    [StructLayout(LayoutKind.Sequential)]
    internal struct PerfEventMMapPage
    {
        public uint Version;

        public uint CompatVersion;

        public uint Lock;

        public uint Index;

        public long Offset;

        public ulong TimeEnabled;

        public ulong TimeRunning;

        public ulong Capabilities;

        public ushort PMCWidth;

        public ushort TimeShift;

        public uint TimeMult;

        public ulong TimeOffset;

        public ulong TimeZero;

        public uint Size;

        public unsafe fixed byte Reserved[(118 * 8) + 4];

        public ulong DataHead;

        public ulong DataTail;

        public ulong DataOffset;

        public ulong DataSize;

        public ulong AUXHead;

        public ulong AUXTail;

        public ulong AUXOffset;

        public ulong AUXSize;
    }
}
