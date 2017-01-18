// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "Serializer.h"

class EtwLogger;

class EtwEvent
{
public:
  virtual ~EtwEvent() = default;
  EtwEvent();
  virtual int32_t GetEventId() = 0;
  virtual void Serialize(Serializer &t);

protected:
  int32_t processId;
  int32_t threadId;
  int64_t timestamp;
};