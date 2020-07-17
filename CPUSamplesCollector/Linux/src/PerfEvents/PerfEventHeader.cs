// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Explicit)]
    internal struct PerfEventHeader
    {
        [FieldOffset(0)]
        public PerfRecordType Type;

        [FieldOffset(4)]
        public ushort Misc;

        [FieldOffset(6)]
        public ushort Size;
    }
}
