// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal struct StatX
    {
        public uint Mask;

        public uint BlkSize;

        public ulong Attributes;

        public uint STXNLink;

        public uint STXUserId;

        public uint STXGroupId;

        public ushort STXMode;

        public ushort Padding;

        public ulong INode;

        public ulong Size;

        public ulong Blocks;

        public ulong AttributesMask;

        public StatXTimestamp AccessTime;

        public StatXTimestamp CreationTime;

        public StatXTimestamp AttributeChangeTime;

        public StatXTimestamp ModificationTime;

        public uint RDevMajor;

        public uint RDevMinor;

        public uint DevMajor;

        public uint DevMinor;

        public unsafe fixed ulong PaddingArray[14];
    }
}
