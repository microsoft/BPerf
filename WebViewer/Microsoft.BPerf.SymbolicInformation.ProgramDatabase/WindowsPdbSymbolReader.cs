// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolicInformation.ProgramDatabase
{
    using System.Collections.Generic;
    using System.Text;
    using Microsoft.BPerf.PdbSymbolReader.Interfaces;
    using Microsoft.BPerf.SymbolicInformation.Interfaces;

    internal sealed class WindowsPdbSymbolReader : IPdbSymbolReader
    {
        private readonly string pdbPath;

        private readonly Dictionary<uint, string> rvaToFunctionNameMap = new Dictionary<uint, string>();

        private readonly HashSet<uint> processIdsProcessed = new HashSet<uint>();

        public WindowsPdbSymbolReader(string pdbPath)
        {
            this.pdbPath = pdbPath;
        }

        public string FindNameForRVA(uint rva)
        {
            return !this.rvaToFunctionNameMap.TryGetValue(rva, out string retVal) ? null : retVal;
        }

        public SourceLocation FindSourceLocation(uint rva)
        {
            LineColumnInformation lineColumnInformation;
            string sourceLink;
            string srcSrv;

            using (var reader = new NativePdbReader(this.pdbPath))
            {
                reader.GetSourceLinkData(out sourceLink);
                reader.GetSrcSrvData(out srcSrv);

                // TODO: FIXME
                if (!reader.FindLineNumberForNativeMethod(rva, out lineColumnInformation))
                {
                    return null;
                }
            }

            var sourceIndexKind = SourceKind.None;

            if (!string.IsNullOrEmpty(srcSrv))
            {
                sourceIndexKind = SourceKind.SrcSrv;
            }
            else if (!string.IsNullOrEmpty(srcSrv))
            {
                sourceIndexKind = SourceKind.SourceLink;
            }

            var sourceFile = new SourceFile(lineColumnInformation.Filename, sourceIndexKind, sourceIndexKind == SourceKind.SourceLink ? sourceLink : srcSrv); // we should support embedded

            return new SourceLocation(sourceFile, (int)lineColumnInformation.LineStart);
        }

        public SourceLocation FindSourceLocation(uint methodToken, int offset)
        {
            LineColumnInformation lineColumnInformation;
            string sourceLink;
            string srcSrv;

            using (var reader = new NativePdbReader(this.pdbPath))
            {
                reader.GetSourceLinkData(out sourceLink);
                reader.GetSrcSrvData(out srcSrv);

                if (!reader.FindLineNumberForManagedMethod(methodToken, (uint)offset, out lineColumnInformation))
                {
                    return null;
                }
            }

            var sourceIndexKind = SourceKind.None;

            if (!string.IsNullOrEmpty(srcSrv))
            {
                sourceIndexKind = SourceKind.SrcSrv;
            }
            else if (!string.IsNullOrEmpty(srcSrv))
            {
                sourceIndexKind = SourceKind.SourceLink;
            }

            var sourceFile = new SourceFile(lineColumnInformation.Filename, sourceIndexKind, sourceIndexKind == SourceKind.SourceLink ? sourceLink : srcSrv); // we should support embedded

            return new SourceLocation(sourceFile, (int)lineColumnInformation.LineStart);
        }

        public void AddInstructionPointersForProcess(uint pid, ulong imageBase, HashSet<ulong> instructionPointers)
        {
            if (this.HasAnyInstructionPointersForProcess(pid))
            {
                return;
            }

            this.AddInstructionPointersForProcessInternal(pid, imageBase, instructionPointers);
        }

        private void AddInstructionPointersForProcessInternal(uint pid, ulong imageBase, HashSet<ulong> instructionPointers)
        {
            lock (this.processIdsProcessed)
            {
                this.processIdsProcessed.Add(pid);
                var rvasToLookup = new List<uint>();

                foreach (var eip in instructionPointers)
                {
                    uint rva = (uint)(eip - imageBase);
                    if (!this.rvaToFunctionNameMap.ContainsKey(rva))
                    {
                        rvasToLookup.Add(rva);
                    }
                }

                if (rvasToLookup.Count == 0)
                {
                    return;
                }

                using (var reader = new NativePdbReader(this.pdbPath))
                {
                    foreach (var rva in rvasToLookup)
                    {
                        ulong eip = rva + imageBase;

                        if (!reader.FindNameForRVA(rva, out var functionName))
                        {
                            functionName = eip.ToString("x");
                        }

                        this.rvaToFunctionNameMap.Add(rva, functionName);
                    }
                }
            }
        }

        private bool HasAnyInstructionPointersForProcess(uint pid)
        {
            return this.processIdsProcessed.Contains(pid);
        }
    }
}
