// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "EtwEvent.h"

class MethodUnloadEvent : public EtwEvent
{
public:
  MethodUnloadEvent(int64_t methodId, int64_t moduleId, int64_t methodStartAddress, int32_t methodSize, int32_t methodToken, int32_t methodFlags, portable_wide_string methodNamespace, portable_wide_string methodName, portable_wide_string methodSignature, int16_t clrInstanceId, int64_t rejitId);
  void Serialize(Serializer &t) override;
  int32_t GetEventId() override;

private:
  int64_t methodId;
  int64_t moduleId;
  int64_t methodStartAddress;
  int32_t methodSize;
  int32_t methodToken;
  int32_t methodFlags;
  portable_wide_string methodNamespace;
  portable_wide_string methodName;
  portable_wide_string methodSignature;
  int16_t clrInstanceId;
  int64_t rejitId;
};