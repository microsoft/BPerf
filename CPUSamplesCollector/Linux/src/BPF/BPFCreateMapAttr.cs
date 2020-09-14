// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal readonly struct BPFCreateMapAttr
    {
        private readonly uint mapType;

        private readonly uint keySize;

        private readonly uint valueSize;

        private readonly uint maxEntries;

        private readonly uint mapFlags;

        public BPFCreateMapAttr(uint mapType, uint keySize, uint valueSize, uint maxEntries, uint mapFlags)
        {
            this.mapType = mapType;
            this.keySize = keySize;
            this.valueSize = valueSize;
            this.maxEntries = maxEntries;
            this.mapFlags = mapFlags;
        }
    }
}
