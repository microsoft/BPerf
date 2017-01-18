// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "EtwEvent.h"

class ModuleUnloadEvent : public EtwEvent
{
public:
  ModuleUnloadEvent(int64_t moduleId, int64_t assemblyId, int32_t moduleFlags, portable_wide_string moduleILPath, portable_wide_string moduleNativePath, int16_t clrInstanceId, GUID managedPdbSignature, int32_t managedPdbAge, portable_wide_string managedPdbBuildPath, GUID nativePdbSignature, int32_t nativePdbAge, portable_wide_string nativePdbBuildPath);
  void Serialize(Serializer &t) override;
  int32_t GetEventId() override;

private:
  int64_t moduleId;
  int64_t assemblyId;
  int32_t moduleFlags;
  portable_wide_string moduleILPath;
  portable_wide_string moduleNativePath;
  int16_t clrInstanceId;
  GUID managedPdbSignature;
  int32_t managedPdbAge;
  portable_wide_string managedPdbBuildPath;
  GUID nativePdbSignature;
  int32_t nativePdbAge;
  portable_wide_string nativePdbBuildPath;
};