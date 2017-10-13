// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Abstractions
{
    using System;

    public struct ProcessImageId : IEquatable<ProcessImageId>
    {
        private readonly uint pid;

        private readonly ulong imageBase;

        public ProcessImageId(uint pid, ulong imageBase)
        {
            this.pid = pid;
            this.imageBase = imageBase;
        }

        public bool Equals(ProcessImageId other)
        {
            return this.pid == other.pid && this.imageBase == other.imageBase;
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return ((int)this.pid * 397) ^ ((long)this.imageBase).GetHashCode();
            }
        }

        public override bool Equals(object other)
        {
            return this.Equals((ProcessImageId)other);
        }
    }
}
