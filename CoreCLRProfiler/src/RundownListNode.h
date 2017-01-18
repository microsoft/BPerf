// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

template <class T>
class RundownList;

template <class T>
class RundownListNode
{
    template <typename>
    friend class RundownList;
    RundownListNode(std::unique_ptr<T> data, RundownListNode<T> *next) : data(std::move(data)), next(next), prev(nullptr)
    {
    }

    RundownListNode() : next(nullptr), prev(nullptr)
    {
    }

    std::unique_ptr<T> data;
    RundownListNode<T> *next;
    RundownListNode<T> *prev;
};

template <typename T>
using RundownListNodeId = RundownListNode<T> *;