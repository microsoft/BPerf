// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <cstdint>
#include <vector>
#include "PortableString.h"
#include "GuidDef.h"

class Serializer
{
public:
  virtual ~Serializer() = default;
  virtual void Write(int8_t value) = 0;
  virtual void Write(int16_t value) = 0;
  virtual void Write(int32_t value) = 0;
  virtual void Write(uint32_t value) = 0;
  virtual void Write(int64_t value) = 0;
  virtual void Write(uint64_t value) = 0;
  virtual void Write(const portable_wide_string &value) = 0;
  virtual void Write(const GUID &value) = 0;
  virtual void Write(const std::vector<uint32_t> &value) = 0;
  virtual void Write(const std::vector<uint64_t> &value) = 0;
  virtual void WriteNull() = 0;
  virtual void Commit() = 0;
};