// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;

    internal enum ELFHeaderType : ushort
    {
        Relocatable = 1,
        Executable = 2,
        Shared = 3,
        Core = 4,
    }

    internal struct ELFHeader
    {
        public static ReadOnlySpan<byte> ElfMagic => new byte[] { 0x7F, (byte)'E', (byte)'L', (byte)'F', (byte)'\0' };

        public unsafe fixed char Identity[16]; /* ELF identification */
        public ELFHeaderType Type; // e_type; /* Object file type */
        public ushort Machine; // e_machine; /* Machine type */
        public uint Version; // e_version; /* Object file version */
        public ulong EntryPoint; // e_entry; /* Entry point address */
        public ulong ProgramHeaderOffset; // e_phoff; /* Program header offset */
        public ulong SectionHeaderOffset; // e_shoff; /* Section header offset */
        public uint Flags; // e_flags; /* Processor-specific flags */
        public ushort EHSize; // e_ehsize; /* ELF header size */
        public ushort ProgramHeaderEntrySize; // e_phentsize; /* Size of program header entry */
        public ushort ProgramHeaderCount; // e_phnum; /* Number of program header entries */
        public ushort SectionHeaderEntrySize; // e_shentsize; /* Size of section header entry */
        public ushort SectionHeaderCount; // e_shnum; /* Number of section header entries */
        public ushort SectionHeaderStringIndex; // e_shstrndx; /* Section name string table index */
    }
}
