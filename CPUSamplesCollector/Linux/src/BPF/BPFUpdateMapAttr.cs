// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Explicit)]
    internal readonly struct BPFUpdateMapAttr
    {
        [FieldOffset(0)]
        private readonly int mapfd;

        [FieldOffset(8)]
        private readonly ulong key;

        [FieldOffset(16)]
        private readonly ulong value;

        [FieldOffset(24)]
        private readonly ulong flags;

        public unsafe BPFUpdateMapAttr(int mapfd, void* key, void* value, ulong flags)
        {
            this.mapfd = mapfd;
            this.key = (ulong)key;
            this.value = (ulong)value;
            this.flags = flags;
        }
    }
}
