// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "EtwEvent.h"

class MethodILToNativeMapEvent : public EtwEvent
{
  friend class EtwLogger;

public:
  MethodILToNativeMapEvent(int64_t methodId, int64_t reJitId, int8_t methodExtent, int16_t countOfMapEntries, std::vector<uint32_t> ilOffsets, std::vector<uint32_t> nativeOffsets, int16_t clrInstanceId);
  void Serialize(Serializer &t) override;
  int32_t GetEventId() override;

private:
  int64_t methodId;
  int64_t reJitId;
  int8_t methodExtent;
  int16_t countOfMapEntries;
  std::vector<uint32_t> ilOffsets;
  std::vector<uint32_t> nativeOffsets;
  int16_t clrInstanceId;
};