// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolicInformation.ProgramDatabase
{
    internal struct LineColumnInformation
    {
        public LineColumnInformation(string filename, uint lineStart, uint lineEnd, ushort columnStart, ushort columnEnd)
        {
            this.Filename = filename;
            this.LineStart = lineStart;
            this.LineEnd = lineEnd;
            this.ColumnStart = columnStart;
            this.ColumnEnd = columnEnd;
        }

        public string Filename { get; }

        public uint LineStart { get; }

        public uint LineEnd { get; }

        public ushort ColumnStart { get; }

        public ushort ColumnEnd { get; }
    }
}
