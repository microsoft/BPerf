// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer.ConfigurationBinders
{
    using Microsoft.Extensions.Configuration;
    using Microsoft.Extensions.Options;

    internal abstract class ConfigurationBinder<T> : IOptions<T>
        where T : class, new()
    {
        protected ConfigurationBinder(IConfiguration configuration, string key)
        {
            var obj = new T();
            configuration.GetSection(key).Bind(obj);
            this.Value = obj;
        }

        public T Value { get; }
    }
}
