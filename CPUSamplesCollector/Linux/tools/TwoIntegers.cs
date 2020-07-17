// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace GenStringMap
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Explicit)]
    internal struct TwoIntegers
    {
        [FieldOffset(0)]
        public int RowIndex;

        [FieldOffset(4)]
        public int Length;

        [FieldOffset(0)]
        public long Value;

        public TwoIntegers(int a, int b)
        {
            this.Value = 0;
            this.RowIndex = a;
            this.Length = b;
        }
    }
}
