namespace Microsoft.BPerf.SymbolicInformation.ProgramDatabase
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Reflection.Metadata;
    using System.Reflection.PortableExecutable;
    using System.Runtime.InteropServices;
    using System.Threading.Tasks;
    using Microsoft.BPerf.StackInformation.Abstractions;
    using Microsoft.BPerf.SymbolicInformation.Abstractions;

    public sealed class TracePdbSymbolReaderProvider : ITracePdbSymbolReaderProvider
    {
        private readonly Dictionary<string, IPdbSymbolReader> pdbSymbolReaderMap = new Dictionary<string, IPdbSymbolReader>();

        private readonly Dictionary<DbgId, IPdbSymbolReader> signaturePdbSymbolReaderMap = new Dictionary<DbgId, IPdbSymbolReader>();

        private readonly ISymbolServerArtifactRetriever symbolServerArtifactRetriever;

        public TracePdbSymbolReaderProvider(ISymbolServerArtifactRetriever symbolServerArtifactRetriever)
        {
            this.symbolServerArtifactRetriever = symbolServerArtifactRetriever;
        }

        public async ValueTask<IPdbSymbolReader> GetSymbolReader(string peFilePath)
        {
            if (!this.pdbSymbolReaderMap.TryGetValue(peFilePath, out var pdbSymbolReader))
            {
                // for now just bail on extended path stuff, TODO: need to fix this, need to audit file access APIs
                if (peFilePath.StartsWith(@"\??\"))
                {
                    return null;
                }

                var dbgId = GetDbgIdFromPEFile(peFilePath);
                if (dbgId.HasValue)
                {
                    pdbSymbolReader = await this.GetSymbolReader(peFilePath, dbgId.Value.Signature, dbgId.Value.Age, dbgId.Value.Filename);
                }

                // now add if the file exists on disk or not, so we don't repeated
                this.pdbSymbolReaderMap.Add(peFilePath, pdbSymbolReader);
            }

            return pdbSymbolReader;
        }

        public IPdbSymbolReader GetSymbolReader(Guid signature, uint age, string pdbName)
        {
            if (this.signaturePdbSymbolReaderMap.TryGetValue(new DbgId(signature, age, pdbName), out var pdbSymbolReader))
            {
                return pdbSymbolReader;
            }

            return null;
        }

        public async ValueTask<IPdbSymbolReader> GetSymbolReader(string peFilePath, Guid signature, uint age, string pdbName)
        {
            var sigKey = new DbgId(signature, age, pdbName);
            if (!this.signaturePdbSymbolReaderMap.TryGetValue(sigKey, out var pdbSymbolReader))
            {
                // now check if we can find this pdb adjacent to the dll and obviously validate its guid and age
                string pdbPath = Path.ChangeExtension(peFilePath, ".pdb");

                bool validPdb = false;
                bool validPortablePdb = IsValidPortablePdb(pdbPath, signature);
                if (!validPortablePdb)
                {
                    // now check to see if the filename embedded in the debug info works
                    pdbPath = pdbName;
                    validPortablePdb = IsValidPortablePdb(pdbPath, signature);

                    if (!validPortablePdb)
                    {
                        // now goto the symbol server
                        pdbPath = await this.symbolServerArtifactRetriever.DownloadPortablePdb(Path.GetFileName(pdbName), signature, skipSymbolServer: false);
                        validPortablePdb = pdbPath != null; // convention, we return null on failure from symbol servers

                        if (!validPortablePdb)
                        {
                            // now check if we can find this pdb adjacent to the dll and obviously validate its guid and age
                            pdbPath = Path.ChangeExtension(peFilePath, ".pdb");

                            validPdb = IsValidPdb(pdbPath, signature, age);
                            if (!validPdb)
                            {
                                // now check to see if the filename embedded in the debug info works
                                pdbPath = pdbName;
                                validPdb = IsValidPdb(pdbPath, signature, age);

                                if (!validPdb)
                                {
                                    // now goto the symbol server
                                    pdbPath = await this.symbolServerArtifactRetriever.DownloadPdb(Path.GetFileName(pdbName), signature, age, skipSymbolServer: false);
                                    validPdb = pdbPath != null; // convention, we return null on failure from symbol servers
                                }
                            }
                        }
                    }
                }

                if (validPdb)
                {
                    pdbSymbolReader = new WindowsPdbSymbolReader(pdbPath);
                }
                else if (validPortablePdb)
                {
                    pdbSymbolReader = new PortablePdbSymbolReader(pdbPath);
                }

                this.signaturePdbSymbolReaderMap.Add(sigKey, pdbSymbolReader); // pdbSymbolReader can be null, which means we couldn't find one
            }

            return pdbSymbolReader;
        }

        private static DbgId? GetDbgIdFromPEFile(string peFilePath)
        {
            DbgId? dbgId = null;

            using (var file = File.Open(peFilePath, FileMode.Open, FileAccess.Read))
            {
                using (var portableExecutableReader = new PEReader(file))
                {
                    foreach (var debugDirectory in portableExecutableReader.ReadDebugDirectory())
                    {
                        if (debugDirectory.Type == DebugDirectoryEntryType.CodeView)
                        {
                            var cv = portableExecutableReader.ReadCodeViewDebugDirectoryData(debugDirectory);
                            dbgId = new DbgId(cv.Guid, (uint)cv.Age, cv.Path);
                            break;
                        }
                    }
                }
            }

            return dbgId;
        }

        private static bool IsValidPdb(string pdbPath, Guid signature, uint age)
        {
            // otherwise we'll try to load DIA on non Windows
            if (File.Exists(pdbPath) && RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                var dataSource = DiaLoader.GetDiaSourceObject();
                var local = signature;

                try
                {
                    dataSource.loadAndValidateDataFromPdb(pdbPath, ref local, 0x53445352, age);
                }
                catch (COMException)
                {
                    return false;
                }

                return true;
            }

            return false;
        }

        private static bool IsValidPortablePdb(string pdbPath, Guid signature)
        {
            if (File.Exists(pdbPath))
            {
                try
                {
                    using (var stream = File.Open(pdbPath, FileMode.Open, FileAccess.Read))
                    {
                        using (var metadataReaderProvider = MetadataReaderProvider.FromPortablePdbStream(stream))
                        {
                            var header = metadataReaderProvider.GetMetadataReader().DebugMetadataHeader;
                            var buf = header.Id;

                            var a = buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0];
                            var b = (short)(buf[5] << 8 | buf[4]);
                            var c = (short)(buf[7] << 8 | buf[6]);

                            var guid = new Guid(a, b, c, buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

                            return signature == guid;
                        }
                    }
                }
                catch (Exception)
                {
                    return false;
                }
            }

            return false;
        }
    }
}
