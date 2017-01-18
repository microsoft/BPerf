// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "EtwEvent.h"

class MethodEnterEvent : public EtwEvent
{
public:
    MethodEnterEvent(intptr_t functionId, std::vector<uintptr_t> * functionIds);
    void Serialize(Serializer &t) override;
    int32_t GetEventId() override;

private:
    intptr_t functionId;
    std::vector<uintptr_t> *functionIds;
};