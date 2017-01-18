// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "RuntimeInformationEvent.h"

RuntimeInformationEvent::RuntimeInformationEvent(int16_t clrInstanceId, int16_t sku, int16_t vmMajorVersion, int16_t vmMinorVersion, int16_t vmBuildNumber, int16_t vmQfeNumber) : EtwEvent(), clrInstanceId(clrInstanceId), sku(sku), vmMajorVersion(vmMajorVersion), vmMinorVersion(vmMinorVersion), vmBuildNumber(vmBuildNumber), vmQfeNumber(vmQfeNumber)
{
}

void RuntimeInformationEvent::Serialize(Serializer &t)
{
    EtwEvent::Serialize(t);

    t.Write(this->clrInstanceId);
    t.Write(this->sku);
    t.Write(4);
    t.Write(0);
    t.Write(0);
    t.Write(0);
    t.Write(this->vmMajorVersion);
    t.Write(this->vmMinorVersion);
    t.Write(this->vmBuildNumber);
    t.Write(this->vmQfeNumber);
    t.WriteNull();
    t.WriteNull();
    t.WriteNull();
    t.WriteNull();
    t.WriteNull();
}

int32_t RuntimeInformationEvent::GetEventId()
{
    return 11;
}