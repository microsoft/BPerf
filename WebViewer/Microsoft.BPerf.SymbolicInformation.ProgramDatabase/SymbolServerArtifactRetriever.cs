// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolicInformation.ProgramDatabase
{
    using System;
    using System.Diagnostics.Tracing;
    using System.IO;
    using System.Linq;
    using System.Management.Automation.Internal;
    using System.Net;
    using System.Net.Http;
    using System.Reflection.Metadata;
    using System.Threading.Tasks;
    using Microsoft.BPerf.SymbolServer.Interfaces;

    public sealed class SymbolServerArtifactRetriever : ISymbolServerArtifactRetriever
    {
        private readonly ISymbolServerInformation symbolServerInformation;

        public SymbolServerArtifactRetriever(ISymbolServerInformation symbolServerInformation)
        {
            this.symbolServerInformation = symbolServerInformation;
        }

        public async ValueTask<string> DownloadPdb(string pdbName, Guid signature, uint age, bool skipSymbolServer)
        {
            var key = signature.ToString("N") + age;

            SymbolServerArtifactRetrieverEventSource.Logger.BeginPdbDownload(pdbName, key);

            var canonicalPdbFileName = Path.GetFileName(pdbName); // because sometimes full path is baked in
            var downloadDir = Path.Combine(this.symbolServerInformation.DownloadLocation, "Symbols", canonicalPdbFileName, key);
            Directory.CreateDirectory(downloadDir);
            var downloadPath = Path.Combine(downloadDir, canonicalPdbFileName);

            // if we had this file on disk somehow and it is a valid pdb, use it.
            if (File.Exists(downloadPath) && IsValidPdb(downloadPath, signature, age))
            {
                SymbolServerArtifactRetrieverEventSource.Logger.EndPdbDownload(pdbName, key);
                return downloadPath;
            }

            if (skipSymbolServer)
            {
                return null;
            }

            var fullKey = $"{canonicalPdbFileName}/{key}/{canonicalPdbFileName}";
            var fullCompressedKey = $"{canonicalPdbFileName}/{key}/{Path.GetFileNameWithoutExtension(canonicalPdbFileName)}.pd_";

            SymbolServerArtifactRetrieverEventSource.Logger.BeginSymbolServerLookups();

            foreach (var symbolServer in this.symbolServerInformation.SymbolServers.OrderBy(t => t.Priority))
            {
                SymbolServerArtifactRetrieverEventSource.Logger.BeginSymbolServerRequest(symbolServer.Url, fullKey);

                // download and verify using the regular mechanism
                if (await this.DownloadPdbFromSymbolServer(symbolServer, fullKey, downloadPath) && IsValidPdb(downloadPath, signature, age))
                {
                    SymbolServerArtifactRetrieverEventSource.Logger.EndSymbolServerRequest(symbolServer.Url, fullKey);
                    SymbolServerArtifactRetrieverEventSource.Logger.EndSymbolServerLookups();
                    SymbolServerArtifactRetrieverEventSource.Logger.EndPdbDownload(pdbName, key);

                    return downloadPath;
                }

                SymbolServerArtifactRetrieverEventSource.Logger.EndSymbolServerRequest(symbolServer.Url, fullKey);
                SymbolServerArtifactRetrieverEventSource.Logger.BeginSymbolServerRequest(symbolServer.Url, fullCompressedKey);

                // now test the compressed case
                var pdcompressedDownloadPath = Path.Combine(downloadDir, Path.GetFileNameWithoutExtension(canonicalPdbFileName) + ".pd_");

                if (await this.DownloadPdbFromSymbolServer(symbolServer, fullCompressedKey, pdcompressedDownloadPath) &&
                    CabinetExtractorFactory.GetCabinetExtractor().Extract(string.Empty, pdcompressedDownloadPath, downloadDir) &&
                    IsValidPdb(downloadPath, signature, age))
                {
                    SymbolServerArtifactRetrieverEventSource.Logger.EndSymbolServerRequest(symbolServer.Url, fullCompressedKey);
                    SymbolServerArtifactRetrieverEventSource.Logger.EndSymbolServerLookups();
                    SymbolServerArtifactRetrieverEventSource.Logger.EndPdbDownload(pdbName, key);
                    return downloadPath;
                }

                SymbolServerArtifactRetrieverEventSource.Logger.EndSymbolServerRequest(symbolServer.Url, fullCompressedKey);
            }

            SymbolServerArtifactRetrieverEventSource.Logger.EndSymbolServerLookups();
            SymbolServerArtifactRetrieverEventSource.Logger.EndPdbDownload(pdbName, key);

            return null;
        }

        public async ValueTask<string> DownloadPortablePdb(string pdbName, Guid signature, bool skipSymbolServer)
        {
            var key = signature.ToString("N") + "FFFFFFFF";

            SymbolServerArtifactRetrieverEventSource.Logger.BeginPdbDownload(pdbName, key);

            var canonicalPdbFileName = Path.GetFileName(pdbName);
            var downloadDir = Path.Combine(this.symbolServerInformation.DownloadLocation, "Symbols", canonicalPdbFileName, key);
            Directory.CreateDirectory(downloadDir);
            var downloadPath = Path.Combine(downloadDir, canonicalPdbFileName);

            // if we had this file on disk somehow and it is a valid pdb, use it.
            if (File.Exists(downloadPath) && IsValidPortablePdb(downloadPath, signature))
            {
                SymbolServerArtifactRetrieverEventSource.Logger.EndPdbDownload(pdbName, key);
                return downloadPath;
            }

            if (skipSymbolServer)
            {
                return null;
            }

            var fullKey = $"{canonicalPdbFileName}/{key}/{canonicalPdbFileName}";

            SymbolServerArtifactRetrieverEventSource.Logger.BeginSymbolServerLookups();

            foreach (var symbolServer in this.symbolServerInformation.SymbolServers.OrderBy(t => t.Priority))
            {
                SymbolServerArtifactRetrieverEventSource.Logger.BeginSymbolServerRequest(symbolServer.Url, fullKey);

                // download and verify using the regular mechanism
                if (await this.DownloadPortablePdbFromSymbolServer(symbolServer, fullKey, downloadPath) && IsValidPortablePdb(downloadPath, signature))
                {
                    SymbolServerArtifactRetrieverEventSource.Logger.EndSymbolServerRequest(symbolServer.Url, fullKey);
                    SymbolServerArtifactRetrieverEventSource.Logger.EndSymbolServerLookups();
                    SymbolServerArtifactRetrieverEventSource.Logger.EndPdbDownload(pdbName, key);

                    return downloadPath;
                }

                SymbolServerArtifactRetrieverEventSource.Logger.EndSymbolServerRequest(symbolServer.Url, fullKey);
            }

            SymbolServerArtifactRetrieverEventSource.Logger.EndSymbolServerLookups();
            SymbolServerArtifactRetrieverEventSource.Logger.EndPdbDownload(pdbName, key);

            return null;
        }

        private static ValueTask<bool> DownloadFileFromUrl(ISymbolServer symbolServer, string key, string pdbPath)
        {
            return Download(GetHttpClient(symbolServer), symbolServer.Url + "/" + key, pdbPath);
        }

        private static HttpClient GetHttpClient(ISymbolServer symbolServer)
        {
            var client = new HttpClient(new HttpClientHandler { AllowAutoRedirect = false })
            {
                BaseAddress = new Uri(symbolServer.Url)
            };

            if (!string.IsNullOrEmpty(symbolServer.AuthorizationHeader))
            {
                client.DefaultRequestHeaders.Add("Authorization", symbolServer.AuthorizationHeader);
            }

            return client;
        }

        private static async ValueTask<bool> Download(HttpClient client, string downloadUrl, string localPath)
        {
            try
            {
                using (HttpResponseMessage response = await client.GetAsync(downloadUrl, HttpCompletionOption.ResponseHeadersRead))
                {
                    if (response.StatusCode == HttpStatusCode.NotFound)
                    {
                        return false;
                    }

                    if (response.StatusCode == HttpStatusCode.Unauthorized)
                    {
                        return false;
                    }

                    if (response.StatusCode == HttpStatusCode.Found && response.Headers.Location != null)
                    {
                        var client2 = new HttpClient();
                        return await Download(client2, response.Headers.Location.AbsoluteUri, localPath);
                    }

                    response.EnsureSuccessStatusCode();

                    using (Stream contentStream = await response.Content.ReadAsStreamAsync(), fileStream = new FileStream(localPath, FileMode.Create, FileAccess.Write, FileShare.None, 8192, true))
                    {
                        var buffer = new byte[8192];
                        var isMoreToRead = true;

                        do
                        {
                            var read = await contentStream.ReadAsync(buffer, 0, buffer.Length);
                            if (read == 0)
                            {
                                isMoreToRead = false;
                            }
                            else
                            {
                                await fileStream.WriteAsync(buffer, 0, read);
                            }
                        }
                        while (isMoreToRead);
                    }
                }
            }
            catch (Exception)
            {
                return false;
            }

            return true;
        }

        private static bool IsValidPdb(string pdbPath, Guid incomingSignature, uint incomingAge)
        {
            try
            {
                var dataSource = DiaLoader.GetDiaSourceObject();
                var local = incomingSignature;
                dataSource.loadAndValidateDataFromPdb(pdbPath, ref local, 0x53445352, incomingAge);
            }
            catch (System.Runtime.InteropServices.COMException)
            {
                return false;
            }

            return true;
        }

        private static bool IsValidPortablePdb(string pdbPath, Guid signature)
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

        private ValueTask<bool> DownloadPdbFromSymbolServer(ISymbolServer symbolServer, string key, string downloadPath)
        {
            return DownloadFileFromUrl(symbolServer, key, downloadPath);
        }

        private ValueTask<bool> DownloadPortablePdbFromSymbolServer(ISymbolServer symbolServer, string key, string downloadPath)
        {
            return DownloadFileFromUrl(symbolServer, key, downloadPath);
        }

        [EventSource(Guid = "8a2f3c75-e9a9-5e4e-5814-5734dfe832e8")]
        private class SymbolServerArtifactRetrieverEventSource : EventSource
        {
            public const EventTask PdbDownload = (EventTask)1;
            public const EventTask SymbolServerLookups = (EventTask)2;
            public const EventTask SymbolServerRequest = (EventTask)3;

            public static SymbolServerArtifactRetrieverEventSource Logger { get; } = new SymbolServerArtifactRetrieverEventSource();

            [Event(1, Opcode = EventOpcode.Start, Task = PdbDownload)]
            public void BeginPdbDownload(string pdbName, string key)
            {
                this.WriteEvent(1, pdbName, key);
            }

            [Event(2, Opcode = EventOpcode.Stop, Task = PdbDownload)]

            public void EndPdbDownload(string pdbName, string key)
            {
                this.WriteEvent(2, pdbName, key);
            }

            [Event(3, Opcode = EventOpcode.Start, Task = SymbolServerLookups)]
            public void BeginSymbolServerLookups()
            {
                this.WriteEvent(3);
            }

            [Event(4, Opcode = EventOpcode.Stop, Task = SymbolServerLookups)]
            public void EndSymbolServerLookups()
            {
                this.WriteEvent(4);
            }

            [Event(5, Opcode = EventOpcode.Start, Task = SymbolServerRequest)]
            public void BeginSymbolServerRequest(string url, string key)
            {
                this.WriteEvent(5, url, key);
            }

            [Event(6, Opcode = EventOpcode.Stop, Task = SymbolServerRequest)]
            public void EndSymbolServerRequest(string url, string key)
            {
                this.WriteEvent(6, url, key);
            }
        }
    }
}
