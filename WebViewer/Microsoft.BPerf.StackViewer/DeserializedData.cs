// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Net.Http;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.BPerf.StackInformation.Abstractions;
    using Microsoft.BPerf.StackInformation.Etw;

    public sealed class DeserializedData : IDeserializedData
    {
        private readonly string uri;

        private readonly FileLocationType locationType;

        private readonly string tempDownloadLocation;

        private readonly SemaphoreSlim semaphoreSlim = new SemaphoreSlim(1, 1);

        private readonly List<StackEventTypeInfo> stackEventTypes = new List<StackEventTypeInfo>();

        private readonly List<ProcessInfo> processList = new List<ProcessInfo>();

        private readonly Dictionary<StackViewerModel, ICallTreeData> callTreeDataCache = new Dictionary<StackViewerModel, ICallTreeData>();

        private int initialized;

        private TraceLogEtwDeserializer deserializer;

        public DeserializedData(string uri, FileLocationType locationType, string tempDownloadLocation)
        {
            this.uri = uri;
            this.locationType = locationType;
            this.tempDownloadLocation = tempDownloadLocation;
        }

        public async ValueTask<List<StackEventTypeInfo>> GetStackEventTypesAsync()
        {
            await this.EnsureInitialized();
            return this.stackEventTypes;
        }

        public async ValueTask<ICallTreeData> GetCallTreeAsync(StackViewerModel model)
        {
            await this.EnsureInitialized();

            lock (this.callTreeDataCache)
            {
                if (!this.callTreeDataCache.TryGetValue(model, out var value))
                {
                    value = new CallTreeData(this.deserializer.StackSource, model);
                    this.callTreeDataCache.Add(model, value);
                }

                return value;
            }
        }

        public async ValueTask<List<ProcessInfo>> GetProcessListAsync()
        {
            await this.EnsureInitialized();
            return this.processList;
        }

        private async Task<string> DownloadFile(string url)
        {
            using (HttpClient client = new HttpClient())
            {
                using (var response = await client.GetAsync(url, HttpCompletionOption.ResponseHeadersRead))
                {
                    using (var streamToReadFrom = await response.Content.ReadAsStreamAsync())
                    {
                        string fileToWriteTo = Path.Combine(this.tempDownloadLocation + Guid.NewGuid() + ".etl");
                        using (Stream streamToWriteTo = File.Open(fileToWriteTo, FileMode.Create))
                        {
                            await streamToReadFrom.CopyToAsync(streamToWriteTo);
                        }

                        return fileToWriteTo;
                    }
                }
            }
        }

        private async Task EnsureInitialized()
        {
            if (Interlocked.CompareExchange(ref this.initialized, 1, comparand: -1) == 0)
            {
                await this.Initialize();
            }
        }

        private async Task Initialize()
        {
            await this.semaphoreSlim.WaitAsync();

            try
            {
                if (this.initialized == 1)
                {
                    return;
                }

                var filePath = this.locationType == FileLocationType.Url ? await this.DownloadFile(this.uri) : this.uri;

                this.deserializer = new TraceLogEtwDeserializer(filePath);

                foreach (var pair in this.deserializer.EventStats)
                {
                    this.stackEventTypes.Add(new StackEventTypeInfo(pair.Key, pair.Value.EventName, pair.Value.Count, pair.Value.StackCount));
                }

                foreach (var pair in this.deserializer.TraceProcesses.OrderByDescending(t => t.CPUMSec))
                {
                    this.processList.Add(new ProcessInfo(pair.Name + $"({pair.ProcessID})", (int)pair.ProcessIndex, pair.CPUMSec));
                }

                this.initialized = 1;
            }
            finally
            {
                this.semaphoreSlim.Release();
            }
        }
    }
}
