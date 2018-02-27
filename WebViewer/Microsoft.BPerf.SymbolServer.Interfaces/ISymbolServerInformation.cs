// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolServer.Interfaces
{
    using System.Collections.Generic;

    public interface ISymbolServerInformation
    {
        IEnumerable<ISymbolServer> SymbolServers { get; }

        string DownloadLocation { get; }
    }
}
