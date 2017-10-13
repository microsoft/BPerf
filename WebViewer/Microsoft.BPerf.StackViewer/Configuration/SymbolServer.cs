// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using Microsoft.BPerf.SymbolicInformation.Abstractions;

    public sealed class SymbolServer : ISymbolServer
    {
        public string Url { get; set; }

        public string AuthorizationHeader { get; set; }

        public int Priority { get; set; }
    }
}