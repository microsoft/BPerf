// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Abstractions
{
    using System;
    using System.Collections.Generic;

    public sealed class StackEventType : IEquatable<StackEventType>
    {
        public StackEventType(int eventId, string eventName)
        {
            this.EventId = eventId;
            this.EventName = eventName;
            this.SampleIndices = new List<int>();
        }

        public int EventId { get; }

        public string EventName { get; }

        public List<int> SampleIndices { get; }

        public bool Equals(StackEventType other)
        {
            return this.EventId == other.EventId && string.Equals(this.EventName, other.EventName) && Equals(this.SampleIndices, other.SampleIndices);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = this.EventId;
                hashCode = (hashCode * 397) ^ (this.EventName != null ? this.EventName.GetHashCode() : 0);
                hashCode = (hashCode * 397) ^ (this.SampleIndices != null ? this.SampleIndices.GetHashCode() : 0);
                return hashCode;
            }
        }

        public override bool Equals(object other)
        {
            if (ReferenceEquals(null, other))
            {
                return false;
            }

            if (ReferenceEquals(this, other))
            {
                return true;
            }

            return this.Equals((StackEventType)other);
        }
    }
}
