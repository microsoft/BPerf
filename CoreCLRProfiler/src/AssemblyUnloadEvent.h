// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "EtwEvent.h"

class AssemblyUnloadEvent : public EtwEvent
{
public:
  AssemblyUnloadEvent(int64_t assemblyId, int64_t appDomainId, int64_t bindingId, int32_t assemblyFlags, portable_wide_string fullyQualifiedAssemblyName, int16_t clrInstanceId);
  void Serialize(Serializer &t) override;
  int32_t GetEventId() override;

private:
  int64_t assemblyId;
  int64_t appDomainId;
  int64_t bindingId;
  int32_t assemblyFlags;
  portable_wide_string fullyQualifiedAssemblyName;
  int16_t clrInstanceId;
};