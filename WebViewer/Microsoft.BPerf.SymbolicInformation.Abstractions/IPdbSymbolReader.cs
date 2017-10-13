// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolicInformation.Abstractions
{
    using System.Collections.Generic;
    using Microsoft.BPerf.StackInformation.Abstractions;

    public interface IPdbSymbolReader
    {
        string FindNameForRVA(uint rva);

        SourceLocation FindSourceLocation(uint rva);

        SourceLocation FindSourceLocation(uint methodToken, int offset);

        void AddInstructionPointersForProcess(uint pid, ulong imageBase, HashSet<ulong> instructionPointers);
    }
}
