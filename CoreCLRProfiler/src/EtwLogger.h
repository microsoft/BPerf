// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once
#include "EtwLogger.Generated.h"

#ifdef _WINDOWS

class EtwLogger
{
public:
    static void WriteEvent(AppDomainLoadEvent * data);
    static void WriteEvent(AssemblyLoadEvent * data);
    static void WriteEvent(ModuleLoadEvent * data);
    static void WriteEvent(JITCompilationFinishedEvent * data);
    static void WriteEvent(MethodILToNativeMapEvent * data);
};

void EtwLogger::WriteEvent(AppDomainLoadEvent *data)
{
    EventWriteAppDomainDCEnd_V1(data->appDomainId, data->appDomainFlags, data->appDomainName.c_str(), data->appDomainIndex, data->clrInstanceId);
}

void EtwLogger::WriteEvent(AssemblyLoadEvent *data)
{
    EventWriteAssemblyDCEnd_V1(data->assemblyId, data->appDomainId, data->bindingId, data->assemblyFlags, data->fullyQualifiedAssemblyName.c_str(), data->clrInstanceId);
}

void EtwLogger::WriteEvent(ModuleLoadEvent *data)
{
    EventWriteModuleDCEnd_V2(data->moduleId, data->assemblyId, data->moduleFlags, 0, data->moduleILPath.c_str(), data->moduleNativePath.c_str(), data->clrInstanceId, &data->managedPdbSignature, data->managedPdbAge, data->managedPdbBuildPath.c_str(), &data->nativePdbSignature, data->nativePdbAge, data->nativePdbBuildPath.c_str());
}

void EtwLogger::WriteEvent(JITCompilationFinishedEvent *data)
{
    EventWriteMethodDCEndVerbose_V2(data->methodId, data->moduleId, data->methodStartAddress, data->methodSize, data->methodToken, data->methodFlags, data->methodNamespace.c_str(), data->methodName.c_str(), data->methodSignature.c_str(), data->clrInstanceId, data->rejitId);
}

void EtwLogger::WriteEvent(MethodILToNativeMapEvent *data)
{
    EventWriteMethodDCEndILToNativeMap(data->methodId, data->reJitId, data->methodExtent, data->countOfMapEntries, data->ilOffsets.data(), data->nativeOffsets.data(), data->clrInstanceId);
}

#endif