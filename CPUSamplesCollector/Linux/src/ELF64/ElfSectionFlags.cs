// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    internal enum ElfSectionFlags : uint
    {
        // Section data should be writable during execution.
        SHF_WRITE = 0x1,

        // Section occupies memory during program execution.
        SHF_ALLOC = 0x2,

        // Section contains executable machine instructions.
        SHF_EXECINSTR = 0x4,

        // The data in this section may be merged.
        SHF_MERGE = 0x10,

        // The data in this section is null-terminated strings.
        SHF_STRINGS = 0x20,

        // A field in this section holds a section header table index.
        SHF_INFO_LINK = 0x40U,

        // Adds special ordering requirements for link editors.
        SHF_LINK_ORDER = 0x80U,

        // This section requires special OS-specific processing to avoid incorrect
        // behavior.
        SHF_OS_NONCONFORMING = 0x100U,

        // This section is a member of a section group.
        SHF_GROUP = 0x200U,

        // This section holds Thread-Local Storage.
        SHF_TLS = 0x400U,

        // Identifies a section containing compressed data.
        SHF_COMPRESSED = 0x800U,

        // This section is excluded from the final executable or shared library.
        SHF_EXCLUDE = 0x80000000U,
    }
}
