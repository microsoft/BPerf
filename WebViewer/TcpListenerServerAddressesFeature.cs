// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the MIT Licensee. See LICENSE in the project root for license information.

namespace BPerfViewer
{
    using System.Collections.Generic;
    using Microsoft.AspNetCore.Hosting.Server.Features;

    public sealed class TcpListenerServerAddressesFeature : IServerAddressesFeature
    {
        public ICollection<string> Addresses { get; }
    }
}