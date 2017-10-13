// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Abstractions
{
    using System.Collections.Generic;

    // We purposely make the naked properties available, as opposed to providing an abstracted API.
    // Having an API here had a non-trivial perf impact so we decided to just expose the data structures.
    public interface IImageInformationProvider
    {
        Dictionary<ulong, ManagedMethodILToNativeMapping> MethodILNativeMap { get; }

        Dictionary<uint, List<ManagedMethodInfo>> MethodLoadMap { get; }

        Dictionary<uint, List<ImageInfo>> ImageLoadMap { get; }

        Dictionary<ProcessModuleId, ManagedModuleInfo> ManagedModuleInfoMap { get; }

        Dictionary<ProcessImageId, DbgId> ImageToDebugInfoMap { get; }
    }
}
