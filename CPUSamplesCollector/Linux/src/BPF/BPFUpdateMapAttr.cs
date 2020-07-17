// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal readonly struct BPFUpdateMapAttr
    {
        private readonly long mapFD;

        private readonly unsafe void* key;

        private readonly unsafe void* value;

        private readonly long flags;

        public unsafe BPFUpdateMapAttr(long mapFD, void* key, void* value, long flags)
        {
            this.mapFD = mapFD;
            this.key = key;
            this.value = value;
            this.flags = flags;
        }
    }
}
