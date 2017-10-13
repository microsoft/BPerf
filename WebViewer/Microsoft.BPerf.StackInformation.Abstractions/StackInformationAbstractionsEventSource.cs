// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Abstractions
{
    using System.Diagnostics.Tracing;

    [EventSource(Guid = "c1a88d9c-7990-583d-bdf4-b1f33b471134")]
    internal sealed class StackInformationAbstractionsEventSource : EventSource
    {
        public const EventTask ForEachSample = (EventTask)1;

        public static StackInformationAbstractionsEventSource Logger { get; } = new StackInformationAbstractionsEventSource();

        [Event(1, Opcode = EventOpcode.Start, Task = ForEachSample)]
        public void BeginForEachSample()
        {
            this.WriteEvent(1);
        }

        [Event(2, Opcode = EventOpcode.Stop, Task = ForEachSample)]
        public void EndForEachSample()
        {
            this.WriteEvent(2);
        }
    }
}
