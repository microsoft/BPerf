// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal readonly struct RLimit
    {
        private readonly ulong current;

        private readonly ulong maximum;

        public RLimit(ulong cur, ulong max)
        {
            this.current = cur;
            this.maximum = max;
        }
    }
}
