// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.PdbSymbolReader.Interfaces
{
    using System;
    using System.Threading.Tasks;

    public interface ITracePdbSymbolReaderProvider
    {
        ValueTask<IPdbSymbolReader> GetSymbolReader(string peFilePath);

        ValueTask<IPdbSymbolReader> GetSymbolReader(string peFilePath, Guid signature, uint age, string pdbName);

        IPdbSymbolReader GetSymbolReader(Guid signature, uint age, string pdbName);
    }
}
