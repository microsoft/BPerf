// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <unordered_map>
#include <thread>
#include "ThreadSafeQueue.h"
#include "EtwEvent.h"
#include "sqlite3.h"

class SQLiteEventLogger
{
public:
  SQLiteEventLogger(sqlite3 *db, std::shared_ptr<ThreadSafeQueue<std::unique_ptr<EtwEvent>>> eventQueue, const std::string &bPerfLogsPath);
  ~SQLiteEventLogger();
  void BeginTransaction();
  void EndTransaction();
  void FinalizeStatements();
  void RunEventLoop();

private:
  sqlite3 *db;
  std::shared_ptr<ThreadSafeQueue<std::unique_ptr<EtwEvent>>> eventQueue;
  std::unordered_map<int32_t, sqlite3_stmt *> statementMap;
  std::thread heartBeatThread;
};
