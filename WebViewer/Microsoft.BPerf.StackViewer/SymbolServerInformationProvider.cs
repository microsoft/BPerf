// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System;
    using System.Collections.Generic;
    using Microsoft.BPerf.SymbolServer.Interfaces;
    using Microsoft.Extensions.Options;

    public sealed class SymbolServerInformationProvider : ISymbolServerInformation
    {
        public SymbolServerInformationProvider(IOptions<SymbolServerInformation> options)
        {
            this.SymbolServers = options.Value.SymbolServers;
            this.DownloadLocation = Environment.ExpandEnvironmentVariables(options.Value.DownloadLocation);
        }

        public IEnumerable<ISymbolServer> SymbolServers { get; }

        public string DownloadLocation { get; }
    }
}
