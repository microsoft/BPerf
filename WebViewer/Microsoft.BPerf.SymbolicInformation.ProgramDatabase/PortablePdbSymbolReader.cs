// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolicInformation.ProgramDatabase
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Reflection.Metadata;
    using System.Reflection.Metadata.Ecma335;
    using System.Text;
    using Microsoft.BPerf.StackInformation.Abstractions;
    using Microsoft.BPerf.SymbolicInformation.Abstractions;

    internal sealed class PortablePdbSymbolReader : IPdbSymbolReader
    {
        private static readonly Guid SourceLink = new Guid("CC110556-A091-4D38-9FEC-25AB9A351A6A");

        private static readonly Guid EmbeddedSource = new Guid("0E8A571B-6926-466E-B4AD-8AB04611F5FE");

        private readonly string pdbPath;

        public PortablePdbSymbolReader(string pdbPath)
        {
            this.pdbPath = pdbPath;
        }

        public string FindNameForRVA(uint rva)
        {
            throw new NotSupportedException();
        }

        public SourceLocation FindSourceLocation(uint rva)
        {
            throw new NotSupportedException();
        }

        public string GetSrcSrvStream()
        {
            using (var stream = File.Open(this.pdbPath, FileMode.Open, FileAccess.Read))
            {
                using (var metadataReaderProvider = MetadataReaderProvider.FromPortablePdbStream(stream))
                {
                    var metadataReader = metadataReaderProvider.GetMetadataReader();
                    var customDebugInformationHandleCollection = metadataReader.CustomDebugInformation;

                    foreach (var customDebugInformationHandle in customDebugInformationHandleCollection)
                    {
                        var customDebugInformation = metadataReader.GetCustomDebugInformation(customDebugInformationHandle);

                        if (metadataReader.GetGuid(customDebugInformation.Kind) == SourceLink)
                        {
                            return Encoding.UTF8.GetString(metadataReader.GetBlobBytes(customDebugInformation.Value));
                        }
                    }
                }
            }

            return null;
        }

        public SourceLocation FindSourceLocation(uint methodToken, int offset)
        {
            using (var stream = File.Open(this.pdbPath, FileMode.Open, FileAccess.Read))
            {
                using (var metadataReaderProvider = MetadataReaderProvider.FromPortablePdbStream(stream))
                {
                    var metadataReader = metadataReaderProvider.GetMetadataReader();
                    var methodDebugInformationHandle = MetadataTokens.MethodDebugInformationHandle((int)methodToken);
                    var methodDebugInformation = metadataReader.GetMethodDebugInformation(methodDebugInformationHandle);
                    var documentHandle = methodDebugInformation.Document;

                    if (documentHandle.IsNil)
                    {
                        return null;
                    }

                    var document = metadataReader.GetDocument(documentHandle);
                    var documentNameBlobHandle = document.Name;
                    var documentName = metadataReader.GetString(documentNameBlobHandle);

                    bool embeddedSource = false;
                    var customDebugInformationHandleCollection = metadataReader.GetCustomDebugInformation(documentHandle);
                    foreach (var customDebugInformationHandle in customDebugInformationHandleCollection)
                    {
                        var customDebugInformation = metadataReader.GetCustomDebugInformation(customDebugInformationHandle);
                        if (metadataReader.GetGuid(customDebugInformation.Kind) == EmbeddedSource)
                        {
                            embeddedSource = true;
                        }
                    }

                    customDebugInformationHandleCollection = metadataReader.CustomDebugInformation;

                    string srcSrvString = null;
                    foreach (var customDebugInformationHandle in customDebugInformationHandleCollection)
                    {
                        var customDebugInformation = metadataReader.GetCustomDebugInformation(customDebugInformationHandle);

                        if (metadataReader.GetGuid(customDebugInformation.Kind) == SourceLink)
                        {
                            srcSrvString = Encoding.UTF8.GetString(metadataReader.GetBlobBytes(customDebugInformation.Value));
                        }
                    }

                    var sequencePoints = methodDebugInformation.GetSequencePoints();

                    int lineNumber = -1;
                    int firstLineNumberInSequencePoints = -1;

                    foreach (var sequencePoint in sequencePoints)
                    {
                        if (firstLineNumberInSequencePoints == -1 && !sequencePoint.IsHidden)
                        {
                            firstLineNumberInSequencePoints = sequencePoint.StartLine; // set this in case we need it
                        }

                        if (offset == sequencePoint.Offset && !sequencePoint.IsHidden)
                        {
                            lineNumber = sequencePoint.StartLine;
                        }
                    }

                    if (lineNumber == -1)
                    {
                        lineNumber = firstLineNumberInSequencePoints;
                    }

                    var kind = lineNumber == -1 ? SourceKind.Illegal : SourceKind.None;
                    kind = srcSrvString != null ? SourceKind.Indexed : kind;
                    kind = embeddedSource ? SourceKind.Embedded : kind; /* yeah, kinda weird that embedded takes precedence, in fact it could be indexed and embedded
                                                                           however, if it is embedded likely I want to get it from here because I already have it */

                    return new SourceLocation(new SourceFile(documentName, kind, srcSrvString), lineNumber);
                }
            }
        }

        public bool HasAnyInstructionPointersForProcess(uint pid)
        {
            throw new NotSupportedException();
        }

        public void AddInstructionPointersForProcess(uint pid, ulong imageBase, HashSet<ulong> instructionPointers)
        {
            throw new NotSupportedException();
        }
    }
}
