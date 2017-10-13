// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System.Collections.Generic;

    public sealed class SymbolServerInformation
    {
        public IEnumerable<SymbolServer> SymbolServers { get; set; }

        public string DownloadLocation { get; set; }
    }
}