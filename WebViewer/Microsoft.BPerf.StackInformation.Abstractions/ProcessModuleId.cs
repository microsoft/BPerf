// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Abstractions
{
    using System;

    public struct ProcessModuleId : IEquatable<ProcessModuleId>
    {
        private readonly uint pid;

        private readonly ulong moduleId;

        public ProcessModuleId(uint pid, ulong moduleId)
        {
            this.pid = pid;
            this.moduleId = moduleId;
        }

        public bool Equals(ProcessModuleId other)
        {
            return this.pid == other.pid && this.moduleId == other.moduleId;
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return ((int)this.pid * 397) ^ ((long)this.moduleId).GetHashCode();
            }
        }

        public override bool Equals(object other)
        {
            return this.Equals((ProcessModuleId)other);
        }
    }
}
