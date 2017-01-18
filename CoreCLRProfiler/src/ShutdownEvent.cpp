// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "ShutdownEvent.h"

ShutdownEvent::ShutdownEvent(int16_t clrInstanceId) : EtwEvent(), clrInstanceId(clrInstanceId)
{
}

void ShutdownEvent::Serialize(Serializer &t)
{
    EtwEvent::Serialize(t);

    t.Write(this->clrInstanceId);
}

int32_t ShutdownEvent::GetEventId()
{
    return 0;
}