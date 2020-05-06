// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.BuildTools
{
    using System;
    using System.IO;
    using System.Reflection;
    using System.Reflection.Metadata;
    using System.Reflection.Metadata.Ecma335;
    using System.Reflection.PortableExecutable;

    internal static class Program
    {
        public static void Main(string[] args)
        {
            if (args.Length != 2)
            {
                Console.WriteLine("Usage: Microsoft.BPerf.BuildTools.exe OutputAssemblyPath PublicKeyBinPath");
            }

            var outputAssemblyPath = args[0];
            var publicKey = File.ReadAllBytes(args[1]);
            CreateInternalCallsAssembly(Path.GetFileName(outputAssemblyPath), Path.GetFileNameWithoutExtension(outputAssemblyPath), new Version(1, 0, 0, 0), outputAssemblyPath, publicKey);
        }

        public static void CreateInternalCallsAssembly(string outputModuleName, string outputAssemblyName, Version version, string outputAssemblyFilePath, byte[] publicKey)
        {
            var metadataBuilder = new MetadataBuilder();
            var publicKeyBlobHandle = metadataBuilder.GetOrAddBlob(publicKey);

            metadataBuilder.AddModule(0, metadataBuilder.GetOrAddString(outputModuleName), metadataBuilder.GetOrAddGuid(Guid.NewGuid()), default, default);
            var assemblyHandle = metadataBuilder.AddAssembly(metadataBuilder.GetOrAddString(outputAssemblyName), version, default, publicKeyBlobHandle, AssemblyFlags.PublicKey, AssemblyHashAlgorithm.Sha1);
            metadataBuilder.AddTypeDefinition(default, default, metadataBuilder.GetOrAddString("<Module>"), default, MetadataTokens.FieldDefinitionHandle(1), MetadataTokens.MethodDefinitionHandle(1));

            var netstandardAssemblyRef = metadataBuilder.AddAssemblyReference(metadataBuilder.GetOrAddString("netstandard"), new Version(2, 0, 0, 0), default, metadataBuilder.GetOrAddBlob(new byte[] { 0xCC, 0x7B, 0x13, 0xFF, 0xCD, 0x2D, 0xDD, 0x51 }), default, default);
            var attributeTypeRef = metadataBuilder.AddTypeReference(netstandardAssemblyRef, metadataBuilder.GetOrAddString("System"), metadataBuilder.GetOrAddString("Attribute"));
            var assemblyNameField = CreateAssemblyNameField(metadataBuilder);
            var systemObjectTypeRef = metadataBuilder.AddTypeReference(netstandardAssemblyRef, metadataBuilder.GetOrAddString("System"), metadataBuilder.GetOrAddString("Object"));

            var codeBuilder = new BlobBuilder();

            var spcAssemblyRef = metadataBuilder.AddAssemblyReference(metadataBuilder.GetOrAddString("System.Private.CoreLib"), new Version(4, 0, 0, 0), default, metadataBuilder.GetOrAddBlob(new byte[] { 0x7C, 0xEC, 0x85, 0xD7, 0xBE, 0xA7, 0x79, 0x8E }), default, default);
            var gcTypeRef = metadataBuilder.AddTypeReference(spcAssemblyRef, metadataBuilder.GetOrAddString("System"), metadataBuilder.GetOrAddString("GC"));
            var exceptionTypeRef = metadataBuilder.AddTypeReference(spcAssemblyRef, metadataBuilder.GetOrAddString("System"), metadataBuilder.GetOrAddString("Exception"));
            var runtimeEventSourceHelperTypeRef = metadataBuilder.AddTypeReference(spcAssemblyRef, metadataBuilder.GetOrAddString("System.Diagnostics.Tracing"), metadataBuilder.GetOrAddString("RuntimeEventSourceHelper"));
            var systemReflectionTypeRef = metadataBuilder.AddTypeReference(spcAssemblyRef, metadataBuilder.GetOrAddString("System.Reflection"), metadataBuilder.GetOrAddString("Assembly"));
            var moduleReferenceHandle = metadataBuilder.AddModuleReference(metadataBuilder.GetOrAddString("Microsoft.BPerf.CoreCLRProfiler"));

            metadataBuilder.AddTypeDefinition(TypeAttributes.Public | TypeAttributes.Abstract | TypeAttributes.AutoLayout | TypeAttributes.AnsiClass | TypeAttributes.Sealed | TypeAttributes.BeforeFieldInit, metadataBuilder.GetOrAddString("Microsoft.BPerf"), metadataBuilder.GetOrAddString("ProfilerInterop"), systemObjectTypeRef, assemblyNameField, MetadataTokens.MethodDefinitionHandle(1));

            CreateGetGenerationSizeMethod(metadataBuilder, codeBuilder, CreateGetGenerationSizeMemberReferenceHandle(metadataBuilder, gcTypeRef));
            CreateNoArgsThunkMethod(metadataBuilder, codeBuilder, CreateNoArgsMemberReferenceHandle(metadataBuilder, runtimeEventSourceHelperTypeRef, "GetCpuUsage", TypeCode.Int32), "GetCpuUsage", TypeCode.Int32);
            CreateNoArgsThunkMethod(metadataBuilder, codeBuilder, CreateNoArgsMemberReferenceHandle(metadataBuilder, exceptionTypeRef, "GetExceptionCount", TypeCode.UInt32), "GetExceptionCount", TypeCode.UInt32);
            CreateNoArgsThunkMethod(metadataBuilder, codeBuilder, CreateNoArgsMemberReferenceHandle(metadataBuilder, gcTypeRef, "GetLastGCPercentTimeInGC", TypeCode.Int32), "GetLastGCPercentTimeInGC", TypeCode.Int32);
            CreateNoArgsThunkMethod(metadataBuilder, codeBuilder, CreateNoArgsMemberReferenceHandle(metadataBuilder, systemReflectionTypeRef, "GetAssemblyCount", TypeCode.UInt32), "GetAssemblyCount", TypeCode.UInt32);

            int methodIndex = 5; // because we've put 5 methods prior to this method.

            CreatePInvokeImpl(metadataBuilder, "GetNumberOfExceptionsThrown", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetNumberOfFiltersExecuted", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetNumberOfFinallysExecuted", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalILBytesJittedForMetadataMethods", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfMetadataMethodsJitted",  moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetCurrentNumberOfMetadataMethodsJitted", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalMachineCodeBytesGeneratedForMetadataMethods", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalILBytesJittedForDynamicMethods", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalMachineCodeBytesGeneratedForDynamicMethods", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfDynamicMethods", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetCurrentNumberOfDynamicMethods", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalCachedMethodsSearched", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalCachedMethodsRestored", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalCachedMethodsMachineCodeBytesRestored", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfRuntimeSuspsensions", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfRuntimeSuspensionsForGC", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfRuntimeSuspensionsForGCPrep", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetCurrentNumberOfClassesLoaded", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfClassesLoaded", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfClassLoadFailures", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetCurrentNumberOfAssembliesLoaded", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfAssembliesLoaded", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetCurrentNumberOfModulesLoaded", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfModulesLoaded", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetCurrentNumberOfLogicalThreads", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfInducedGarbageCollections", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfGen0Collections", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfGen1Collections", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfGen2Collections", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetTotalNumberOfBytesInAllHeaps", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetGen0HeapSize", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetGen1HeapSize", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetGen2HeapSize", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetGen3HeapSize", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetPinnedObjectHeapSize", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetFrozenHeapSize", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetNumberOfGCSegments", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "GetNumberOfFrozenSegments", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "EnableAllocationMonitoring", moduleReferenceHandle, ref methodIndex);
            CreatePInvokeImpl(metadataBuilder, "DisableAllocationMonitoring", moduleReferenceHandle, ref methodIndex);

            var ignoresAccessChecksToAttributeConstructor = CreateIgnoresAccessChecksToAttributeConstructorMethod(metadataBuilder, codeBuilder, CreateAttributeConstructorMemberRef(metadataBuilder, attributeTypeRef), assemblyNameField);
            metadataBuilder.AddCustomAttribute(assemblyHandle, ignoresAccessChecksToAttributeConstructor, metadataBuilder.GetOrAddBlob(new byte[] { 0x01, 0x00, 0x16, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6D, 0x2E, 0x50, 0x72, 0x69, 0x76, 0x61, 0x74, 0x65, 0x2E, 0x43, 0x6F, 0x72, 0x65, 0x4C, 0x69, 0x62, 0x00, 0x00 }));

            var ignoresAccessChecksToAttributeTypeDefinitionHandle = metadataBuilder.AddTypeDefinition(TypeAttributes.Public | TypeAttributes.AutoLayout | TypeAttributes.AnsiClass | TypeAttributes.BeforeFieldInit, metadataBuilder.GetOrAddString("System.Runtime.CompilerServices"), metadataBuilder.GetOrAddString("IgnoresAccessChecksToAttribute"), attributeTypeRef, assemblyNameField, ignoresAccessChecksToAttributeConstructor);
            metadataBuilder.AddCustomAttribute(ignoresAccessChecksToAttributeTypeDefinitionHandle, CreateAttributeUsageAttributeMemberRef(metadataBuilder, netstandardAssemblyRef), metadataBuilder.GetOrAddBlob(new byte[] { 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x54, 0x02, 0x0D, 0x41, 0x6C, 0x6C, 0x6F, 0x77, 0x4D, 0x75, 0x6C, 0x74, 0x69, 0x70, 0x6C, 0x65, 0x01 }));
            metadataBuilder.AddMethodSemantics(CreateIgnoresAccessChecksToAttributeAssemblyNameProperty(metadataBuilder, ignoresAccessChecksToAttributeTypeDefinitionHandle), MethodSemanticsAttributes.Getter, CreateIgnoresAccessChecksToAttributeGetAssemblyNameMethod(metadataBuilder, codeBuilder, assemblyNameField));

            using var fs = new FileStream(outputAssemblyFilePath, FileMode.Create, FileAccess.Write);
            WritePEImage(fs, metadataBuilder, codeBuilder);
        }

        private static MemberReferenceHandle CreateGetGenerationSizeMemberReferenceHandle(MetadataBuilder metadataBuilder, TypeReferenceHandle systemReflectionTypeRef)
        {
            var signatureBuilder = new BlobBuilder();

            new BlobEncoder(signatureBuilder).
                MethodSignature().
                Parameters(1,
                    returnType => returnType.Type().UInt64(),
                    parameters =>
                    {
                        parameters.AddParameter().Type().Int32();
                    });

            return metadataBuilder.AddMemberReference(systemReflectionTypeRef, metadataBuilder.GetOrAddString("GetGenerationSize"), metadataBuilder.GetOrAddBlob(signatureBuilder));
        }

        private static MemberReferenceHandle CreateNoArgsMemberReferenceHandle(MetadataBuilder metadataBuilder, TypeReferenceHandle typeRef, string methodName, TypeCode type)
        {
            var signatureBuilder = new BlobBuilder();

            new BlobEncoder(signatureBuilder).
                MethodSignature().
                Parameters(0,
                    returnType =>
                    {
                        switch (type)
                        {
                            case TypeCode.Int32:
                                returnType.Type().Int32();
                                break;
                            case TypeCode.UInt32:
                                returnType.Type().UInt32();
                                break;
                            case TypeCode.UInt64:
                                returnType.Type().UInt64();
                                break;
                        }
                    },
                    parameters =>
                    {
                    });

            return metadataBuilder.AddMemberReference(typeRef, metadataBuilder.GetOrAddString(methodName), metadataBuilder.GetOrAddBlob(signatureBuilder));
        }

        private static void CreateGetGenerationSizeMethod(MetadataBuilder metadataBuilder, BlobBuilder codeBuilder, MemberReferenceHandle createGetGenerationSizeMemberReferenceHandle)
        {
            codeBuilder.Align(4);

            var ilBuilder = new BlobBuilder();
            var il = new InstructionEncoder(ilBuilder);

            il.LoadArgument(0);
            il.OpCode(ILOpCode.Call);
            il.Token(createGetGenerationSizeMemberReferenceHandle);
            il.OpCode(ILOpCode.Ret);

            var methodBodyStream = new MethodBodyStreamEncoder(codeBuilder);
            int bodyOffset = methodBodyStream.AddMethodBody(il);
            ilBuilder.Clear();

            var signatureBuilder = new BlobBuilder();

            new BlobEncoder(signatureBuilder).
                MethodSignature().
                Parameters(1,
                returnType => returnType.Type().UInt64(),
                parameters =>
                {
                    parameters.AddParameter().Type().Int32();
                });

            metadataBuilder.AddParameter(ParameterAttributes.None, metadataBuilder.GetOrAddString("generation"), 1);
            metadataBuilder.AddMethodDefinition(MethodAttributes.Public | MethodAttributes.HideBySig | MethodAttributes.Static, MethodImplAttributes.IL | MethodImplAttributes.Managed, metadataBuilder.GetOrAddString("GetGenerationSize"), metadataBuilder.GetOrAddBlob(signatureBuilder), bodyOffset, parameterList: MetadataTokens.ParameterHandle(1));
        }

        private static void CreateNoArgsThunkMethod(MetadataBuilder metadataBuilder, BlobBuilder codeBuilder, MemberReferenceHandle memberReferenceHandle, string methodName, TypeCode type)
        {
            codeBuilder.Align(4);

            var ilBuilder = new BlobBuilder();
            var il = new InstructionEncoder(ilBuilder);

            il.OpCode(ILOpCode.Call);
            il.Token(memberReferenceHandle);
            il.OpCode(ILOpCode.Ret);

            var methodBodyStream = new MethodBodyStreamEncoder(codeBuilder);
            int bodyOffset = methodBodyStream.AddMethodBody(il);
            ilBuilder.Clear();

            var signatureBuilder = new BlobBuilder();

            new BlobEncoder(signatureBuilder).
                MethodSignature().
                Parameters(0,
                    returnType =>
                    {
                        switch (type)
                        {
                            case TypeCode.Int32:
                                returnType.Type().Int32();
                                break;
                            case TypeCode.UInt32:
                                returnType.Type().UInt32();
                                break;
                            case TypeCode.UInt64:
                                returnType.Type().UInt64();
                                break;
                        }
                    },
                    parameters =>
                    {
                    });

            metadataBuilder.AddMethodDefinition(MethodAttributes.Public | MethodAttributes.HideBySig | MethodAttributes.Static, MethodImplAttributes.IL | MethodImplAttributes.Managed, metadataBuilder.GetOrAddString(methodName), metadataBuilder.GetOrAddBlob(signatureBuilder), bodyOffset, parameterList: MetadataTokens.ParameterHandle(2));
        }

        private static void CreatePInvokeImpl2(MetadataBuilder metadataBuilder, string methodName)
        {
            var signatureBuilder = new BlobBuilder();

            new BlobEncoder(signatureBuilder).
                MethodSignature().
                Parameters(0,
                    returnType =>
                    {
                        returnType.Type().IntPtr();
                    },
                    parameters =>
                    {
                    });


            metadataBuilder.AddMethodDefinition(MethodAttributes.Public | MethodAttributes.HideBySig | MethodAttributes.Static | MethodAttributes.PinvokeImpl, MethodImplAttributes.IL | MethodImplAttributes.PreserveSig | MethodImplAttributes.Managed, metadataBuilder.GetOrAddString(methodName), metadataBuilder.GetOrAddBlob(signatureBuilder), -1, parameterList: MetadataTokens.ParameterHandle(2));
        }

        private static void CreatePInvokeImpl(MetadataBuilder metadataBuilder, string methodName, ModuleReferenceHandle moduleReferenceHandle, ref int methodRowIndex)
        {
            methodRowIndex++;
            metadataBuilder.AddMethodImport(MetadataTokens.MethodDefinitionHandle(methodRowIndex), MethodImportAttributes.CallingConventionWinApi, metadataBuilder.GetOrAddString(methodName), moduleReferenceHandle);
            CreatePInvokeImpl2(metadataBuilder, methodName);
        }

        private static MemberReferenceHandle CreateAttributeConstructorMemberRef(MetadataBuilder metadataBuilder, TypeReferenceHandle attributeTypeRef)
        {
            var signatureBuilder = new BlobBuilder();

            new BlobEncoder(signatureBuilder).
                MethodSignature(SignatureCallingConvention.Default, 0, true).
                Parameters(0,
                returnType => returnType.Void(),
                parameters =>
                {
                });

            return metadataBuilder.AddMemberReference(attributeTypeRef, metadataBuilder.GetOrAddString(".ctor"), metadataBuilder.GetOrAddBlob(signatureBuilder));
        }

        private static MemberReferenceHandle CreateAttributeUsageAttributeMemberRef(MetadataBuilder metadataBuilder, AssemblyReferenceHandle netstandardAssemblyRef)
        {
            var attributeTargetsTypeRef = metadataBuilder.AddTypeReference(netstandardAssemblyRef, metadataBuilder.GetOrAddString("System"), metadataBuilder.GetOrAddString("AttributeTargets"));
            var attributeUsageAttributeTypeRef = metadataBuilder.AddTypeReference(netstandardAssemblyRef, metadataBuilder.GetOrAddString("System"), metadataBuilder.GetOrAddString("AttributeUsageAttribute"));

            var signatureBuilder = new BlobBuilder();

            new BlobEncoder(signatureBuilder).
                MethodSignature(SignatureCallingConvention.Default, 0, true).
                Parameters(1,
                returnType => returnType.Void(),
                parameters =>
                {
                    parameters.AddParameter().Type().Type(attributeTargetsTypeRef, true);
                });

            return metadataBuilder.AddMemberReference(attributeUsageAttributeTypeRef, metadataBuilder.GetOrAddString(".ctor"), metadataBuilder.GetOrAddBlob(signatureBuilder));
        }

        private static FieldDefinitionHandle CreateAssemblyNameField(MetadataBuilder metadataBuilder)
        {
            var signatureBuilder = new BlobBuilder();

            new BlobEncoder(signatureBuilder).FieldSignature().String();

            return metadataBuilder.AddFieldDefinition(FieldAttributes.Private | FieldAttributes.InitOnly, metadataBuilder.GetOrAddString("assemblyName"), metadataBuilder.GetOrAddBlob(signatureBuilder));
        }

        private static PropertyDefinitionHandle CreateIgnoresAccessChecksToAttributeAssemblyNameProperty(MetadataBuilder metadataBuilder, TypeDefinitionHandle typeDefinitionHandle)
        {
            var signatureBuilder = new BlobBuilder();

            new BlobEncoder(signatureBuilder).
                PropertySignature(true).
                Parameters(0,
                returnType => returnType.Type().String(),
                parameters =>
                {
                });

            var propertyDefinitionHandle = metadataBuilder.AddProperty(PropertyAttributes.None, metadataBuilder.GetOrAddString("AssemblyName"), metadataBuilder.GetOrAddBlob(signatureBuilder));

            metadataBuilder.AddPropertyMap(typeDefinitionHandle, propertyDefinitionHandle);

            return propertyDefinitionHandle;
        }

        private static MethodDefinitionHandle CreateIgnoresAccessChecksToAttributeConstructorMethod(MetadataBuilder metadataBuilder, BlobBuilder codeBuilder, MemberReferenceHandle attributeConstructor, FieldDefinitionHandle assemblyNameField)
        {
            codeBuilder.Align(4);

            var ilBuilder = new BlobBuilder();
            var il = new InstructionEncoder(ilBuilder);

            il.LoadArgument(0);
            il.OpCode(ILOpCode.Dup);
            il.OpCode(ILOpCode.Call);
            il.Token(attributeConstructor);
            il.LoadArgument(1);
            il.OpCode(ILOpCode.Stfld);
            il.Token(assemblyNameField);
            il.OpCode(ILOpCode.Ret);

            var methodBodyStream = new MethodBodyStreamEncoder(codeBuilder);
            int mainBodyOffset = methodBodyStream.AddMethodBody(il);
            ilBuilder.Clear();

            var signatureBuilder = new BlobBuilder();

            new BlobEncoder(signatureBuilder).
                MethodSignature(SignatureCallingConvention.Default, 0, true).
                Parameters(1,
                returnType => returnType.Void(),
                parameters =>
                {
                    parameters.AddParameter().Type().String();
                });

            metadataBuilder.AddParameter(ParameterAttributes.None, metadataBuilder.GetOrAddString("assemblyName"), 1);

            return metadataBuilder.AddMethodDefinition(MethodAttributes.Public | MethodAttributes.HideBySig | MethodAttributes.SpecialName | MethodAttributes.RTSpecialName, MethodImplAttributes.IL | MethodImplAttributes.Managed, metadataBuilder.GetOrAddString(".ctor"), metadataBuilder.GetOrAddBlob(signatureBuilder), mainBodyOffset, parameterList: MetadataTokens.ParameterHandle(2));
        }

        private static MethodDefinitionHandle CreateIgnoresAccessChecksToAttributeGetAssemblyNameMethod(MetadataBuilder metadataBuilder, BlobBuilder codeBuilder, FieldDefinitionHandle assemblyNameField)
        {
            codeBuilder.Align(4);

            var ilBuilder = new BlobBuilder();
            var il = new InstructionEncoder(ilBuilder);

            il.LoadArgument(0);
            il.OpCode(ILOpCode.Ldfld);
            il.Token(assemblyNameField);
            il.OpCode(ILOpCode.Ret);

            var methodBodyStream = new MethodBodyStreamEncoder(codeBuilder);
            int bodyOffset = methodBodyStream.AddMethodBody(il);
            ilBuilder.Clear();

            var signatureBuilder = new BlobBuilder();

            new BlobEncoder(signatureBuilder).
                MethodSignature(SignatureCallingConvention.Default, 0, true).
                Parameters(0,
                returnType => returnType.Type().String(),
                parameters =>
                {
                });

            return metadataBuilder.AddMethodDefinition(MethodAttributes.Public | MethodAttributes.HideBySig | MethodAttributes.SpecialName, MethodImplAttributes.IL | MethodImplAttributes.Managed, metadataBuilder.GetOrAddString("get_AssemblyName"), metadataBuilder.GetOrAddBlob(signatureBuilder), bodyOffset, parameterList: MetadataTokens.ParameterHandle(2));
        }

        private static void WritePEImage(Stream peStream, MetadataBuilder metadataBuilder, BlobBuilder ilBuilder, Blob mvidFixup = default)
        {
            var peBuilder = new ManagedPEBuilder(new PEHeaderBuilder(imageCharacteristics: Characteristics.ExecutableImage | Characteristics.Dll), new MetadataRootBuilder(metadataBuilder), ilBuilder, entryPoint: default, flags: CorFlags.ILOnly | CorFlags.StrongNameSigned);

            var peBlob = new BlobBuilder();

            var contentId = peBuilder.Serialize(peBlob);

            if (!mvidFixup.IsDefault)
            {
                new BlobWriter(mvidFixup).WriteGuid(contentId.Guid);
            }

            peBlob.WriteContentTo(peStream);
        }
    }
}
