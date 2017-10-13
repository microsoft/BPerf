// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Abstractions
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.Diagnostics.Tracing.Stacks;

    public sealed class GenericStackSource : StackSource
    {
        private readonly List<StackInfo> stacks;

        private readonly List<Frame> frames;

        private readonly List<string> pseudoFrames;

        private readonly int pseudoFramesStartOffset;

        private readonly List<StackSourceSample> samples;

        private readonly IInstructionPointerToSymbolicNameProvider eipToNameProvider;

        private readonly Action<Action<StackSourceSample>> indirectCallback;

        public GenericStackSource(IStackSamplesProvider samplesProvider, IInstructionPointerToSymbolicNameProvider eipToNameProvider, Action<Action<StackSourceSample>> indirectCallback)
        {
            this.stacks = samplesProvider.Stacks;
            this.frames = samplesProvider.Frames;
            this.pseudoFrames = samplesProvider.PseudoFrames;
            this.pseudoFramesStartOffset = samplesProvider.PseudoFramesStartOffset;
            this.samples = samplesProvider.Samples;
            this.SampleTimeRelativeMSecLimit = samplesProvider.MaxTimeRelativeMSec;

            this.eipToNameProvider = eipToNameProvider;
            this.indirectCallback = indirectCallback;
        }

        public override int CallStackIndexLimit => this.stacks.Count;

        public override int CallFrameIndexLimit => this.frames.Count + (int)StackSourceFrameIndex.Start;

        public override int SampleIndexLimit => this.samples.Count;

        public override double SampleTimeRelativeMSecLimit { get; }

        public override void ForEach(Action<StackSourceSample> callback)
        {
            StackInformationAbstractionsEventSource.Logger.BeginForEachSample();

            this.indirectCallback(callback);

            StackInformationAbstractionsEventSource.Logger.EndForEachSample();
        }

        public override StackSourceCallStackIndex GetCallerIndex(StackSourceCallStackIndex callStackIndex)
        {
            return (StackSourceCallStackIndex)this.stacks[(int)callStackIndex].CallerID;
        }

        public override StackSourceFrameIndex GetFrameIndex(StackSourceCallStackIndex callStackIndex)
        {
            return (StackSourceFrameIndex)this.stacks[(int)callStackIndex].FrameID;
        }

        public override string GetFrameName(StackSourceFrameIndex frameIndex, bool verboseName)
        {
            var frame = this.frames[(int)frameIndex];

            if ((int)frameIndex < this.pseudoFramesStartOffset)
            {
                return this.eipToNameProvider.GetSymbolicName(frame.ProcessId, frame.InstructionPointer);
            }

            return this.pseudoFrames[(int)frame.ProcessId];
        }

        public override StackSourceSample GetSampleByIndex(StackSourceSampleIndex sampleIndex)
        {
            return this.samples[(int)sampleIndex];
        }

        public ValueTask<SourceLocation> GetSourceLocation(StackSourceFrameIndex frameIndex)
        {
            var frame = this.frames[(int)frameIndex];
            return this.eipToNameProvider.GetSourceLocation(frame.ProcessId, frame.InstructionPointer);
        }
    }
}
