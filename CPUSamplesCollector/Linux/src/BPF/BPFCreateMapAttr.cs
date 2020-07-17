// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal readonly struct BPFCreateMapAttr
    {
        private readonly int mapType;

        private readonly int keySize;

        private readonly int valueSize;

        private readonly int maxEntries;

        private readonly int mapFlags;

        public BPFCreateMapAttr(int mapType, int keySize, int valueSize, int maxEntries, int mapFlags)
        {
            this.mapType = mapType;
            this.keySize = keySize;
            this.valueSize = valueSize;
            this.maxEntries = maxEntries;
            this.mapFlags = mapFlags;
        }
    }
}
