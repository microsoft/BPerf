// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolicInformation.ProgramDatabase
{
    using System.Collections.Generic;
    using System.Text;
    using Dia2Lib;
    using Microsoft.BPerf.StackInformation.Abstractions;
    using Microsoft.BPerf.SymbolicInformation.Abstractions;

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
            var dataSource = DiaLoader.GetDiaSourceObject();
            dataSource.loadDataFromPdb(this.pdbPath);
            dataSource.openSession(out var session);

            session.findLinesByRVA(rva, 0, out var sourceLocs);
            sourceLocs.Next(1, out var sourceLoc, out var fetchCount);
            if (fetchCount == 0)
            {
                return null;
            }

            dataSource.getStreamSize("srcsrv", out var len);

            string srcSrvString = null;
            if (len > 0)
            {
                var buffer = new byte[len];
                unsafe
                {
                    fixed (byte* bufferPtr = buffer)
                    {
                        dataSource.getStreamRawData("srcsrv", len, out *bufferPtr);
                        srcSrvString = Encoding.UTF8.GetString(buffer);
                    }
                }
            }

            var sourceFile = new SourceFile(sourceLoc.sourceFile.fileName, len == 0 ? SourceKind.Indexed : SourceKind.None, srcSrvString); // we should support embedded
            var lineNum = (int)sourceLoc.lineNumber;

            return new SourceLocation(sourceFile, lineNum);
        }

        public SourceLocation FindSourceLocation(uint methodToken, int offset)
        {
            var dataSource = DiaLoader.GetDiaSourceObject();
            dataSource.loadDataFromPdb(this.pdbPath);
            dataSource.openSession(out var session);

            session.findSymbolByToken(methodToken, SymTagEnum.SymTagFunction, out var methodSym);
            if (methodSym == null)
            {
                return null;
            }

            session.findLinesByRVA(methodSym.relativeVirtualAddress + (uint)offset, 256, out var sourceLocs);
            sourceLocs.Next(1, out var sourceLoc, out uint fetchCount);
            if (fetchCount == 0)
            {
                return null;
            }

            dataSource.getStreamSize("srcsrv", out var len);

            string srcSrvString = null;
            if (len > 0)
            {
                var buffer = new byte[len];
                unsafe
                {
                    fixed (byte* bufferPtr = buffer)
                    {
                        dataSource.getStreamRawData("srcsrv", len, out *bufferPtr);
                        srcSrvString = Encoding.UTF8.GetString(buffer);
                    }
                }
            }

            var sourceFile = new SourceFile(sourceLoc.sourceFile.fileName, len == 0 ? SourceKind.Indexed : SourceKind.None, srcSrvString); // we should support embedded

            int lineNum;

            while (true)
            {
                lineNum = (int)sourceLoc.lineNumber;
                if (lineNum != 0xFEEFEE)
                {
                    break;
                }

                lineNum = 0;
                sourceLocs.Next(1, out sourceLoc, out fetchCount);
                if (fetchCount == 0)
                {
                    break;
                }
            }

            return new SourceLocation(sourceFile, lineNum);
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

                var dataSource = DiaLoader.GetDiaSourceObject();
                dataSource.loadDataFromPdb(this.pdbPath);
                dataSource.openSession(out var session);
                session.getSymbolsByAddr(out var symbolsByAddr);

                foreach (var rva in rvasToLookup)
                {
                    this.AddEntry(session, symbolsByAddr, rva + imageBase, rva);
                }

                symbolsByAddr = null;
                session = null;
                dataSource = null;
            }
        }

        private bool HasAnyInstructionPointersForProcess(uint pid)
        {
            return this.processIdsProcessed.Contains(pid);
        }

        private void AddEntry(IDiaSession session, IDiaEnumSymbolsByAddr symbolsByAddr, ulong eip, uint rva)
        {
            session.findSymbolByRVA(rva, SymTagEnum.SymTagPublicSymbol, out var symbol);
            if (symbol == null)
            {
                session.findSymbolByRVA(rva, SymTagEnum.SymTagFunction, out symbol);
                if (symbol == null)
                {
                    symbol = symbolsByAddr.symbolByRVA(rva);
                }
            }

            string functionName;
            if (symbol != null)
            {
                symbol.get_undecoratedNameEx(0x1000, out var unmangled);
                if (!string.IsNullOrEmpty(unmangled))
                {
                    functionName = unmangled;
                }
                else
                {
                    functionName = !string.IsNullOrEmpty(symbol.name) ? symbol.name : eip.ToString("x");
                }
            }
            else
            {
                functionName = eip.ToString("x");
            }

            this.rvaToFunctionNameMap.Add(rva, functionName);
        }
    }
}
