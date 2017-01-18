// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "EtwEvent.h"

class RuntimeInformationEvent : public EtwEvent
{
public:
  RuntimeInformationEvent(int16_t clrInstanceId, int16_t sku, int16_t vmMajorVersion, int16_t vmMinorVersion, int16_t vmBuildNumber, int16_t vmQfeNumber);
  void Serialize(Serializer &t) override;
  int32_t GetEventId() override;

private:
  int16_t clrInstanceId;
  int16_t sku;
  int16_t vmMajorVersion;
  int16_t vmMinorVersion;
  int16_t vmBuildNumber;
  int16_t vmQfeNumber;
};