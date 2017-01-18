// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "EtwEvent.h"

class AppDomainUnloadEvent : public EtwEvent
{
public:
  AppDomainUnloadEvent(int64_t appDomainId, int32_t appDomainFlags, portable_wide_string appDomainName, int32_t appDomainIndex, int16_t clrInstanceId);
  void Serialize(Serializer &t) override;
  int32_t GetEventId() override;

private:
  int64_t appDomainId;
  int32_t appDomainFlags;
  portable_wide_string appDomainName;
  int32_t appDomainIndex;
  int16_t clrInstanceId;
};