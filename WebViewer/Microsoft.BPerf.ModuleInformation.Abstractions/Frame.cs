// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.ModuleInformation.Abstractions
{
    using System;

    public struct Frame : IEquatable<Frame>
    {
        public Frame(uint pid, ulong eip)
        {
            this.ProcessId = pid;
            this.InstructionPointer = eip;
        }

        public uint ProcessId { get; }

        public ulong InstructionPointer { get; }

        public bool Equals(Frame other)
        {
            return this.ProcessId == other.ProcessId && this.InstructionPointer == other.InstructionPointer;
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return ((int)this.ProcessId * 397) ^ this.InstructionPointer.GetHashCode();
            }
        }

        public override bool Equals(object other)
        {
            return this.Equals((Frame)other);
        }
    }
}
