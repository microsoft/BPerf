// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "JITCompilationStartedEvent.h"

JITCompilationStartedEvent::JITCompilationStartedEvent(int64_t methodId, int64_t moduleId, int32_t methodToken, int32_t methodILSize, portable_wide_string methodNamespace, portable_wide_string methodName, portable_wide_string methodSignature, int16_t clrInstanceId) : EtwEvent(), methodId(methodId), moduleId(moduleId), methodToken(methodToken), methodILSize(methodILSize), methodNamespace(std::move(methodNamespace)), methodName(std::move(methodName)), methodSignature(std::move(methodSignature)), clrInstanceId(clrInstanceId)
{
}

void JITCompilationStartedEvent::Serialize(Serializer &t)
{
    EtwEvent::Serialize(t);

    t.Write(this->methodId);
    t.Write(this->moduleId);
    t.Write(this->methodToken);
    t.Write(this->methodILSize);
    t.Write(this->methodNamespace);
    t.Write(this->methodName);
    t.Write(this->methodSignature);
    t.Write(this->clrInstanceId);
}

int32_t JITCompilationStartedEvent::GetEventId()
{
    return 6;
}