// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    internal enum ELFProgramHeaderType : uint
    {
        Null = 0,
        Load = 1,
        Dynamic = 2,
        Interp = 3,
        Note = 4,
        Shlib = 5,
        Phdr = 6,
    }

    internal struct ELFProgramHeader
    {
        public ELFProgramHeaderType Type;       // p_type /* Type of segment */
        public uint Flags;                      // p_flags /* Segment attributes */
        public ulong FileOffset;                // p_offset /* Offset in file */
        public ulong VirtualAddress;            // p_vaddr /* Virtual address in memory */
        public ulong PhysicalAddress;           // p_paddr /* Reserved */
        public ulong FileSize;                  // p_filesz /* Size of segment in file */
        public ulong VirtualSize;               // p_memsz /* Size of segment in memory */
        public ulong Alignment;                 // p_align /* Alignment of segment */
    }
}
