// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System;
    using System.Diagnostics.Tracing;
    using Microsoft.Extensions.Caching.Memory;
    using Microsoft.Extensions.Options;

    public class DeserializedDataCache : IDeserializedDataCache
    {
        private readonly CallTreeDataCache cache;

        private readonly ICacheExpirationTimeProvider cacheExpirationTimeProvider;

        private readonly IOptions<StackViewerSettings> stackViewerSettings;

        public DeserializedDataCache(CallTreeDataCache cache, ICacheExpirationTimeProvider cacheExpirationTimeProvider, IOptions<StackViewerSettings> stackViewerSettings)
        {
            this.cache = cache;
            this.cacheExpirationTimeProvider = cacheExpirationTimeProvider;
            this.stackViewerSettings = stackViewerSettings;
        }

        public void ClearAllCacheEntries()
        {
            this.cache.Compact(100);
        }

        public IDeserializedData GetData(StackViewerModel model)
        {
            lock (this.cache)
            {
                if (!this.cache.TryGetValue(model.Filename, out IDeserializedData data))
                {
                    var cacheEntryOptions = new MemoryCacheEntryOptions().SetPriority(CacheItemPriority.NeverRemove).RegisterPostEvictionCallback(callback: EvictionCallback, state: this).SetSlidingExpiration(this.cacheExpirationTimeProvider.Expiration);
                    data = new DeserializedData(model.Filename, model.LocationType, this.stackViewerSettings.Value.TemporaryDataFileDownloadLocation);
                    this.cache.Set(model.Filename, data, cacheEntryOptions);
                    CacheMonitorEventSource.Logger.CacheEntryAdded(Environment.MachineName, model.Filename);
                }

                return data;
            }
        }

        private static void EvictionCallback(object key, object value, EvictionReason reason, object state)
        {
            CacheMonitorEventSource.Logger.CacheEntryRemoved(Environment.MachineName, (string)key);
        }

        [EventSource(Guid = "203010e5-cae2-5761-b597-a757ae66787b")]
        private sealed class CacheMonitorEventSource : EventSource
        {
            public static CacheMonitorEventSource Logger { get; } = new CacheMonitorEventSource();

            public void CacheEntryAdded(string source, string cacheKey)
            {
                this.WriteEvent(1, source, cacheKey);
            }

            public void CacheEntryRemoved(string source, string cacheKey)
            {
                this.WriteEvent(2, source, cacheKey);
            }
        }
    }
}
