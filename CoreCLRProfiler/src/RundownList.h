// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include "RundownListNode.h"

template <class T>
class RundownList
{
  public:
    RundownList() : head(new RundownListNode<T>()), isInGCMode(false)
    {
    }

    ~RundownList()
    {
        while (this->head != nullptr)
        {
            this->Remove(this->head);
        }
    }

    // Thread Safety Concerns : (1) modifying shared instance variable (this->head), protected by this-headNodeMutex
    RundownListNodeId<T> Add(std::unique_ptr<T> data)
    {
        std::lock_guard<std::mutex> lock(this->headNodeMutex); // guarantees only one adder
        this->head->prev = new RundownListNode<T>(std::move(data), this->head);
        this->head = this->head->prev;
        return this->head;
    }

    // Thread Safety Concerns : (1) multiple ::Remove calls are serialized via this->removeMutex
    //                          (2) modifying shared instance variable (this->head), protected by this-headNodeMutex
    //                          (3) deleting incoming parameter (node), protected by nullptr check inside a lock (this->removeMutex)
    //                          (4) modifying shared instance variable (this->collectables), **NOT** protected, however atomic variable this->isInGCMode arbitrates alternating exclusive access
    void Remove(RundownListNodeId<T> node)
    {
        std::lock_guard<std::mutex> lock(this->removeMutex); // guarantees only one remover

        if (node == nullptr)
        {
            return;
        }

        this->headNodeMutex.lock();
        if (node->prev == nullptr)
        {
            this->head = node->next;

            if (this->head != nullptr)
            {
                this->head->prev = nullptr;
            }
        }
        else
        {
            // the unlock could technically be here since we're not modifying this->head anymore, but ...
            node->prev->next = node->next;
            if (node->next != nullptr)
            {
                node->next->prev = node->prev;
            }
        }
        this->headNodeMutex.unlock();

        // If someone started iterating, we need to make sure that this node isn't deleted just yet
        // Instead we put it in a GC list of future collectables, and it's the job of the iterator
        // to do the so-called "collection".
        if (this->isInGCMode.load())
        {
            this->collectables.push_back(node);
        }
        else
        {
            delete node;
        }
    }

    void Iterate(void(callback)(T *))
    {
        // Well-known state: There are precisely 0 or 1 iterations happening right now.

        std::lock_guard<std::mutex> lock(this->iterationMutex); // guarantees only one iterator

        // Well-known state: There are precisely 0 iterations happening right now. This has the following properties:
        //                   (1) this->collectables.length() is equal to 0.
        //                   (2) this->isIncGCMode is equal to false, consequently any calls to ::Remove immediately delete the node and free up memory.
        //                   (3) ::Add is happily adding items to the list, there is a lock (headNodeMutex), but it is a constant-time lock (i.e. fixed instructions in between the lock) with no contention UNLESS ::Remove is also removing the HEAD node at the same time.

        this->isInGCMode.store(true); // any ::Remove() calls now add to a GC list instead of being immediately deleted

        this->headNodeMutex.lock(); // acquire a conistent start point
        auto start = this->head;
        this->headNodeMutex.unlock();

        // lock-free iteration
        while (start != nullptr && start->next != nullptr)
        {
            callback(start->data.get());
            start = start->next;
        }

        this->isInGCMode.store(false); // any ::Remove() calls now delete nodes immediately, and GC list is not being modified anymore

        // lock-free iteration to empty out the GC list
        for (auto &e : this->collectables)
        {
            delete e;
        }
    }

  private:
    std::vector<RundownListNode<T> *> collectables;
    RundownListNode<T> *head;
    std::atomic_bool isInGCMode;
    std::mutex headNodeMutex;
    std::mutex removeMutex;
    std::mutex iterationMutex;
};