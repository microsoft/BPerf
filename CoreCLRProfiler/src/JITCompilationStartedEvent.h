// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "EtwEvent.h"

class JITCompilationStartedEvent : public EtwEvent
{
public:
  JITCompilationStartedEvent(int64_t methodId, int64_t moduleId, int32_t methodToken, int32_t methodILSize, portable_wide_string methodNamespace, portable_wide_string methodName, portable_wide_string methodSignature, int16_t clrInstanceId);
  void Serialize(Serializer &t) override;
  int32_t GetEventId() override;

private:
  int64_t methodId;
  int64_t moduleId;
  int32_t methodToken;
  int32_t methodILSize;
  portable_wide_string methodNamespace;
  portable_wide_string methodName;
  portable_wide_string methodSignature;
  int16_t clrInstanceId;
};