// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "MethodUnloadEvent.h"

MethodUnloadEvent::MethodUnloadEvent(int64_t methodId, int64_t moduleId, int64_t methodStartAddress, int32_t methodSize, int32_t methodToken, int32_t methodFlags, portable_wide_string methodNamespace, portable_wide_string methodName, portable_wide_string methodSignature, int16_t clrInstanceId, int64_t rejitId) : EtwEvent(), methodId(methodId), moduleId(moduleId), methodStartAddress(methodStartAddress), methodSize(methodSize), methodToken(methodToken), methodFlags(methodFlags), methodNamespace(std::move(methodNamespace)), methodName(std::move(methodName)), methodSignature(std::move(methodSignature)), clrInstanceId(clrInstanceId), rejitId(rejitId)
{
}

void MethodUnloadEvent::Serialize(Serializer &t)
{
    EtwEvent::Serialize(t);

    t.Write(this->methodId);
    t.Write(this->moduleId);
    t.Write(this->methodStartAddress);
    t.Write(this->methodSize);
    t.Write(this->methodToken);
    t.Write(this->methodFlags);
    t.Write(this->methodNamespace);
    t.Write(this->methodName);
    t.Write(this->methodSignature);
    t.Write(this->clrInstanceId);
    t.Write(this->rejitId);
}

int32_t MethodUnloadEvent::GetEventId()
{
    return 8;
}