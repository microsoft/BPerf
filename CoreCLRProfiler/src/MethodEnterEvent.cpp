// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "MethodEnterEvent.h"

MethodEnterEvent::MethodEnterEvent(intptr_t functionId, std::vector<uintptr_t> *functionIds) : functionId(functionId), functionIds(functionIds)
{
}

void MethodEnterEvent::Serialize(Serializer &t)
{
    EtwEvent::Serialize(t);
    t.Write(this->functionId);
    t.Write(this->functionIds->size());
    t.Write(*this->functionIds);
}

int32_t MethodEnterEvent::GetEventId()
{
    return 13;
}