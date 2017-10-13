// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Abstractions
{
    public sealed class ManagedMethodILToNativeMapping
    {
        public ManagedMethodILToNativeMapping(ulong methodId, int[] iloffsets, int[] nativeOffsets)
        {
            this.MethodId = methodId;
            this.ILOffsets = iloffsets;
            this.NativeOffsets = nativeOffsets;
        }

        public ulong MethodId { get; }

        public int[] ILOffsets { get; }

        public int[] NativeOffsets { get; }
    }
}
