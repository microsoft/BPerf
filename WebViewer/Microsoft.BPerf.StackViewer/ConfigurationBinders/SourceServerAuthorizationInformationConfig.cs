// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer.ConfigurationBinders
{
    using Microsoft.Extensions.Configuration;

    internal sealed class SourceServerAuthorizationInformationConfig : ConfigurationBinder<SourceServerAuthorizationInformation>
    {
        public SourceServerAuthorizationInformationConfig(IConfiguration configuration)
            : base(configuration, "SourceServerAuthorizationInformation")
        {
        }
    }
}
