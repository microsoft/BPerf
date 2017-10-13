// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    public sealed class ProcessInfo
    {
        public ProcessInfo(string processName, uint pid, long samplesCount)
        {
            this.Name = processName;
            this.Id = pid;
            this.SamplesCount = samplesCount;
        }

        public string Name { get; }

        public uint Id { get; }

        public long SamplesCount { get; }
    }
}
