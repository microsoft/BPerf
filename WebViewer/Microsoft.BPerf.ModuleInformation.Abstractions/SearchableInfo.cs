// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.ModuleInformation.Abstractions
{
    public abstract class SearchableInfo
    {
        public abstract ulong Begin { get; }

        public abstract ulong End { get; set; }
    }
}
