// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Etw
{
    using System.Collections.Generic;
    using Microsoft.BPerf.StackAggregation;
    using Microsoft.Diagnostics.Tracing.Etlx;
    using Microsoft.Diagnostics.Tracing.Stacks;

    public sealed class TraceLogEtwDeserializer
    {
        public TraceLogEtwDeserializer(string etlFileName)
        {
            var traceLog = new TraceLog(TraceLog.CreateFromEventTraceLogFile(etlFileName));
            this.StackSource = new SourceAwareStackSource(new TraceEventStackSource(traceLog.Events));
            this.EventStats = new Dictionary<int, TraceEventCounts>(traceLog.Stats.Count);
            this.TraceProcesses = traceLog.Processes;

            int i = 0;
            foreach (var eventStat in traceLog.Stats)
            {
                this.EventStats.Add(i, eventStat);
                i++;
            }
        }

        public Dictionary<int, TraceEventCounts> EventStats { get; }

        public TraceProcesses TraceProcesses { get; }

        public GenericStackSource StackSource { get; }
    }
}
