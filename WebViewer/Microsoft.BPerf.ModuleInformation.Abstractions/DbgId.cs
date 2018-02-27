// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.ModuleInformation.Abstractions
{
    using System;

    public struct DbgId : IEquatable<DbgId>
    {
        public DbgId(Guid signature, uint age, string filename)
        {
            this.Signature = signature;
            this.Age = age;
            this.Filename = filename;
        }

        public Guid Signature { get; }

        public uint Age { get; }

        public string Filename { get; }

        public bool Equals(DbgId other)
        {
            return this.Signature.Equals(other.Signature) && this.Age == other.Age && string.Equals(this.Filename, other.Filename);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = this.Signature.GetHashCode();
                hashCode = (hashCode * 397) ^ (int)this.Age;
                hashCode = (hashCode * 397) ^ (this.Filename != null ? this.Filename.GetHashCode() : 0);
                return hashCode;
            }
        }

        public override bool Equals(object other)
        {
            return this.Equals((DbgId)other);
        }
    }
}
