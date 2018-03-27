// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using Microsoft.BPerf.SymbolServer.Interfaces;

    public sealed class SourceServer : ISourceServerAuthorizationInformation
    {
        public string UrlPrefix { get; set; }

        public string AuthorizationHeader { get; set; }
    }
}
