// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;

    [Flags]
    internal enum BPFJumpEncoding
    {
        BPF_JNE = 0x50, /* jump != */
        BPF_JLT = 0xa0, /* LT is unsigned, '<' */
        BPF_JLE = 0xb0, /* LE is unsigned, '<=' */
        BPF_JSGT = 0x60, /* SGT is signed '>', GT in x86 */
        BPF_JSGE = 0x70, /* SGE is signed '>=', GE in x86 */
        BPF_JSLT = 0xc0, /* SLT is signed, '<' */
        BPF_JSLE = 0xd0, /* SLE is signed, '<=' */
        BPF_CALL = 0x80, /* function call */
        BPF_EXIT = 0x90, /* function return */
    }
}
