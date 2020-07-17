// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Explicit, Pack = 1)]
    internal struct EPollEvent
    {
        [FieldOffset(0)]
        public EPollFlags Events;

        [FieldOffset(4)]
        public int Index;

        [FieldOffset(8)]
        public int Unused;
    }
}
