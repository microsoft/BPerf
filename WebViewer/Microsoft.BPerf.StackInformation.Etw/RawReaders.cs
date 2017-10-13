// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Etw
{
    using System;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;

    internal static class RawReaders
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe ulong ReadAlignedPointer(ref byte* userData, int pointerSize)
        {
            return pointerSize == 8 ? ReadAlignedUInt64(ref userData) : ReadAlignedUInt32(ref userData);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe long ReadAlignedInt64(ref byte* userData)
        {
#if DEBUG
            CheckAlignment(userData, sizeof(long));
#endif
            var retVal = *(long*)userData;
            userData += sizeof(long);
            return retVal;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe ulong ReadAlignedUInt64(ref byte* userData)
        {
#if DEBUG
            CheckAlignment(userData, sizeof(ulong));
#endif
            var retVal = *(ulong*)userData;
            userData += sizeof(ulong);
            return retVal;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe uint ReadAlignedUInt32(ref byte* userData)
        {
#if DEBUG
            CheckAlignment(userData, sizeof(uint));
#endif
            var retVal = *(uint*)userData;
            userData += sizeof(uint);
            return retVal;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe int ReadAlignedInt32(ref byte* userData)
        {
#if DEBUG
            CheckAlignment(userData, sizeof(int));
#endif
            var retVal = *(int*)userData;
            userData += sizeof(int);
            return retVal;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe short ReadAlignedInt16(ref byte* userData)
        {
#if DEBUG
            CheckAlignment(userData, sizeof(short));
#endif
            var retVal = *(short*)userData;
            userData += sizeof(short);
            return retVal;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe string ReadAnsiNullTerminatedString(ref byte* userData)
        {
            var retVal = Marshal.PtrToStringAnsi((IntPtr)userData);
            userData += retVal.Length + 1;
            return retVal;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe string ReadWideNullTerminatedString(ref byte* userData)
        {
            var retVal = Marshal.PtrToStringUni((IntPtr)userData);
            userData += (retVal.Length + 1) * 2;
            return retVal;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe uint ReadUInt32(ref byte* ptr)
        {
            byte a = *ptr;
            byte b = *(ptr + 1);
            byte c = *(ptr + 2);
            byte d = *(ptr + 3);

            ptr += sizeof(uint);

            return (uint)(d << 24 | c << 16 | b << 8 | a);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe int ReadInt32(ref byte* ptr)
        {
            byte a = *ptr;
            byte b = *(ptr + 1);
            byte c = *(ptr + 2);
            byte d = *(ptr + 3);

            ptr += sizeof(int);

            return d << 24 | c << 16 | b << 8 | a;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe short ReadInt16(ref byte* ptr)
        {
            byte a = *ptr;
            byte b = *(ptr + 1);

            ptr += sizeof(short);

            return (short)(b << 8 | a);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe ushort ReadUInt16(ref byte* ptr)
        {
            byte a = *ptr;
            byte b = *(ptr + 1);

            ptr += sizeof(ushort);

            return (ushort)(b << 8 | a);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe Guid ReadAlignedGuid(ref byte* userData)
        {
            int part1 = ReadAlignedInt32(ref userData);
            short part2 = ReadAlignedInt16(ref userData);
            short part3 = ReadAlignedInt16(ref userData);

            var signature = new Guid(part1, part2, part3, *userData, *(userData + 1), *(userData + 2), *(userData + 3), *(userData + 4), *(userData + 5), *(userData + 6), *(userData + 7));

            userData += 8; // 8 single-byte accesses

            return signature;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe Guid ReadGuid(ref byte* userData)
        {
            int part1 = ReadInt32(ref userData);
            short part2 = ReadInt16(ref userData);
            short part3 = ReadInt16(ref userData);

            var signature = new Guid(part1, part2, part3, *userData, *(userData + 1), *(userData + 2), *(userData + 3), *(userData + 4), *(userData + 5), *(userData + 6), *(userData + 7));

            userData += 8; // 8 single-byte accesses

            return signature;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe void SkipUInt8(ref byte* userData)
        {
            userData += sizeof(byte);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe void SkipUInt16(ref byte* userData)
        {
            userData += sizeof(ushort);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe void SkipUInt32(ref byte* userData)
        {
            userData += sizeof(uint);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe void SkipUInt64(ref byte* userData)
        {
            userData += sizeof(ulong);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe void SkipPointer(ref byte* userData, int pointerSize)
        {
            userData += pointerSize;
        }

        public static unsafe void SkipUserSID(ref byte* p, int pointerSize)
        {
            int sid = ReadInt32(ref p);
            p -= sizeof(int);

            if (sid == 0)
            {
                p += 4;
            }

            int tokenSize = HostOffset(pointerSize, 8, 2);
            int numAuthorities = (int)(byte)*(p + tokenSize + 1);
            p += tokenSize + 8 + (4 * numAuthorities);
        }

        private static unsafe void CheckAlignment(byte* p, int n)
        {
            var intPtr = new IntPtr(p);
            bool isAligned = IntPtr.Size == 8 ? intPtr.ToInt64() % n == 0 : intPtr.ToInt32() % n == 0;
            if (!isAligned)
            {
                throw new Exception("Unaligned");
            }
        }

        private static int HostOffset(int pointerSize, int offset, int numPointers)
        {
            return offset + ((pointerSize - 4) * numPointers);
        }
    }
}
