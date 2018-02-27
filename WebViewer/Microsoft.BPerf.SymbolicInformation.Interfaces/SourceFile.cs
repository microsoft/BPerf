// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolicInformation.Interfaces
{
    public sealed class SourceFile
    {
        public SourceFile(string buildTimeFilePath, SourceKind kind, string srcSrvString)
        {
            this.BuildTimeFilePath = buildTimeFilePath;
            this.Kind = kind;
            this.SrcSrvString = srcSrvString;
        }

        public string BuildTimeFilePath { get; }

        public string SrcSrvString { get; }

        public SourceKind Kind { get; }
    }
}
