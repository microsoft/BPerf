// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolicInformation.Interfaces
{
    public sealed class SourceLocation
    {
        public SourceLocation(SourceFile sourceFile, int lineNumber)
        {
            // Invalid line number
            if (lineNumber >= 0xFEEFEE)
            {
                lineNumber = 0;
            }

            this.SourceFile = sourceFile;
            this.LineNumber = lineNumber;
        }

        public SourceFile SourceFile { get; }

        public int LineNumber { get; }
    }
}
