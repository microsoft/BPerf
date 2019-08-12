#pragma once

#include "profiler_pal.h"

class JittingStartedEvent
{
public:
    JittingStartedEvent(void* ptr, int64_t methodId, int64_t moduleId, int32_t methodToken, int32_t methodILSize, const portable_wide_string& methodNamespace, const portable_wide_string& methodName, const portable_wide_string& methodSignature)
    {

    }
};
