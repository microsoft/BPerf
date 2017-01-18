// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "EtwEvent.h"
#include "sqlite3.h"
#include "SQLiteSerializer.h"

SQLiteSerializer::SQLiteSerializer(sqlite3_stmt *stmt) : stmt(stmt), i(1)
{
}

void SQLiteSerializer::Write(int8_t value)
{
    sqlite3_bind_int(this->stmt, i++, value);
}

void SQLiteSerializer::Write(int16_t value)
{
    sqlite3_bind_int(this->stmt, i++, value);
}

void SQLiteSerializer::Write(int32_t value)
{
    sqlite3_bind_int(this->stmt, i++, value);
}

void SQLiteSerializer::Write(uint32_t value)
{
    sqlite3_bind_int(this->stmt, i++, value);
}

void SQLiteSerializer::Write(int64_t value)
{
    sqlite3_bind_int64(this->stmt, i++, value);
}

void SQLiteSerializer::Write(uint64_t value)
{
    sqlite3_bind_int64(this->stmt, i++, value);
}

void SQLiteSerializer::Write(const portable_wide_string &value)
{
    sqlite3_bind_blob(this->stmt, i++, value.c_str(), static_cast<int32_t>(value.length() * 2), nullptr);
}

void SQLiteSerializer::Write(const GUID &value)
{
    sqlite3_bind_blob(this->stmt, i++, &value, 16, nullptr);
}

void SQLiteSerializer::Write(const std::vector<uint32_t> &value)
{
    sqlite3_bind_blob(this->stmt, i++, value.data(), static_cast<int32_t>(value.size() * sizeof(uint32_t)), nullptr);
}

void SQLiteSerializer::Write(const std::vector<uint64_t> &value)
{
    sqlite3_bind_blob(this->stmt, i++, value.data(), static_cast<int32_t>(value.size() * sizeof(uint64_t)), nullptr);
}

void SQLiteSerializer::WriteNull()
{
    sqlite3_bind_null(this->stmt, i++);
}

void SQLiteSerializer::Commit()
{
    sqlite3_step(this->stmt);
    sqlite3_reset(this->stmt);
}