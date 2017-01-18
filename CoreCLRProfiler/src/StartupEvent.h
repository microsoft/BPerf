// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "EtwEvent.h"

class StartupEvent : public EtwEvent
{
public:
  StartupEvent(int16_t clrInstanceId);
  void Serialize(Serializer &t) override;
  int32_t GetEventId() override;

private:
  int16_t clrInstanceId;
  int64_t steadyClockTime;
  int64_t systemClockTime;
};