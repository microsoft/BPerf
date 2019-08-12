#pragma once

#include <atomic>

class FileWriteOperation
{
public:

    FileWriteOperation(void* buffer, size_t size, size_t minEventTime, size_t maxEventTime, std::atomic<bool>* flushOperationPending)
    {
        this->Buffer = buffer;
        this->Size = size;
        this->MinEventTime = minEventTime;
        this->MaxEventTime = maxEventTime;
        this->FlushOperationPending = flushOperationPending;
    }

    void* Buffer;
    size_t Size;
    size_t MinEventTime;
    size_t MaxEventTime;
    std::atomic<bool>* FlushOperationPending;
};
