// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "SQLiteEventLogger.h"
#include "SQLiteSerializer.h"
#include "HeartBeatEvent.h"

template <class T, class... Types>
std::unique_ptr<T> LogEvent(Types &&... Args)
{
    return std::unique_ptr<T>(new T(std::forward<Types>(Args)...));
}

template <int32_t I>
void PrepareSql(sqlite3 *db, const char *sql, std::unordered_map<int32_t, sqlite3_stmt *> &map)
{
    sqlite3_stmt *stmt;
    sqlite3_prepare(db, sql, -1, &stmt, nullptr);
    map[I] = stmt;
}

void CreateSql(sqlite3 *db, const char *sql)
{
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

SQLiteEventLogger::SQLiteEventLogger(sqlite3 *db, std::shared_ptr<ThreadSafeQueue<std::unique_ptr<EtwEvent>>> eventQueue, const std::string &bPerfLogsPath) : db(db), eventQueue(std::move(eventQueue))
{
    auto err = sqlite3_open(bPerfLogsPath.c_str(), &this->db);
    if (err != SQLITE_OK)
    {
        printf("Encountered SQLite error when opening %s file (check write permissions/directory path exists): %s", bPerfLogsPath.c_str(), sqlite3_errmsg(this->db));
    }

    CreateSql(this->db, "CREATE TABLE `HeartBeatEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT);");
    CreateSql(this->db, "CREATE TABLE `ShutdownEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `ClrInstanceID` SMALLINT);");
    CreateSql(this->db, "CREATE TABLE `AppDomainLoadEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `AppDomainID` BIGINT, `AppDomainFlags` INT, `AppDomainName` TEXT, `AppDomainIndex` INT, `ClrInstanceID` SMALLINT);");
    CreateSql(this->db, "CREATE TABLE `AppDomainUnloadEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `AppDomainID` BIGINT, `AppDomainFlags` INT, `AppDomainName` TEXT, `AppDomainIndex` INT, `ClrInstanceID` SMALLINT);");
    CreateSql(this->db, "CREATE TABLE `AssemblyLoadEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `AssemblyID` BIGINT, `AppDomainID` BIGINT, `BindingID` BIGINT, `AssemblyFlags` INT, `FullyQualifiedAssemblyName` TEXT, `ClrInstanceID` SMALLINT);");
    CreateSql(this->db, "CREATE TABLE `AssemblyUnloadEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `AssemblyID` BIGINT, `AppDomainID` BIGINT, `BindingID` BIGINT, `AssemblyFlags` INT, `FullyQualifiedAssemblyName` TEXT, `ClrInstanceID` SMALLINT);");
    CreateSql(this->db, "CREATE TABLE `JITCompilationFinishedEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `MethodID` BIGINT, `ModuleID` BIGINT, `MethodStartAddress` BIGINT, `MethodSize` INT, `MethodToken` INT, `MethodFlags` INT, `MethodNamespace` TEXT, `MethodName` TEXT, `MethodSignature` TEXT, `ClrInstanceID` SMALLINT, `ReJITID` BIGINT);");
    CreateSql(this->db, "CREATE TABLE `JITCompilationStartedEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `MethodID` BIGINT, `ModuleID` BIGINT, `MethodToken` INT, `MethodILSize` INT, `MethodNamespace` TEXT, `MethodName` TEXT, `MethodSignature` TEXT, `ClrInstanceID` SMALLINT);");
    CreateSql(this->db, "CREATE TABLE `MethodILToNativeMapEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `MethodID` BIGINT, `ReJITID` BIGINT, `MethodExtent` TINYINT, `CountOfMapEntries` SMALLINT, `ILOffsets` BLOB, `NativeOffsets` BLOB, `ClrInstanceID` SMALLINT);");
    CreateSql(this->db, "CREATE TABLE `MethodUnloadEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `MethodID` BIGINT, `ModuleID` BIGINT, `MethodStartAddress` BIGINT, `MethodSize` INT, `MethodToken` INT, `MethodFlags` INT, `MethodNamespace` TEXT, `MethodName` TEXT, `MethodSignature` TEXT, `ClrInstanceID` SMALLINT, `ReJITID` BIGINT);");
    CreateSql(this->db, "CREATE TABLE `ModuleLoadEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `ModuleID` BIGINT, `AssemblyID` BIGINT, `ModuleFlags` INT, `Reserved1` INT, `ModuleILPath` TEXT, `ModuleNativePath` TEXT, `ClrInstanceID` SMALLINT, `ManagedPdbSignature` BLOB, `ManagedPdbAge` INT, `ManagedPdbBuildPath` TEXT, `NativePdbSignature` BLOB, `NativePdbAge` INT, `NativePdbBuildPath` TEXT);");
    CreateSql(this->db, "CREATE TABLE `ModuleUnloadEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `ModuleID` BIGINT, `AssemblyID` BIGINT, `ModuleFlags` INT, `Reserved1` INT, `ModuleILPath` TEXT, `ModuleNativePath` TEXT, `ClrInstanceID` SMALLINT, `ManagedPdbSignature` BLOB, `ManagedPdbAge` INT, `ManagedPdbBuildPath` TEXT, `NativePdbSignature` BLOB, `NativePdbAge` INT, `NativePdbBuildPath` TEXT);");
    CreateSql(this->db, "CREATE TABLE `RuntimeInformationEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `ClrInstanceID` SMALLINT, `Sku` SMALLINT, `BclMajorVersion` SMALLINT, `BclMinorVersion` SMALLINT, `BclBuildNumber` SMALLINT, `BclQfeNumber` SMALLINT, `VMMajorVersion` SMALLINT, `VMMinorVersion` SMALLINT, `VMBuildNumber` SMALLINT, `VMQfeNumber` SMALLINT, `StartupFlags` INT, `StartupMode` TINYINT, `CommandLine` TEXT, `ComObjectGuid` BLOB, `RuntimeDllPath` TEXT);");
    CreateSql(this->db, "CREATE TABLE `StartupEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `ClrInstanceID` SMALLINT, `SteadyClockTime` BIGINT, `SystemClockTime` BIGINT);");
    CreateSql(this->db, "CREATE TABLE `MethodEnterEvent` (`Timestamp` BIGINT, `ProcessId` INT, `ThreadId` INT, `FunctionID` BIGINT, `FrameCount` INT, `Frames` BLOB);");

    PrepareSql<-1>(this->db, "INSERT INTO `HeartBeatEvent` (`Timestamp`, `ProcessId`, `ThreadId`) VALUES (@1, @2, @3);", this->statementMap);
    PrepareSql<0>(this->db, "INSERT INTO `ShutdownEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `ClrInstanceID`) VALUES (@1, @2, @3, @4);", this->statementMap);
    PrepareSql<1>(this->db, "INSERT INTO `AppDomainLoadEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `AppDomainID`, `AppDomainFlags`, `AppDomainName`, `AppDomainIndex`, `ClrInstanceID`) VALUES (@1, @2, @3, @4, @5, @6, @7, @8);", this->statementMap);
    PrepareSql<2>(this->db, "INSERT INTO `AppDomainUnloadEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `AppDomainID`, `AppDomainFlags`, `AppDomainName`, `AppDomainIndex`, `ClrInstanceID`) VALUES (@1, @2, @3, @4, @5, @6, @7, @8);", this->statementMap);
    PrepareSql<3>(this->db, "INSERT INTO `AssemblyLoadEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `AssemblyID`, `AppDomainID`, `BindingID`, `AssemblyFlags`, `FullyQualifiedAssemblyName`, `ClrInstanceID`) VALUES (@1, @2, @3, @4, @5, @6, @7, @8, @9);", this->statementMap);
    PrepareSql<4>(this->db, "INSERT INTO `AssemblyUnloadEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `AssemblyID`, `AppDomainID`, `BindingID`, `AssemblyFlags`, `FullyQualifiedAssemblyName`, `ClrInstanceID`) VALUES (@1, @2, @3, @4, @5, @6, @7, @8, @9);", this->statementMap);
    PrepareSql<5>(this->db, "INSERT INTO `JITCompilationFinishedEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `MethodID`, `ModuleID`, `MethodStartAddress`, `MethodSize`, `MethodToken`, `MethodFlags`, `MethodNamespace`, `MethodName`, `MethodSignature`, `ClrInstanceID`, `ReJITID`) VALUES (@1, @2, @3, @4, @5, @6, @7, @8, @9, @10, @11, @12, @13, @14);", this->statementMap);
    PrepareSql<6>(this->db, "INSERT INTO `JITCompilationStartedEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `MethodID`, `ModuleID`, `MethodToken`, `MethodILSize`, `MethodNamespace`, `MethodName`, `MethodSignature`, `ClrInstanceID`) VALUES (@1, @2, @3, @4, @5, @6, @7, @8, @9, @10, @11); ", this->statementMap);
    PrepareSql<7>(this->db, "INSERT INTO `MethodILToNativeMapEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `MethodID`, `ReJITID`, `MethodExtent`, `CountOfMapEntries`, `ILOffsets`, `NativeOffsets`, `ClrInstanceID`) VALUES (@1, @2, @3, @4, @5, @6, @7, @8, @9, @10);", this->statementMap);
    PrepareSql<8>(this->db, "INSERT INTO `MethodUnloadEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `MethodID`, `ModuleID`, `MethodStartAddress`, `MethodSize`, `MethodToken`, `MethodFlags`, `MethodNamespace`, `MethodName`, `MethodSignature`, `ClrInstanceID`, `ReJITID`) VALUES (@1, @2, @3, @4, @5, @6, @7, @8, @9, @10, @11, @12, @13, @14);", this->statementMap);
    PrepareSql<9>(this->db, "INSERT INTO `ModuleLoadEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `ModuleID`, `AssemblyID`, `ModuleFlags`, `Reserved1`, `ModuleILPath`, `ModuleNativePath`, `ClrInstanceID`, `ManagedPdbSignature`, `ManagedPdbAge`, `ManagedPdbBuildPath`, `NativePdbSignature`, `NativePdbAge`, `NativePdbBuildPath`) VALUES (@1, @2, @3, @4, @5, @6, @7, @8, @9, @10, @11, @12, @13, @14, @15, @16);", this->statementMap);
    PrepareSql<10>(this->db, "INSERT INTO `ModuleUnloadEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `ModuleID`, `AssemblyID`, `ModuleFlags`, `Reserved1`, `ModuleILPath`, `ModuleNativePath`, `ClrInstanceID`, `ManagedPdbSignature`, `ManagedPdbAge`, `ManagedPdbBuildPath`, `NativePdbSignature`, `NativePdbAge`, `NativePdbBuildPath`) VALUES (@1, @2, @3, @4, @5, @6, @7, @8, @9, @10, @11, @12, @13, @14, @15, @16);", this->statementMap);
    PrepareSql<11>(this->db, "INSERT INTO `RuntimeInformationEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `ClrInstanceID`, `Sku`, `BclMajorVersion`, `BclMinorVersion`, `BclBuildNumber`, `BclQfeNumber`, `VMMajorVersion`, `VMMinorVersion`, `VMBuildNumber`, `VMQfeNumber`, `StartupFlags`, `StartupMode`, `CommandLine`, `ComObjectGuid`, `RuntimeDllPath`) VALUES (@1, @2, @3, @4, @5, @6, @7, @8, @9, @10, @11, @12, @13, @14, @15, @16, @17, @18);", this->statementMap);
    PrepareSql<12>(this->db, "INSERT INTO `StartupEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `ClrInstanceID`, `SteadyClockTime`, `SystemClockTime`) VALUES (@1, @2, @3, @4, @5, @6);", this->statementMap);
    PrepareSql<13>(this->db, "INSERT INTO `MethodEnterEvent` (`Timestamp`, `ProcessId`, `ThreadId`, `FunctionID`, `FrameCount`, `FunctionIDs`) VALUES (@1, @2, @3, @4, @5, @6);", this->statementMap);

    this->heartBeatThread = std::thread([this] {
        while (true)
        {
            this->eventQueue->push(LogEvent<HeartBeatEvent>());
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });
}

void SQLiteEventLogger::BeginTransaction()
{
    sqlite3_exec(this->db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
}

void SQLiteEventLogger::EndTransaction()
{
    sqlite3_exec(this->db, "END TRANSACTION", nullptr, nullptr, nullptr);
}

void SQLiteEventLogger::FinalizeStatements()
{
    for (auto &&e : this->statementMap)
    {
        sqlite3_finalize(e.second);
    }
}

void SQLiteEventLogger::RunEventLoop()
{
    this->BeginTransaction();

    while (true)
    {
        auto event = this->eventQueue->pop();
        auto eventId = event->GetEventId();
        SQLiteSerializer serializer(this->statementMap[eventId]);
        event->Serialize(serializer);
        serializer.Commit();

        if (eventId <= 0)
        {
            this->EndTransaction();
            if (eventId == 0)
            {
                this->FinalizeStatements();
                break;
            }
            this->BeginTransaction();
        }
    }
}

SQLiteEventLogger::~SQLiteEventLogger()
{
    if (this->db != nullptr)
    {
        sqlite3_close(this->db);
    }

    this->heartBeatThread.detach(); // not harmful since in a very short while the profiler will be torn down
}