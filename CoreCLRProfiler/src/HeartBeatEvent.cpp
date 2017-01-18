// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "HeartBeatEvent.h"

HeartBeatEvent::HeartBeatEvent() : EtwEvent()
{
}

void HeartBeatEvent::Serialize(Serializer &t)
{
    EtwEvent::Serialize(t);
}

int32_t HeartBeatEvent::GetEventId()
{
    return -1;
}