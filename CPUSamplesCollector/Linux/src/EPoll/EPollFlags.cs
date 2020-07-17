// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;

    [Flags]
    internal enum EPollFlags
    {
        EPOLLIN = 0x001,
        EPOLLPRI = 0x002,
        EPOLLOUT = 0x004,
        EPOLLRDNORM = 0x040,
        EPOLLRDBAND = 0x080,
        EPOLLWRNORM = 0x100,
        EPOLLWRBAND = 0x200,
        EPOLLMSG = 0x400,
        EPOLLERR = 0x008,
        EPOLLHUP = 0x010,
        EPOLLRDHUP = 0x2000,
        EPOLLONESHOT = 1 << 30,
        EPOLLET = 1 << 31,
    }
}
