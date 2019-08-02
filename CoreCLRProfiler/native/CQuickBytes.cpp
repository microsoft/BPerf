// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#include "CQuickBytes.h"

CQuickBytes::CQuickBytes()
{
    this->size = 0;
    this->availableBytes = 1024;
    this->bytes.resize(this->availableBytes);
}
void CQuickBytes::Shrink(size_t iItems) { this->size = iItems; }
size_t CQuickBytes::Size() const { return this->size; }
void *CQuickBytes::Ptr() { return this->bytes.data(); }
int CQuickBytes::ReSizeNoThrow(size_t iItems)
{
    this->size = iItems;

    if (iItems > this->availableBytes)
    {
        this->availableBytes *= 2;
        this->bytes.resize(this->availableBytes);
    }

    return 0; // S_OK
}
