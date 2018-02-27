// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackAggregation
{
    using System.Collections.Generic;
    using Microsoft.BPerf.ModuleInformation.Abstractions;
    using Microsoft.Diagnostics.Tracing.Stacks;

    // We purposely make the naked properties available, as opposed to providing an abstracted API.
    // Having an API here had a non-trivial perf impact so we decided to just expose the data structures.
    public interface IStackSamplesProvider
    {
        List<StackInfo> Stacks { get; }

        List<Frame> Frames { get; }

        List<StackSourceSample> Samples { get; }

        Dictionary<int, StackEventType> EventStacks { get; }

        int PseudoFramesStartOffset { get; }

        List<string> PseudoFrames { get; }

        double MaxTimeRelativeMSec { get; }
    }
}
