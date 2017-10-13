// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolicInformation.Abstractions
{
    public interface ISymbolServer
    {
        string Url { get; }

        string AuthorizationHeader { get; }

        int Priority { get; }
    }
}