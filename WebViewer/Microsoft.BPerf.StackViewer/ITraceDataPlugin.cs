// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System;
    using Microsoft.Diagnostics.Tracing.Stacks;

    public interface ITraceDataPlugin
    {
        string Type { get; }

        Func<CallTreeNodeBase, bool> SummaryPredicate { get; }

        StackSource GetStackSource();
    }
}