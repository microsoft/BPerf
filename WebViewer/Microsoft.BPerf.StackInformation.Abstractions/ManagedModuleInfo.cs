// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Abstractions
{
    using System;
    using System.IO;

    public sealed class ManagedModuleInfo
    {
        public ManagedModuleInfo(string moduleILPath, string nativeILPath, Guid managedPdbSignature, uint managedPdbAge, Guid nativePdbSignature, uint nativePdbAge, string managedPdbPath, string nativePdbPath)
        {
            this.ModuleILPath = moduleILPath.ToLowerInvariant();
            this.NativeILPath = nativeILPath.ToLowerInvariant();
            this.ManagedPdbSignature = managedPdbSignature;
            this.ManagedPdbAge = managedPdbAge;
            this.NativePdbSignature = nativePdbSignature;
            this.NativePdbAge = nativePdbAge;
            this.ManagedPdbPath = managedPdbPath;
            this.NativePdbPath = nativePdbPath;

            this.ModuleNamePrefix = Path.GetFileNameWithoutExtension(this.ModuleILPath) + "!"; // TODO: Remove System.IO.*
        }

        public string ModuleNamePrefix { get; }

        public string ModuleILPath { get; }

        public string NativeILPath { get; }

        public Guid ManagedPdbSignature { get; }

        public uint ManagedPdbAge { get; }

        public Guid NativePdbSignature { get; }

        public uint NativePdbAge { get; }

        public string ManagedPdbPath { get; }

        public string NativePdbPath { get; }
    }
}
