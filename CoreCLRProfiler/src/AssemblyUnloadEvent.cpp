// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "AssemblyUnloadEvent.h"

AssemblyUnloadEvent::AssemblyUnloadEvent(int64_t assemblyId, int64_t appDomainId, int64_t bindingId, int32_t assemblyFlags, portable_wide_string fullyQualifiedAssemblyName, int16_t clrInstanceId) : EtwEvent(), assemblyId(assemblyId), appDomainId(appDomainId), bindingId(bindingId), assemblyFlags(assemblyFlags), fullyQualifiedAssemblyName(std::move(fullyQualifiedAssemblyName)), clrInstanceId(clrInstanceId)
{
}

void AssemblyUnloadEvent::Serialize(Serializer &t)
{
    EtwEvent::Serialize(t);

    t.Write(this->assemblyId);
    t.Write(this->appDomainId);
    t.Write(this->bindingId);
    t.Write(this->assemblyFlags);
    t.Write(this->fullyQualifiedAssemblyName);
    t.Write(this->clrInstanceId);
}

int32_t AssemblyUnloadEvent::GetEventId()
{
    return 4;
}