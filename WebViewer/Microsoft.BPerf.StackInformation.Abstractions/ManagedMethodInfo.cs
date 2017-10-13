// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Abstractions
{
    using System;

    public sealed class ManagedMethodInfo : SearchableInfo, IComparable<ManagedMethodInfo>
    {
        public ManagedMethodInfo(uint methodToken, ulong methodId, ulong moduleId, ulong begin, ulong end, string methodName)
        {
            this.MethodToken = methodToken;
            this.MethodID = methodId;
            this.ModuleID = moduleId;
            this.Begin = begin;
            this.End = end;
            this.MethodName = methodName;
        }

        public uint MethodToken { get; }

        public ulong MethodID { get; }

        public ulong ModuleID { get; }

        public override ulong Begin { get; }

        public override ulong End { get; set; }

        public string MethodName { get; }

        public int CompareTo(ManagedMethodInfo other)
        {
            if (this.Begin == other.Begin)
            {
                return 0;
            }

            return this.Begin < other.Begin ? -1 : 1;
        }
    }
}
