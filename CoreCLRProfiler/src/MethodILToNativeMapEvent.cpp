// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "MethodILToNativeMapEvent.h"

MethodILToNativeMapEvent::MethodILToNativeMapEvent(int64_t methodId, int64_t reJitId, int8_t methodExtent, int16_t countOfMapEntries, std::vector<uint32_t> ilOffsets, std::vector<uint32_t> nativeOffsets, int16_t clrInstanceId) : EtwEvent(), methodId(methodId), reJitId(reJitId), methodExtent(methodExtent), countOfMapEntries(countOfMapEntries), ilOffsets(std::move(ilOffsets)), nativeOffsets(std::move(nativeOffsets)), clrInstanceId(clrInstanceId)
{
}

void MethodILToNativeMapEvent::Serialize(Serializer &t)
{
    EtwEvent::Serialize(t);

    t.Write(this->methodId);
    t.Write(this->reJitId);
    t.Write(this->methodExtent);
    t.Write(this->countOfMapEntries);
    t.Write(this->ilOffsets);
    t.Write(this->nativeOffsets);
    t.Write(this->clrInstanceId);
}

int32_t MethodILToNativeMapEvent::GetEventId()
{
    return 7;
}