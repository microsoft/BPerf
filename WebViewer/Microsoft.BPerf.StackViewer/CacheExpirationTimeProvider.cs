// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System;
    using Microsoft.Extensions.Options;

    public sealed class CacheExpirationTimeProvider : ICacheExpirationTimeProvider
    {
        private readonly IOptions<CacheSettings> settings;

        public CacheExpirationTimeProvider(IOptions<CacheSettings> settings)
        {
            this.settings = settings;
        }

        public TimeSpan Expiration => TimeSpan.FromMinutes(this.settings.Value.DurationInMinutes);
    }
}
