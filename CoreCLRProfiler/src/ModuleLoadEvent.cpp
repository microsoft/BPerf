// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "ModuleLoadEvent.h"

ModuleLoadEvent::ModuleLoadEvent(int64_t moduleId, int64_t assemblyId, int32_t moduleFlags, portable_wide_string moduleILPath, portable_wide_string moduleNativePath, int16_t clrInstanceId, GUID managedPdbSignature, int32_t managedPdbAge, portable_wide_string managedPdbBuildPath, GUID nativePdbSignature, int32_t nativePdbAge, portable_wide_string nativePdbBuildPath) : EtwEvent(), moduleId(moduleId), assemblyId(assemblyId), moduleFlags(moduleFlags), moduleILPath(std::move(moduleILPath)), moduleNativePath(std::move(moduleNativePath)), clrInstanceId(clrInstanceId), managedPdbSignature(managedPdbSignature), managedPdbAge(managedPdbAge), managedPdbBuildPath(std::move(managedPdbBuildPath)), nativePdbSignature(nativePdbSignature), nativePdbAge(nativePdbAge), nativePdbBuildPath(std::move(nativePdbBuildPath))
{
}

void ModuleLoadEvent::Serialize(Serializer &t)
{
    EtwEvent::Serialize(t);

    t.Write(this->moduleId);
    t.Write(this->assemblyId);
    t.Write(this->moduleFlags);
    t.Write(0); // Reserved1
    t.Write(this->moduleILPath);
    t.Write(this->moduleNativePath);
    t.Write(this->clrInstanceId);
    t.Write(this->managedPdbSignature);
    t.Write(this->managedPdbAge);
    t.Write(this->managedPdbBuildPath);
    t.Write(this->nativePdbSignature);
    t.Write(this->nativePdbAge);
    t.Write(this->nativePdbBuildPath);
}

int32_t ModuleLoadEvent::GetEventId()
{
    return 9;
}