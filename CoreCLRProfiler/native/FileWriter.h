// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <fstream>
#include <string>
#include <thread>
#include "FileWriteOperation.h"
#include "ThreadSafeQueue.h"

class FileWriter
{
private:
    ThreadSafeQueue<FileWriteOperation> eventQueue;
    std::thread diskWriteThread;
    std::ofstream stream;

public:
    FileWriter(std::string filepath)
    {
        this->stream = std::ofstream(filepath.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
        this->diskWriteThread = std::thread([this] {
            while (true)
            {
                auto t = this->eventQueue.pop();

                if (t.Buffer == nullptr) // break condition
                {
                    break;
                }

                this->stream.write(static_cast<char*>(t.Buffer), t.Size);

                *t.FlushOperationPending = false;
            }
        });
    }

    ~FileWriter()
    {
        this->diskWriteThread.join();
        this->stream.close();
    }

    void Enqueue(FileWriteOperation op)
    {
        this->eventQueue.push(op);
    }
};
