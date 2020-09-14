// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal readonly struct RLimit
    {
        private readonly nuint current;

        private readonly nuint maximum;

        public RLimit(nuint cur, nuint max)
        {
            this.current = cur;
            this.maximum = max;
        }
    }
}
