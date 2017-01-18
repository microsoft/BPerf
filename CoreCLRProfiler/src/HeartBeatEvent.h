// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "EtwEvent.h"

class HeartBeatEvent : public EtwEvent
{
public:
  HeartBeatEvent();
  void Serialize(Serializer &t) override;
  int32_t GetEventId() override;
};