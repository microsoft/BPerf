// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <vector>

class CQuickBytes
{
public:
  CQuickBytes();
  size_t Size() const;
  void *Ptr();
  int ReSizeNoThrow(size_t);

private:
  std::vector<char *> bytes;
  size_t size;
  size_t availableBytes;
};