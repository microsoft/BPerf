// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Etw
{
    using System.Diagnostics.Tracing;

    [EventSource(Guid = "efebb5e3-ed9c-539a-fdcb-26805e59f87b")]
    internal sealed class EtwDeserializerEventSource : EventSource
    {
        public const EventTask ProcessTrace = (EventTask)1;
        public const EventTask SortImages = (EventTask)2;
        public const EventTask SortMethods = (EventTask)3;
        public const EventTask PopulateInstructionPointers = (EventTask)4;
        public const EventTask FixupCallerIndices = (EventTask)5;
        public const EventTask ProcessIdToNameMap = (EventTask)6;

        public static EtwDeserializerEventSource Logger { get; } = new EtwDeserializerEventSource();

        [Event(1, Opcode = EventOpcode.Start, Task = ProcessTrace)]
        public void BeginProcessTrace()
        {
            this.WriteEvent(1);
        }

        [Event(2, Opcode = EventOpcode.Stop, Task = ProcessTrace)]
        public void EndProcessTrace()
        {
            this.WriteEvent(2);
        }

        [Event(3, Opcode = EventOpcode.Start, Task = SortImages)]
        public void BeginSortImages()
        {
            this.WriteEvent(3);
        }

        [Event(4, Opcode = EventOpcode.Stop, Task = SortImages)]
        public void EndSortImages()
        {
            this.WriteEvent(4);
        }

        [Event(5, Opcode = EventOpcode.Start, Task = SortMethods)]
        public void BeginSortMethods()
        {
            this.WriteEvent(5);
        }

        [Event(6, Opcode = EventOpcode.Stop, Task = SortMethods)]
        public void EndSortMethods()
        {
            this.WriteEvent(6);
        }

        [Event(7, Opcode = EventOpcode.Start, Task = PopulateInstructionPointers)]
        public void BeginPopulateInstructionPointers()
        {
            this.WriteEvent(7);
        }

        [Event(8, Opcode = EventOpcode.Stop, Task = PopulateInstructionPointers)]
        public void EndPopulateInstructionPointers()
        {
            this.WriteEvent(8);
        }

        [Event(9, Opcode = EventOpcode.Start, Task = FixupCallerIndices)]
        public void BeginFixupCallerIndices()
        {
            this.WriteEvent(9);
        }

        [Event(10, Opcode = EventOpcode.Stop, Task = FixupCallerIndices)]
        public void EndFixupCallerIndices()
        {
            this.WriteEvent(10);
        }

        [Event(11, Opcode = EventOpcode.Start, Task = ProcessIdToNameMap)]
        public void BeginMapProcessIdToNames()
        {
            this.WriteEvent(11);
        }

        [Event(12, Opcode = EventOpcode.Stop, Task = ProcessIdToNameMap)]
        public void EndMapProcessIdToNames()
        {
            this.WriteEvent(12);
        }
    }
}
