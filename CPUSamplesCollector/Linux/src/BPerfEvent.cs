// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal struct BPerfEvent
    {
        public ulong TimeStamp;

        public ulong InstructionPointer;

        public ulong ProcessIdThreadId;

        public ulong CGroupId;

        public uint UserStackId;

        public uint KernelStackId;

        public override string ToString()
        {
            return $"Timestamp: {this.TimeStamp}, IP: {this.InstructionPointer:x8}, Process Id: {this.ProcessIdThreadId >> 32}, Thread Id: {(int)this.ProcessIdThreadId}, CGroup Id: {this.CGroupId}, UserStack: {this.UserStackId}, KernelStack: {this.KernelStackId}";
        }
    }
}
