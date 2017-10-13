// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Abstractions
{
    using System.Threading.Tasks;

    public interface IInstructionPointerToSymbolicNameProvider
    {
        string GetSymbolicName(uint pid, ulong eip);

        ValueTask<SourceLocation> GetSourceLocation(uint pid, ulong eip);
    }
}
