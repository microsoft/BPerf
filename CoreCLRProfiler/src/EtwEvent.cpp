// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <chrono>
#include "EtwEvent.h"
#include "profiler_pal.h"

EtwEvent::EtwEvent() : processId(GetCurrentProcessId()), threadId(GetCurrentThreadId()), timestamp(std::chrono::steady_clock::now().time_since_epoch().count())
{
}

void EtwEvent::Serialize(Serializer &t)
{
    t.Write(this->timestamp);
    t.Write(this->processId);
    t.Write(this->threadId);
}