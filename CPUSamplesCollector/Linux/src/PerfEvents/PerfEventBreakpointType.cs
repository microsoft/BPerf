// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;

    [Flags]
    internal enum PerfEventBreakpointType : uint
    {
        HW_BREAKPOINT_EMPTY = 0,
        HW_BREAKPOINT_R = 1,
        HW_BREAKPOINT_W = 2,
        HW_BREAKPOINT_RW = HW_BREAKPOINT_R | HW_BREAKPOINT_W,
        HW_BREAKPOINT_X = 4,
        HW_BREAKPOINT_INVALID = HW_BREAKPOINT_RW | HW_BREAKPOINT_X,
    }
}
