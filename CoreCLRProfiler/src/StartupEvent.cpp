// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "StartupEvent.h"
#include <chrono>

StartupEvent::StartupEvent(int16_t clrInstanceId) : EtwEvent(), clrInstanceId(clrInstanceId), steadyClockTime(std::chrono::steady_clock::now().time_since_epoch().count()), systemClockTime(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
{
}

void StartupEvent::Serialize(Serializer &t)
{
    EtwEvent::Serialize(t);

    t.Write(this->clrInstanceId);
    t.Write(this->steadyClockTime);
    t.Write(this->systemClockTime);
}

int32_t StartupEvent::GetEventId()
{
    return 12;
}