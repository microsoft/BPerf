// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template <class T>
class ThreadSafeQueue
{
public:
    void push(T t)
    {
        {
            std::lock_guard<std::mutex> lock(this->mutex);
            this->queue.push(std::move(t));
        }

        this->condition_variable.notify_one();
    }

    T pop()
    {
        std::unique_lock<std::mutex> lock(this->mutex);
        this->condition_variable.wait(lock, [this] { return !this->queue.empty(); });
        T t(std::move(this->queue.front()));
        this->queue.pop();
        return t;
    }

private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable condition_variable;
};
