#pragma once

#include <memory>
#include <atomic>
#include <vector>
#include "FileWriter.h"
#include "PortableString.h"
#include "cor.h"
#include "corprof.h"

#define THRESHOLD 64 * 1024

class EventLogger
{
public:
    EventLogger()
    {
        this->fileWriter = nullptr;
        this->usedSpace = 0;
        this->threadId = 0;
        this->flushOperationPending = false;
        this->space.resize(THRESHOLD * 2);

        this->curr = this->space.data();
        this->next = this->curr + this->space.size() / 2; // must be aligned;
    }

    ~EventLogger()
    {
        this->SwitchBufferAndQueueFlush(0);
        this->fileWriter = nullptr;
    }

    void AttachFileWriter(std::shared_ptr<FileWriter> fileWriter, int threadId)
    {
        this->fileWriter = fileWriter;
        this->threadId = threadId;
    }

    template <class T, class... Types>
    bool LogEvent(Types&& ... Args)
    {
        auto ptr = this->GetPointerForNextEvent(SizeOf(std::forward<Types>(Args)...));
        if (ptr == nullptr)
        {
            return false;
        }

        T t(ptr, std::forward<Types>(Args)...);

        return true;
    }

private:
    std::vector<char> space;
    char* curr;
    char* next;
    size_t usedSpace;
    std::atomic<bool> flushOperationPending;
    std::shared_ptr<FileWriter> fileWriter;
    int threadId;

private:

    template <typename T>
    size_t SizeOf(const T &t)
    {
        static_assert(false, "sdd");
    }

    template <typename T, typename... Rest>
    size_t SizeOf(const T &t, Rest... rest)
    {
        return SizeOf(t) + SizeOf(rest...);
    }

    template<>
    size_t SizeOf(const int64_t &arg)
    {
        return 8;
    }

    template<>
    size_t SizeOf(const int32_t &arg)
    {
        return 4;
    }

    template<>
    size_t SizeOf(const int16_t &arg)
    {
        return 2;
    }

    template<>
    size_t SizeOf(const portable_wide_string & arg)
    {
        return (arg.length() + 1) * sizeof(portable_wide_char);
    }

    template<>
    size_t SizeOf(const UINT_PTR &arg)
    {
        return 8;
    }

    template<>
    size_t SizeOf(const mdToken &arg)
    {
        return 4;
    }

    template<>
    size_t SizeOf(const ULONG &arg)
    {
        return 4;
    }

    template <class T, class... Types>
    size_t SizeOF(Types&& ... Args)
    {
        return SizeOF<T>;
    }

    __declspec(noinline) void* SwitchBufferAndQueueFlush(size_t size)
    {
        if (this->flushOperationPending)
        {
            return nullptr;
        }

        if (this->fileWriter == nullptr)
        {
            return nullptr;
        }

        this->flushOperationPending = true;
        this->fileWriter->Enqueue(FileWriteOperation(this->curr, this->usedSpace, 0, 0, &this->flushOperationPending));

        auto tmp = this->curr;
        this->curr = this->next;
        this->next = tmp;
        this->usedSpace = size;

        return this->curr;
    }

    void* GetPointerForNextEvent(size_t size)
    {
        auto retval = this->curr + this->usedSpace;

        if (size + this->usedSpace < THRESHOLD) [[likely]]
        {
            this->usedSpace += size;
            return retval;
        }
        else
        {
            return this->SwitchBufferAndQueueFlush(size);
        }
    }
};
