// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System.Collections.Generic;
    using Microsoft.BPerf.SymbolServer.Interfaces;
    using Microsoft.Extensions.Options;

    public sealed class SourceServerAuthorizationInformationProvider : ISourceServerAuthorizationInformationProvider
    {
        private readonly IEnumerable<ISourceServerAuthorizationInformation> sourceServers;

        public SourceServerAuthorizationInformationProvider(IOptions<SourceServerAuthorizationInformation> options)
        {
            this.sourceServers = options.Value.SourceServers;
        }

        public string GetAuthorizationHeaderValue(string url)
        {
            foreach (var sourceServer in this.sourceServers)
            {
                if (url.StartsWith(sourceServer.UrlPrefix))
                {
                    return sourceServer.AuthorizationHeader;
                }
            }

            return null;
        }
    }
}
