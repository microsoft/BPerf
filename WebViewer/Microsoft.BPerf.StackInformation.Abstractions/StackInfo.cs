// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Abstractions
{
    using System;

    public struct StackInfo : IEquatable<StackInfo>
    {
        public StackInfo(int callerID, int frameID)
        {
            this.CallerID = callerID;
            this.FrameID = frameID;
        }

        public int CallerID { get; }

        public int FrameID { get; }

        public bool Equals(StackInfo other)
        {
            return this.CallerID == other.CallerID && this.FrameID == other.FrameID;
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return (this.CallerID * 397) ^ this.FrameID;
            }
        }

        public override bool Equals(object other)
        {
            return this.Equals((StackInfo)other);
        }
    }
}
