// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "StackEvent.h"

StackEvent::StackEvent(const std::vector<uintptr_t> &functionIds) : functionIds(functionIds)
{
}

void StackEvent::Serialize(Serializer &t)
{
    EtwEvent::Serialize(t);
    t.Write(static_cast<int32_t>(this->functionIds.size()));
    t.Write(this->functionIds);
}

int32_t StackEvent::GetEventId()
{
    return 13;
}