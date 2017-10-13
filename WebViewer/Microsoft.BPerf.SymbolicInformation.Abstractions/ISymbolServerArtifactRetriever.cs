// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolicInformation.Abstractions
{
    using System;
    using System.Threading.Tasks;

    public interface ISymbolServerArtifactRetriever
    {
        ValueTask<string> DownloadPdb(string pdbName, Guid signature, uint age, bool skipSymbolServer);

        ValueTask<string> DownloadPortablePdb(string pdbName, Guid signature, bool skipSymbolServer);
    }
}
