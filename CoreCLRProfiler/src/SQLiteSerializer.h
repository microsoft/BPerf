// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

class SQLiteSerializer : public Serializer
{
public:
  SQLiteSerializer(sqlite3_stmt *stmt);
  void Write(int8_t value) override;
  void Write(int16_t value) override;
  void Write(int32_t value) override;
  void Write(uint32_t value) override;
  void Write(int64_t value) override;
  void Write(uint64_t value) override;
  void Write(const portable_wide_string &value) override;
  void Write(const GUID &value) override;
  void Write(const std::vector<uint32_t> &value) override;
  void Write(const std::vector<uint64_t> &value) override;
  void WriteNull() override;
  void Commit() override;

private:
  sqlite3_stmt *stmt;
  int i;
};