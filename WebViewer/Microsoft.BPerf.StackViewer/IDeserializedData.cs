// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using global::Diagnostics.Tracing.StackSources;

    public interface IDeserializedData
    {
        ValueTask<List<StackEventTypeInfo>> GetStackEventTypesAsync();

        ValueTask<List<ProcessInfo>> GetProcessListAsync();

        ValueTask<ICallTreeData> GetCallTreeAsync(StackViewerModel model);
    }
}
