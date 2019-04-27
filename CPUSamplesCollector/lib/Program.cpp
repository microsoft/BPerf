#include "Program.h"

#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <tdh.h>
#include <evntcons.h>
#include <strsafe.h>
#include "TraceEventCallback.h"

enum ImageType : BYTE
{
    Native,
    IL,
    NGEN,
    ReadyToRun
};

template <typename T, size_t N>
constexpr size_t length_of(T(&)[N])
{
    return N - 1;
}

template <typename T, size_t N>
constexpr size_t countOf(T const (&)[N]) noexcept
{
    return N;
}

template <typename T, size_t N>
constexpr size_t size_of(T(&)[N])
{
    return N * sizeof(T);
}

void printGuid(GUID guid)
{
    printf("{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
           guid.Data1, guid.Data2, guid.Data3,
           guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
           guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

std::wstring trim(std::wstring const &str)
{
    if (str.empty())
        return str;

    std::size_t firstScan = str.find_first_not_of(' ');
    std::size_t first = firstScan == std::wstring::npos ? str.length() : firstScan;
    std::size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

std::vector<std::wstring> split(const std::wstring &s, wchar_t delimiter)
{
    std::vector<std::wstring> tokens;
    std::wstring token;
    std::wstringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(trim(token));
    }
    return tokens;
}

GUID StringToGuid(const std::wstring &s)
{
    GUID guid;

    unsigned long p0;
    int p1, p2, p3, p4, p5, p6, p7, p8, p9, p10;

    swscanf_s(s.c_str(), L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
              &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);

    guid.Data1 = p0;
    guid.Data2 = (unsigned short)p1;
    guid.Data3 = (unsigned short)p2;
    guid.Data4[0] = (unsigned char)p3;
    guid.Data4[1] = (unsigned char)p4;
    guid.Data4[2] = (unsigned char)p5;
    guid.Data4[3] = (unsigned char)p6;
    guid.Data4[4] = (unsigned char)p7;
    guid.Data4[5] = (unsigned char)p8;
    guid.Data4[6] = (unsigned char)p9;
    guid.Data4[7] = (unsigned char)p10;

    return guid;
}

int ReadETWSessionIntData(const wchar_t* propertyName, const wchar_t* fileName, int defaultValue)
{
    return GetPrivateProfileInt(L"ETWSession", propertyName, defaultValue, fileName);
}

int ReadIntData(const wchar_t* sectionName, const wchar_t* propertyName, const wchar_t* fileName, int defaultValue)
{
    return GetPrivateProfileInt(sectionName, propertyName, defaultValue, fileName);
}

std::wstring ReadStringData(const wchar_t* sectionName, const wchar_t* propertyName, const wchar_t* fileName)
{
    std::wstring data(4096, '0');
    data.resize(GetPrivateProfileString(sectionName, propertyName, nullptr, &data[0], (DWORD)data.size(), fileName));
    return trim(data);
}

std::wstring ReadStringData(const std::wstring &sectionName, const std::wstring &propertyName, const wchar_t* fileName)
{
    std::wstring data(4096, '0');
    data.resize(GetPrivateProfileString(sectionName.c_str(), propertyName.c_str(), nullptr, &data[0], (DWORD)data.size(), fileName));
    return trim(data);
}

std::wstring ReadETWSessionStringData(const std::wstring &propertyName, const wchar_t* fileName)
{
    std::wstring data(4096, '0');
    data.resize(GetPrivateProfileString(L"ETWSession", propertyName.c_str(), nullptr, &data[0], (DWORD)data.size(), fileName));
    return trim(data);
}

std::vector<std::wstring> ReadETWSessionStringListData(const std::wstring &propertyName, const wchar_t* fileName)
{
    auto retVal = ReadETWSessionStringData(propertyName, fileName);
    return split(retVal, ',');
}

struct Provider
{
    std::wstring Name;
    GUID Guid;
    ULONG MatchAnyKeyword;
    ULONG MatchAllKeyword;
    BYTE Level;
    std::wstring ExeNamesFilter;
    std::vector<USHORT> StackEventIds;
    std::vector<USHORT> ExcludeEventIds;
    BOOL EnableStacksForAllEventIds;
};

enum OnDiskFormat
{
    BTL,
    ETL
};

struct Configuration
{
    BOOL EnableEmbeddingEventMetadata;

    BOOL EnableEmbeddingSymbolServerKeyInfo;

    BOOL EnableMetadataSideStream;

    std::wstring SessionName;

    BSTR SessionNameBSTR;

    std::wstring FileNamePrefix;

    std::wstring DataDirectory;

    int BufferSizeInKB;

    std::vector<Provider> ProviderList;

    int LogRotationIntervalInSeconds;

    int CPUSamplingRateInMS;

    ULONG KernelEnableFlags;

    OnDiskFormat OnDiskFormat;

    BOOL EnableCLRSymbolCollection;

    std::wstring CLRSymbolCollectionProcessNames;

    std::unordered_set<std::wstring> CLRSymbolCollectionProcessNamesSet;

    std::vector<CLASSIC_EVENT_ID> ClassicEventIdsStackList;

    std::unordered_map<std::wstring, std::wstring> VolumeMap;

    ~Configuration()
    {
        if (this->SessionNameBSTR != nullptr)
        {
            SysFreeString(this->SessionNameBSTR);
        }
    }
};

constexpr UCHAR SystemPathsOpCode = 33;
constexpr UCHAR VolumeMappingOpcode = 35;
constexpr GUID FakeSysConfigGuid = { 0x9b79ee91, 0xb5fd, 0x41c0, { 0xa2, 0x43, 0x42, 0x48, 0xe2, 0x66, 0xe9, 0xd0 } };

typedef int(__stdcall *NtSetSystemInformation_Fn)(int SystemInformationClass, void *SystemInformation, int SystemInformationLength);

typedef DWORD(__stdcall *RtlCompressBuffer_Fn)(
    USHORT CompressionFormatAndEngine,
    PUCHAR UncompressedBuffer,
    ULONG UncompressedBufferSize,
    PUCHAR CompressedBuffer,
    ULONG CompressedBufferSize,
    ULONG UncompressedChunkSize,
    PULONG FinalCompressedSize,
    PVOID WorkSpace);

LPWSTR SessionName;
RtlCompressBuffer_Fn RtlCompressBuffer;
bool CtrlCTriggered;
NtSetSystemInformation_Fn NtSetSystemInformation;
Configuration gConfiguration;

#define DATE_FORMAT_BTL L"%s%d-%02d-%02d-%02d-%02d-%02d_%d-%02d-%02d-%02d-%02d-%02d.btl"
#define DATE_FORMAT_BTL_ID L"%s%d-%02d-%02d-%02d-%02d-%02d_%d-%02d-%02d-%02d-%02d-%02d.btl.id"
#define DATE_FORMAT_BTL_MD L"%s%d-%02d-%02d-%02d-%02d-%02d_%d-%02d-%02d-%02d-%02d-%02d.btl.md"
#define DATE_FORMAT_ETL L"%s%d-%02d-%02d-%02d-%02d-%02d_%d-%02d-%02d-%02d-%02d-%02d.etl"
#define CLRJITANDLOADERKEYWORD 0x00020018
#define PROCESSANDIMAGELOADKEYWORD 0x00000050
#define BPerfSymbols L"BPerfSymbols"
#define LOGFILE_PATH L""
#define ETL_LOGFILE_NAME L"Relogger.etl"
#define BTL_LOGFILE_NAME L"Relogger.btl"
#define BTL_LOGFILE_NAME_ID L"Relogger.btl.id"
#define BTL_LOGFILE_NAME_MD L"Relogger.btl.md"
#define WriteBufferSizeThreshold 1000000
#define WriteBufferTimeThreshold 1000000
#define CompressionBufferSize 1024 * 1024 * 2
#define BlockSize 4096
#define addr(x) &x[0]

#ifndef align_up
#define align_up(num, align) \
    (((num) + ((align)-1)) & ~((align)-1))
#endif

enum SYSTEM_INFORMATION_CLASS
{
    SystemPerformanceTraceInformation = 31
};

typedef struct _EVENT_TRACE_TIME_PROFILE_INFORMATION
{
    TRACE_INFO_CLASS EventTraceInformationClass;
    ULONG ProfileInterval;
} EVENT_TRACE_TIME_PROFILE_INFORMATION;

DECLSPEC_NOINLINE VOID WriteMDData(PEVENT_HEADER eventHeader, ULONG userDataLength, PVOID userData, HANDLE metadataFile)
{
    DWORD bytesWritten = 0;

    WriteFile(metadataFile, eventHeader, sizeof(EVENT_HEADER), &bytesWritten, nullptr);
    WriteFile(metadataFile, &userDataLength, sizeof(ULONG), &bytesWritten, nullptr);
    WriteFile(metadataFile, userData, userDataLength, &bytesWritten, nullptr);
}

DECLSPEC_NOINLINE HRESULT WriteTraceEventInfo(PEVENT_RECORD eventRecord, HANDLE metadataFile)
{
    ULONG bufferSize = 0;
    TDHSTATUS status;
    BYTE buffer[65536];

    if ((eventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_CLASSIC_HEADER) != 0)
    {
        status = TdhGetEventInformation(eventRecord, 0, nullptr, nullptr, &bufferSize);

        if (status == ERROR_INSUFFICIENT_BUFFER && bufferSize > 0)
        {
            status = TdhGetEventInformation(eventRecord, 0, nullptr, (PTRACE_EVENT_INFO)addr(buffer), &bufferSize);
        }
    }
    else
    {
        status = TdhGetManifestEventInformation(&eventRecord->EventHeader.ProviderId, &eventRecord->EventHeader.EventDescriptor, nullptr, &bufferSize);

        if (status == ERROR_INSUFFICIENT_BUFFER && bufferSize > 0)
        {
            status = TdhGetManifestEventInformation(&eventRecord->EventHeader.ProviderId, &eventRecord->EventHeader.EventDescriptor, (PTRACE_EVENT_INFO)addr(buffer), &bufferSize);
        }
    }

    if (status == ERROR_SUCCESS)
    {
        EVENT_HEADER traceEventInfo = { 0 };
        auto flags = EVENT_HEADER_FLAG_CLASSIC_HEADER;
        if ((eventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) != 0)
        {
            flags |= EVENT_HEADER_FLAG_64_BIT_HEADER;
        }
        else
        {
            flags |= EVENT_HEADER_FLAG_32_BIT_HEADER;
        }

        traceEventInfo.Flags = (USHORT)flags;

        traceEventInfo.EventDescriptor.Id = 0xFFFF;
        traceEventInfo.EventDescriptor.Version = 0;
        traceEventInfo.EventDescriptor.Channel = 0;
        traceEventInfo.EventDescriptor.Level = 0;
        traceEventInfo.EventDescriptor.Opcode = 32;
        traceEventInfo.EventDescriptor.Task = 0;
        traceEventInfo.EventDescriptor.Keyword = 0;

        traceEventInfo.ProviderId = { 0xbbccf6c1, 0x6cd1, 0x48c4, {0x80, 0xff, 0x83, 0x94, 0x82, 0xe3, 0x76, 0x71} };
        traceEventInfo.TimeStamp = eventRecord->EventHeader.TimeStamp;
        traceEventInfo.ProcessId = eventRecord->EventHeader.ProcessId;
        traceEventInfo.ThreadId = eventRecord->EventHeader.ThreadId;

        WriteMDData(&traceEventInfo, bufferSize, addr(buffer), metadataFile);
    }

    return S_OK;
}

DECLSPEC_NOINLINE void InjectKernelTraceMetadataEvents(std::unordered_set<EventKey, EventKeyHasher> &map, PEVENT_RECORD eventRecord, ITraceRelogger *relogger)
{
    InjectTraceEventInfoEvent(eventRecord, relogger);
    const USHORT eventId = (eventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_CLASSIC_HEADER) != 0 ? eventRecord->EventHeader.EventDescriptor.Opcode : eventRecord->EventHeader.EventDescriptor.Id;
    map.insert(EventKey(eventRecord->EventHeader.ProviderId, eventId, eventRecord->EventHeader.EventDescriptor.Version));
}

DECLSPEC_NOINLINE HRESULT WriteMetadata(USHORT eventId, PEVENT_RECORD eventRecord, ContextInfo *context)
{
    if (eventRecord->EventHeader.EventDescriptor.Id == 65533)
    {
        WriteMDData(&eventRecord->EventHeader, eventRecord->UserDataLength, eventRecord->UserData, context->MetadataFile);
        return S_OK;
    }

    if (context->EnableMetadata)
    {
        context->EventMetadataWrittenMap.insert(EventKey(eventRecord->EventHeader.ProviderId, eventId, eventRecord->EventHeader.EventDescriptor.Version));

        switch (eventRecord->EventHeader.EventDescriptor.Id)
        {
        case 65533:
        case 65534:
            WriteMDData(&eventRecord->EventHeader, eventRecord->UserDataLength, eventRecord->UserData, context->MetadataFile);
            break;
        default:
            WriteTraceEventInfo(eventRecord, context->MetadataFile);
            break;
        }

        InjectKernelTraceMetadataEvents(context->EventMetadataWrittenMap, eventRecord, context->Relogger);
    }

    return S_OK;
}

DECLSPEC_NOINLINE BOOL WINAPI CompressChunk(ContextInfo *context, bool writeIndex, LONGLONG timestamp)
{
    DWORD bytesWritten;
    ULONG finalCompressedSize;

    const auto retVal = RtlCompressBuffer(COMPRESSION_ENGINE_STANDARD | COMPRESSION_FORMAT_XPRESS_HUFF, context->Buffer, context->Offset, context->CompressionBuffer, CompressionBufferSize, BlockSize, &finalCompressedSize, context->Workspace);
    if (retVal != 0)
    {
        return FALSE;
    }

    WriteFile(context->DataFile, &finalCompressedSize, sizeof(ULONG), &bytesWritten, nullptr);
    WriteFile(context->DataFile, &context->Offset, sizeof(ULONG), &bytesWritten, nullptr);
    WriteFile(context->DataFile, context->CompressionBuffer, finalCompressedSize, &bytesWritten, nullptr);
    WriteFile(context->DataFile, &bytesWritten, (4 - (finalCompressedSize % 4)) % 4, &bytesWritten, nullptr);

#ifdef DEBUG
    {
        LARGE_INTEGER filePointer = {0};
        SetFilePointerEx(context->DataFile, filePointer, &filePointer, FILE_CURRENT);
        printf("Offset: %llu, Uncompressed Size: %d, Compressed Size: %d, Events: %d\r\n", filePointer.QuadPart, context->Offset, finalCompressedSize, context->TotalEvents);
    }
#endif

    ZeroMemory(context->Buffer, context->Offset);

    context->Offset = 0;
#ifdef DEBUG
    context->TotalEvents = 0;
#endif

    if (writeIndex)
    {
        LARGE_INTEGER filePointer = {0};
        SetFilePointerEx(context->DataFile, filePointer, &filePointer, FILE_CURRENT);
        WriteFile(context->IndexFile, &timestamp, sizeof(LONGLONG), &bytesWritten, nullptr);
        WriteFile(context->IndexFile, &filePointer.QuadPart, sizeof(LONGLONG), &bytesWritten, nullptr);
        context->LastEventIndexedTimeStamp = timestamp;
    }

    return TRUE;
}

BOOL WINAPI BufferCallback(PEVENT_TRACE_LOGFILE logfile)
{
    UNREFERENCED_PARAMETER(logfile);
    return TRUE;
}

DECLSPEC_NOINLINE HRESULT InjectKernelTraceControlEvents(PEVENT_RECORD eventRecord, ContextInfo *context)
{
    // Image Load events don't carry enough information about the PE file needed for symbolic lookup, i.e. it doesn't contain the RSDS information
    if (IsEqualGUID(eventRecord->EventHeader.ProviderId, ImageLoadGuid))
    {
        return InjectKernelTraceControlEvents(eventRecord, context->Relogger);
    }

    return S_OK;
}

// *** NOTE ***
// HOTTEST METHOD IN THE PROGRAM
// ALL NON-ESSENTIAL CALLS NEED TO BE NOINLINED
// LOGICALLY THE ONLY WORK WE DO HERE IS:
// DICTIONARY LOOKUP FOR EVENT METADATA
// IF CHECK FOR ENABLE SYMBOL SERVER INFO FOR AGGREGATE PROFILE USERS
// COPY THE ETW DATA INTO BUFFERS
// COMPRESS THE ETW DATA WHEN BUFFER IS FULL
VOID WINAPI ProcessEvent(PEVENT_RECORD eventRecord)
{
    const auto context = static_cast<ContextInfo *>(eventRecord->UserContext);
    const USHORT eventId = (eventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_CLASSIC_HEADER) != 0 ? eventRecord->EventHeader.EventDescriptor.Opcode : eventRecord->EventHeader.EventDescriptor.Id;

    if (context->EventMetadataWrittenMap.find(EventKey(eventRecord->EventHeader.ProviderId, eventId, eventRecord->EventHeader.EventDescriptor.Version)) == context->EventMetadataWrittenMap.end())
    {
        WriteMetadata(eventId, eventRecord, context);
    }

    if (context->EnableSymbolServerInfo)
    {
        InjectKernelTraceControlEvents(eventRecord, context);
    }

    PBYTE buffer = context->Buffer;
    DWORD offset = context->Offset;

    // Buffer must be 16-byte aligned at this point, which HeapAlloc does and as long as we keep things aligned at 16-bytes at the end of this function, we're good.

    CopyMemory(buffer + offset, &eventRecord->EventHeader, offsetof(EVENT_RECORD, ExtendedData)); // copy uptil the ExtendedData element.
    offset += offsetof(EVENT_RECORD, ExtendedData);

    offset += sizeof(ULONGLONG) * 3; // store space for 3 8-byte pointers regardless of arch

    CopyMemory(buffer + offset, eventRecord->UserData, eventRecord->UserDataLength);
    offset += eventRecord->UserDataLength;

    offset = align_up(offset, sizeof(ULONGLONG));
    EVENT_HEADER_EXTENDED_DATA_ITEM item[1] = {0};

    for (USHORT i = 0; i < eventRecord->ExtendedDataCount; ++i)
    {
        item[0].ExtType = eventRecord->ExtendedData[i].ExtType;
        item[0].DataSize = eventRecord->ExtendedData[i].DataSize;

        CopyMemory(buffer + offset, &item[0], sizeof(EVENT_HEADER_EXTENDED_DATA_ITEM));
        offset += sizeof(EVENT_HEADER_EXTENDED_DATA_ITEM);
    }

    for (USHORT i = 0; i < eventRecord->ExtendedDataCount; ++i)
    {
        CopyMemory(buffer + offset, reinterpret_cast<LPCVOID>(eventRecord->ExtendedData[i].DataPtr), eventRecord->ExtendedData[i].DataSize);
        offset += eventRecord->ExtendedData[i].DataSize;
        offset = align_up(offset, sizeof(ULONGLONG));
    }

    offset = align_up(offset, sizeof(GUID)); // alignment of largest member of EVENT_HEADER

#ifdef DEBUG
    context->TotalEvents++;
#endif

    context->Offset = offset;

    const LONGLONG currentTimestamp = eventRecord->EventHeader.TimeStamp.QuadPart;

    auto elapsedMicroseconds = currentTimestamp - context->LastEventIndexedTimeStamp;
    elapsedMicroseconds *= 1000000;
    elapsedMicroseconds /= context->PerfFreq;

    const bool writeTimeBasedIndex = elapsedMicroseconds >= WriteBufferTimeThreshold;

    if (offset >= WriteBufferSizeThreshold || writeTimeBasedIndex)
    {
        CompressChunk(context, writeTimeBasedIndex, currentTimestamp);
    }
}

DECLSPEC_NOINLINE VOID UntrackProcess(ContextInfo2* contextInfo, PEVENT_RECORD eventRecord)
{
    auto processId = eventRecord->EventHeader.ProcessId;
    contextInfo->OffsetIntoDataFile.erase(processId);
    contextInfo->ManagedMethodMapFile.erase(processId);
    contextInfo->ManagedILToNativeMapFile.erase(processId);
    contextInfo->ManagedModuleMapFile.erase(processId);
    contextInfo->DataFile.erase(processId);
    contextInfo->NativeModuleMapFile.erase(processId);
}

DECLSPEC_NOINLINE VOID TrackProcess(ContextInfo2* contextInfo, PEVENT_RECORD eventRecord)
{
    WCHAR temp[4 * 1024];

    ULONG processId;
    CopyMemory(&processId, eventRecord->UserData, sizeof(ULONG));

    ULARGE_INTEGER ul;
    CopyMemory(&ul.QuadPart, (PBYTE)eventRecord->UserData + sizeof(ULONG), sizeof(LONGLONG));

    auto processName = (LPWSTR)((PBYTE)eventRecord->UserData + 24);

    for (auto &s : *contextInfo->CLRProcessNamesSet)
    {
        if (wcsstr(processName, s.c_str()) != nullptr)
        {
            FILETIME createTime;
            createTime.dwHighDateTime = ul.HighPart;
            createTime.dwLowDateTime = ul.LowPart;

            SYSTEMTIME systemTime;
            FileTimeToSystemTime(&createTime, &systemTime);

            contextInfo->OffsetIntoDataFile.insert(std::make_pair(processId, 0));

            swprintf_s(addr(temp), countOf(temp), L"%s\\BPerfSymbolicData.%d.%u-%02u-%02u-%02u-%02u-%02u-%03u.db.methodmap", contextInfo->DataDirectory, processId, systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
            contextInfo->ManagedMethodMapFile.emplace(std::make_pair(processId, std::move(SafeFileHandle(CreateFile(temp, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)))));

            swprintf_s(addr(temp), countOf(temp), L"%s\\BPerfSymbolicData.%d.%u-%02u-%02u-%02u-%02u-%02u-%03u.db.ilnativemap", contextInfo->DataDirectory, processId, systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
            contextInfo->ManagedILToNativeMapFile.emplace(std::make_pair(processId, std::move(SafeFileHandle(CreateFile(temp, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)))));

            swprintf_s(addr(temp), countOf(temp), L"%s\\BPerfSymbolicData.%d.%u-%02u-%02u-%02u-%02u-%02u-%03u.db.modulemap", contextInfo->DataDirectory, processId, systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
            contextInfo->ManagedModuleMapFile.emplace(std::make_pair(processId, std::move(SafeFileHandle(CreateFile(temp, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)))));

            swprintf_s(addr(temp), countOf(temp), L"%s\\BPerfSymbolicData.%d.%u-%02u-%02u-%02u-%02u-%02u-%03u.db", contextInfo->DataDirectory, processId, systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
            contextInfo->DataFile.emplace(std::make_pair(processId, std::move(SafeFileHandle(CreateFile(temp, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)))));

            swprintf_s(addr(temp), countOf(temp), L"%s\\BPerfSymbolicData.%d.%u-%02u-%02u-%02u-%02u-%02u-%03u.db.nativemodulemap", contextInfo->DataDirectory, processId, systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
            contextInfo->NativeModuleMapFile.emplace(std::make_pair(processId, std::move(SafeFileHandle(CreateFile(temp, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)))));

            break;
        }
    }
}

static DWORD RVAToFileOffset(PIMAGE_NT_HEADERS32 ntHeader, DWORD rva)
{
    PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION(ntHeader);
    DWORD numberOfSections = ntHeader->FileHeader.NumberOfSections;

    for (DWORD i = 0; i < numberOfSections; ++i)
    {
        if (sections[i].VirtualAddress <= rva && rva < sections[i].VirtualAddress + sections[i].Misc.VirtualSize)
        {
            return sections[i].PointerToRawData + (rva - sections[i].VirtualAddress);
        }
    }

    return 0;
}

DECLSPEC_NOINLINE USHORT InjectDbgIdInfo(PEVENT_RECORD eventRecord, PBYTE dest, USHORT * pdbEntries, ImageType * imageType)
{
    WCHAR usablePEFileName[4 * 1024] = { 0 };
    auto peFileNameFromEvent = (LPWSTR)((PBYTE)eventRecord->UserData + 36);

    bool foundSuitableVolumeMapping = false;
    const auto volumeMap = static_cast<ContextInfo2 *>(eventRecord->UserContext)->VolumeMap;
    for (auto const& x : *volumeMap)
    {
        if (_wcsnicmp(peFileNameFromEvent, x.first.c_str(), x.first.length()) == 0)
        {
            StringCbPrintfW(addr(usablePEFileName), countOf(usablePEFileName), L"%s%s", x.second.c_str(), peFileNameFromEvent + x.first.length());
            foundSuitableVolumeMapping = true;
            break;
        }
    }

    if (!foundSuitableVolumeMapping)
    {
        WCHAR windowsDirectory[(MAX_PATH + 1) * 2];
        GetWindowsDirectory(addr(windowsDirectory), MAX_PATH);
        windowsDirectory[3] = L'\0';
        StringCbPrintfW(addr(usablePEFileName), countOf(usablePEFileName), L"%s%s", addr(windowsDirectory), peFileNameFromEvent + 1);
    }

    const auto file = CreateFile(addr(usablePEFileName), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (file == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    const auto mapFile = CreateFileMapping(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (mapFile == nullptr)
    {
        CloseHandle(file);
        return 0;
    }

    const auto imageBase = reinterpret_cast<PBYTE>(MapViewOfFile(mapFile, FILE_MAP_READ, 0, 0, 0));
    const auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(imageBase);

    auto ntHeader = reinterpret_cast<PIMAGE_NT_HEADERS32>(imageBase + dosHeader->e_lfanew);

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE || ntHeader->Signature != IMAGE_NT_SIGNATURE)
    {
        UnmapViewOfFile(imageBase);
        CloseHandle(mapFile);
        CloseHandle(file);
        return 0;
    }

    IMAGE_DATA_DIRECTORY comDescriptorDirectory;
    IMAGE_DATA_DIRECTORY debugDescriptorDirectory;

    if (ntHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        auto ntHeader64 = reinterpret_cast<PIMAGE_NT_HEADERS64>(imageBase + dosHeader->e_lfanew);
        comDescriptorDirectory = ntHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
        debugDescriptorDirectory = ntHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    }
    else
    {
        comDescriptorDirectory = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
        debugDescriptorDirectory = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    }

    if (comDescriptorDirectory.VirtualAddress != 0 && sizeof(IMAGE_COR20_HEADER) <= comDescriptorDirectory.Size)
    {
        *imageType = IL;

        auto managedHeader = (PIMAGE_COR20_HEADER)(imageBase + RVAToFileOffset(ntHeader, comDescriptorDirectory.VirtualAddress));
        if (managedHeader->ManagedNativeHeader.VirtualAddress != 0)
        {
            *imageType = NGEN;

            struct READYTORUN_HEADER
            {
                DWORD                   Signature;      // READYTORUN_SIGNATURE
                USHORT                  MajorVersion;   // READYTORUN_VERSION_XXX
                USHORT                  MinorVersion;

                DWORD                   Flags;          // READYTORUN_FLAG_XXX

                DWORD                   NumberOfSections;

                // Array of sections follows. The array entries are sorted by Type
                // READYTORUN_SECTION   Sections[];
            };

            if (sizeof(READYTORUN_HEADER) <= managedHeader->ManagedNativeHeader.Size)
            {
                auto rtrHeader = (READYTORUN_HEADER*)(imageBase + RVAToFileOffset(ntHeader, managedHeader->ManagedNativeHeader.VirtualAddress));
                if (rtrHeader->Signature == 0x00525452) // 'RTR'
                {
                    *imageType = ReadyToRun;
                }
            }
        }
    }

    USHORT payloadSize = 0;

    if (debugDescriptorDirectory.VirtualAddress != 0)
    {
        auto debugDirectories = (PIMAGE_DEBUG_DIRECTORY)(imageBase + RVAToFileOffset(ntHeader, debugDescriptorDirectory.VirtualAddress));
        ULONG numberOfDebugDirectories = debugDescriptorDirectory.Size / sizeof(IMAGE_DEBUG_DIRECTORY);

        for (PIMAGE_DEBUG_DIRECTORY current = debugDirectories; numberOfDebugDirectories != 0; --numberOfDebugDirectories, ++current)
        {
            if (current->Type == IMAGE_DEBUG_TYPE_CODEVIEW)
            {
                struct CV_INFO_PDB70
                {
                    DWORD CvSignature;
                    GUID Signature;
                    DWORD Age;
                    char PdbFileName[1];
                };

                const auto data = reinterpret_cast<CV_INFO_PDB70 *>(imageBase + current->PointerToRawData);

                if (data->CvSignature == 0x53445352) // RSDS in ASCII
                {
                    auto pdbFileNameSize = (DWORD)(strlen(data->PdbFileName) + 1);
                    auto dataLength = (USHORT)(sizeof(GUID) + sizeof(DWORD) + pdbFileNameSize);
                    payloadSize += dataLength;
                    *pdbEntries = *pdbEntries + 1;

                    CopyMemory(dest, &data->Signature, sizeof(GUID));
                    dest += sizeof(GUID);

                    CopyMemory(dest, &data->Age, sizeof(DWORD));
                    dest += sizeof(DWORD);

                    CopyMemory(dest, addr(data->PdbFileName), pdbFileNameSize);
                    dest += pdbFileNameSize;
                }

                break;
            }
        }
    }

    UnmapViewOfFile(imageBase);
    CloseHandle(mapFile);
    CloseHandle(file);

    return payloadSize;
}

DECLSPEC_NOINLINE VOID DoWorkForTrackingProcessManaged(PEVENT_RECORD eventRecord)
{
    const auto contextInfo = static_cast<ContextInfo2 *>(eventRecord->UserContext);
    auto processId = eventRecord->EventHeader.ProcessId;
    auto eventId = eventRecord->EventHeader.EventDescriptor.Id;

    if (eventRecord->EventHeader.ProviderId == ClrProviderGuid)
    {
        auto iter = contextInfo->OffsetIntoDataFile.find(processId);

        switch (eventId)
        {

        case MethodLoadEvent:
        case MethodUnloadEvent:
        {
            auto jittedCodeMethodMapFile = contextInfo->ManagedMethodMapFile.find(processId)->second.Get();

            WriteFile(jittedCodeMethodMapFile, (PBYTE)eventRecord->UserData + 16, sizeof(LONGLONG), nullptr, nullptr);
            WriteFile(jittedCodeMethodMapFile, (PBYTE)eventRecord->UserData + 24, sizeof(LONG), nullptr, nullptr);
            WriteFile(jittedCodeMethodMapFile, &iter->second, sizeof(LONG), nullptr, nullptr); // yes this limits the file the main file to 4GB, which is plenty.

            break;
        }

        case ILToNativeMapEvent:
        {
            auto ManagedILToNativeMapFile = contextInfo->ManagedILToNativeMapFile.find(processId)->second.Get();
            WriteFile(ManagedILToNativeMapFile, eventRecord->UserData, sizeof(LONGLONG), nullptr, nullptr);
            WriteFile(ManagedILToNativeMapFile, &iter->second, sizeof(LONG), nullptr, nullptr); // yes this limits the file the main file to 4GB, which is plenty.

            break;
        }

        case ModuleLoadEvent:
        case ModuleUnloadEvent:
        {
            auto ManagedModuleMapFile = contextInfo->ManagedModuleMapFile.find(processId)->second.Get();
            WriteFile(ManagedModuleMapFile, eventRecord->UserData, sizeof(LONGLONG), nullptr, nullptr);
            WriteFile(ManagedModuleMapFile, &iter->second, sizeof(LONG), nullptr, nullptr); // yes this limits the file the main file to 4GB, which is plenty.

            break;
        }

        }

        auto jittedCodeInfoFile = contextInfo->DataFile.find(processId)->second.Get();

        WriteFile(jittedCodeInfoFile, &eventId, sizeof(USHORT), nullptr, nullptr);
        WriteFile(jittedCodeInfoFile, &eventRecord->EventHeader.TimeStamp, sizeof(LARGE_INTEGER), nullptr, nullptr);
        WriteFile(jittedCodeInfoFile, &eventRecord->UserDataLength, sizeof(USHORT), nullptr, nullptr);
        WriteFile(jittedCodeInfoFile, eventRecord->UserData, eventRecord->UserDataLength, nullptr, nullptr);

        iter->second += sizeof(USHORT) + sizeof(LARGE_INTEGER) + sizeof(USHORT) + eventRecord->UserDataLength;
    }
}

DECLSPEC_NOINLINE VOID DoWorkForTrackingProcessNative(PEVENT_RECORD eventRecord)
{
    BYTE debugInfoData[4096];

    const auto contextInfo = static_cast<ContextInfo2 *>(eventRecord->UserContext);
    auto processId = eventRecord->EventHeader.ProcessId;

    auto iter = contextInfo->OffsetIntoDataFile.find(processId);

    auto nativeModuleMapFile = contextInfo->NativeModuleMapFile.find(processId)->second.Get();

    WriteFile(nativeModuleMapFile, (PBYTE)eventRecord->UserData, sizeof(LONGLONG), nullptr, nullptr);
    WriteFile(nativeModuleMapFile, (PBYTE)eventRecord->UserData + 8, sizeof(LONG), nullptr, nullptr);
    WriteFile(nativeModuleMapFile, &iter->second, sizeof(LONG), nullptr, nullptr); // yes this limits the file the main file to 4GB, which is plenty.

    USHORT pdbEntries = 0;
    ImageType imageType = Native;
    USHORT payloadSize = InjectDbgIdInfo(eventRecord, addr(debugInfoData), &pdbEntries, &imageType);
    USHORT totalSize = eventRecord->UserDataLength + payloadSize + sizeof(USHORT) + sizeof(BYTE);

    auto dataFile = contextInfo->DataFile.find(processId)->second.Get();

    USHORT fakeEventId = 65535;
    WriteFile(dataFile, &fakeEventId, sizeof(USHORT), nullptr, nullptr);
    WriteFile(dataFile, &eventRecord->EventHeader.TimeStamp, sizeof(LARGE_INTEGER), nullptr, nullptr);
    WriteFile(dataFile, &totalSize, sizeof(USHORT), nullptr, nullptr);
    WriteFile(dataFile, eventRecord->UserData, eventRecord->UserDataLength, nullptr, nullptr);
    WriteFile(dataFile, &imageType, sizeof(BYTE), nullptr, nullptr);
    WriteFile(dataFile, &pdbEntries, sizeof(USHORT), nullptr, nullptr);
    WriteFile(dataFile, addr(debugInfoData), payloadSize, nullptr, nullptr);

    iter->second += sizeof(USHORT) + sizeof(LARGE_INTEGER) + sizeof(USHORT) + totalSize;
}

VOID WINAPI ProcessEventWithSymbols(PEVENT_RECORD eventRecord)
{
    const auto contextInfo = static_cast<ContextInfo2 *>(eventRecord->UserContext);
    bool trackingProcess = contextInfo->OffsetIntoDataFile.find(eventRecord->EventHeader.ProcessId) != contextInfo->OffsetIntoDataFile.end();

    if (eventRecord->EventHeader.ProviderId == KernelProcessGuid)
    {
        switch (eventRecord->EventHeader.EventDescriptor.Id)
        {

        case 1: // Process Start
            TrackProcess(contextInfo, eventRecord);
            return;
        case 2: // Process Stop
            if (trackingProcess)
            {
                UntrackProcess(contextInfo, eventRecord);
            }
            return;

        case 5: // Image Load
        {
            if (trackingProcess)
            {
                DoWorkForTrackingProcessNative(eventRecord);
            }

            return;
        }

        default:
            return;

        }
    }

    // we only enable CLR Events in addition to Image/ProcessLoad so therefore it's cheaper to do an if check
    // instead of having the whole guid compare code be in this method
    if (trackingProcess)
    {
        DoWorkForTrackingProcessManaged(eventRecord);
    }
}

ULONG EnableTraceEx2Shim(_In_ TRACEHANDLE traceHandle, _In_ LPCGUID guid, _In_ BYTE level, _In_ ULONG matchAnyKeyword, _In_ ULONG matchAllKeyword, _In_ ULONG timeout, _In_opt_ LPCWSTR exeNamesFilter, _In_opt_ const USHORT *stackEventIdsFilter, _In_ USHORT stackEventIdsCount, _In_opt_ const USHORT *excludeEventIdsFilter, _In_ USHORT excludeEventIdsCount)
{
    int index = 0;

    EVENT_FILTER_DESCRIPTOR descriptor[4];
    ENABLE_TRACE_PARAMETERS enableParameters;
    ZeroMemory(&enableParameters, sizeof(ENABLE_TRACE_PARAMETERS));
    enableParameters.FilterDescCount = 0;
    enableParameters.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
    enableParameters.EnableFilterDesc = descriptor;

    BYTE stackEventIdsBufferBytes[1024];
    auto stackEventIdsBufferSize = ((stackEventIdsCount - 1) * 2) + sizeof(EVENT_FILTER_EVENT_ID);
    PEVENT_FILTER_EVENT_ID stackEventIdsBuffer = (PEVENT_FILTER_EVENT_ID)addr(stackEventIdsBufferBytes);

    ZeroMemory(stackEventIdsBuffer, stackEventIdsBufferSize);

    stackEventIdsBuffer->FilterIn = TRUE;
    stackEventIdsBuffer->Reserved = 0;
    stackEventIdsBuffer->Count = stackEventIdsCount;

    {
        auto eventIdsPtr = &stackEventIdsBuffer->Events[0];
        for (int i = 0; i < stackEventIdsCount; ++i)
        {
            *eventIdsPtr++ = stackEventIdsFilter[i];
        }
    }

    auto excludeEventIdsBufferSize = ((excludeEventIdsCount - 1) * 2) + sizeof(EVENT_FILTER_EVENT_ID);
    BYTE excludeEventIdsBufferBytes[1024];
    PEVENT_FILTER_EVENT_ID excludeEventIdsBuffer = (PEVENT_FILTER_EVENT_ID)addr(excludeEventIdsBufferBytes);

    ZeroMemory(excludeEventIdsBuffer, excludeEventIdsBufferSize);

    excludeEventIdsBuffer->FilterIn = FALSE;
    excludeEventIdsBuffer->Reserved = 0;
    excludeEventIdsBuffer->Count = excludeEventIdsCount;

    {
        auto eventIdsPtr = &excludeEventIdsBuffer->Events[0];
        for (int i = 0; i < excludeEventIdsCount; ++i)
        {
            *eventIdsPtr++ = excludeEventIdsFilter[i];
        }
    }

    if (exeNamesFilter != nullptr)
    {
        descriptor[index].Ptr = reinterpret_cast<ULONGLONG>(exeNamesFilter);
        descriptor[index].Size = static_cast<ULONG>((wcslen(exeNamesFilter) + 1) * sizeof(WCHAR));
        descriptor[index].Type = EVENT_FILTER_TYPE_EXECUTABLE_NAME;
        index++;
    }

    if (stackEventIdsCount > 0)
    {
        descriptor[index].Ptr = reinterpret_cast<ULONGLONG>(stackEventIdsBuffer);
        descriptor[index].Size = static_cast<ULONG>(stackEventIdsBufferSize);
        descriptor[index].Type = EVENT_FILTER_TYPE_STACKWALK;
        index++;

        enableParameters.EnableProperty = EVENT_ENABLE_PROPERTY_STACK_TRACE;
    }

    if (excludeEventIdsCount > 0)
    {
        descriptor[index].Ptr = ULONGLONG(excludeEventIdsBuffer);
        descriptor[index].Size = static_cast<ULONG>(excludeEventIdsBufferSize);
        descriptor[index].Type = EVENT_FILTER_TYPE_EVENT_ID;
        index++;
    }

    enableParameters.FilterDescCount = index; // set the right size

    const auto retVal = EnableTraceEx2(traceHandle, guid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, level, matchAnyKeyword, matchAllKeyword, timeout, &enableParameters);

    return retVal;
}

void FillEventProperties(PEVENT_TRACE_PROPERTIES sessionProperties, ULONG bufferSize, ULONG eventBufferSizeInKB, LPCWSTR sessionName, ULONG logFileMode)
{
    ZeroMemory(sessionProperties, bufferSize);

    sessionProperties->Wnode.BufferSize = bufferSize;
    sessionProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    sessionProperties->Wnode.ClientContext = 1;
    sessionProperties->EnableFlags = EVENT_TRACE_FLAG_NO_SYSCONFIG;

    SYSTEM_INFO systemInfo = { 0 };
    GetSystemInfo(&systemInfo);

    sessionProperties->MinimumBuffers = max(64, systemInfo.dwNumberOfProcessors * 2);
    sessionProperties->MaximumBuffers = sessionProperties->MinimumBuffers * 2;

    sessionProperties->BufferSize = eventBufferSizeInKB;
    sessionProperties->LogFileMode = logFileMode;
    sessionProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    sessionProperties->LogFileNameOffset = 0;
    sessionProperties->FlushTimer = 1;

    StringCbCopy(reinterpret_cast<LPWSTR>(reinterpret_cast<char *>(sessionProperties) + sessionProperties->LoggerNameOffset), (wcslen(sessionName) + 1) * 2, sessionName);
}

HRESULT StartSession(PTRACEHANDLE sessionHandle, PEVENT_TRACE_PROPERTIES sessionProperties, LPCWSTR sessionName)
{
    HRESULT hr = ERROR_SUCCESS;
    int retryCount = 1;
    while (retryCount < 10)
    {
        hr = StartTrace(static_cast<PTRACEHANDLE>(sessionHandle), sessionName, sessionProperties);
        if (hr != ERROR_SUCCESS)
        {
            if (hr == ERROR_ALREADY_EXISTS)
            {
                hr = ControlTrace(*sessionHandle, sessionName, sessionProperties, EVENT_TRACE_CONTROL_STOP);
                if (hr == ERROR_WMI_INSTANCE_NOT_FOUND || hr == ERROR_SUCCESS)
                {
                    Sleep(100 * retryCount); // sleep for 100-milliseconds to let it propogate.
                    retryCount++;
                    continue;
                }
            }

            return hr;
        }

        break;
    }

    return hr;
}

bool AcquireProfilePrivileges()
{
    bool privileged = false;

    HANDLE tokenHandle;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &tokenHandle))
    {
        TOKEN_PRIVILEGES privileges;

        privileges.PrivilegeCount = 1;
        privileges.Privileges->Luid.LowPart = 11;
        privileges.Privileges->Attributes = SE_PRIVILEGE_ENABLED;

        if (AdjustTokenPrivileges(tokenHandle, false, &privileges, 0, nullptr, nullptr))
        {
            privileged = true;
        }

        CloseHandle(tokenHandle);
    }

    return privileged;
}

BOOL WINAPI ConsoleControlHandler(DWORD ctrlType)
{
    switch (ctrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        wprintf(L"Ctrl+C handled, stopping session.\n");
        CtrlCTriggered = true;

        {
            WIN32_FILE_ATTRIBUTE_DATA fileData;
            GetFileAttributesExW(BTL_LOGFILE_NAME, GetFileExInfoStandard, &fileData);
            SYSTEMTIME utcTime, createTime;
            FileTimeToSystemTime(&fileData.ftCreationTime, &utcTime);
            TIME_ZONE_INFORMATION tz;
            GetTimeZoneInformation(&tz);
            SystemTimeToTzSpecificLocalTime(&tz, &utcTime, &createTime);

            tm end;
            {
                auto in_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                localtime_s(&end, &in_time_t);
            }

            WCHAR dest[1024]; // we make sure the Prefix is not greater than 512, so this is more than enough.
            StringCbPrintfW(addr(dest), size_of(dest), gConfiguration.OnDiskFormat == OnDiskFormat::BTL ? DATE_FORMAT_BTL : DATE_FORMAT_ETL, gConfiguration.FileNamePrefix.c_str(), createTime.wYear, createTime.wMonth, createTime.wDay, createTime.wHour, createTime.wMinute, createTime.wSecond, end.tm_year + 1900, end.tm_mon + 1, end.tm_mday, end.tm_hour, end.tm_min, end.tm_sec);

            if (gConfiguration.OnDiskFormat == OnDiskFormat::BTL)
            {
                wprintf(L"  Writing '%s' ...", dest);
                if (CopyFileW(BTL_LOGFILE_NAME, dest, false) == 0)
                {
                    wprintf(L"Error copying %s to %s during file move: %d\r\n", BTL_LOGFILE_NAME, dest, GetLastError());
                }

                StringCbPrintfW(addr(dest), size_of(dest), DATE_FORMAT_BTL_ID, gConfiguration.FileNamePrefix.c_str(), createTime.wYear, createTime.wMonth, createTime.wDay, createTime.wHour, createTime.wMinute, createTime.wSecond, end.tm_year + 1900, end.tm_mon + 1, end.tm_mday, end.tm_hour, end.tm_min, end.tm_sec);
                if (CopyFileW(BTL_LOGFILE_NAME_ID, dest, false) == 0)
                {
                    wprintf(L"Error copying %s to %s during file move: %d\r\n", BTL_LOGFILE_NAME_ID, dest, GetLastError());
                }

                StringCbPrintfW(addr(dest), size_of(dest), DATE_FORMAT_BTL_MD, gConfiguration.FileNamePrefix.c_str(), createTime.wYear, createTime.wMonth, createTime.wDay, createTime.wHour, createTime.wMinute, createTime.wSecond, end.tm_year + 1900, end.tm_mon + 1, end.tm_mday, end.tm_hour, end.tm_min, end.tm_sec);
                if (CopyFileW(BTL_LOGFILE_NAME_MD, dest, false) == 0)
                {
                    wprintf(L"Error copying %s to %s during file move: %d\r\n", BTL_LOGFILE_NAME_MD, dest, GetLastError());
                }
            }
            else
            {
                wprintf(L"  Writing '%s' ...", dest);

                if (CopyFileW(ETL_LOGFILE_NAME, dest, false) == 0)
                {
                    wprintf(L"Error copying %s to %s during file move: %d\r\n", ETL_LOGFILE_NAME, dest, GetLastError());
                }
            }
        }

        EVENT_TRACE_PROPERTIES properties;
        ZeroMemory(&properties, sizeof(EVENT_TRACE_PROPERTIES));
        properties.Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES);
        ControlTrace(0, SessionName, &properties, EVENT_TRACE_CONTROL_STOP);

        EVENT_TRACE_PROPERTIES properties2;
        ZeroMemory(&properties2, sizeof(EVENT_TRACE_PROPERTIES));
        properties2.Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES);
        ControlTrace(0, BPerfSymbols, &properties2, EVENT_TRACE_CONTROL_STOP);

    default:
        break;
    }

    return FALSE;
}

void InjectVolumeMapInner(ITraceRelogger* relogger, LARGE_INTEGER *qpc, const wchar_t* deviceName, const wchar_t* path, UCHAR opcode)
{
    BYTE payload[64 * 1024];

    ITraceEvent *duplicateEvent;
    relogger->CreateEventInstance(0, EVENT_HEADER_FLAG_CLASSIC_HEADER | EVENT_HEADER_FLAG_64_BIT_HEADER, &duplicateEvent);

    EVENT_DESCRIPTOR eventDescriptor;
    eventDescriptor.Id = 0xFFFF;
    eventDescriptor.Version = 0;
    eventDescriptor.Channel = 0;
    eventDescriptor.Level = 1;
    eventDescriptor.Opcode = opcode;
    eventDescriptor.Task = 0;
    eventDescriptor.Keyword = 0;

    duplicateEvent->SetEventDescriptor(&eventDescriptor);
    duplicateEvent->SetTimeStamp(qpc);
    duplicateEvent->SetProcessorIndex(0);
    duplicateEvent->SetProviderId(&FakeSysConfigGuid);
    duplicateEvent->SetProcessId(0);
    duplicateEvent->SetThreadId(0);

    auto deviceNameSize = (wcslen(deviceName) + 1) * 2;
    auto pathSize = (wcslen(path) + 1) * 2;
    const auto payloadSize = deviceNameSize + pathSize;
    auto ptr = addr(payload);

    CopyMemory(ptr, deviceName, deviceNameSize);
    ptr += deviceNameSize;
    CopyMemory(ptr, path, pathSize);

    duplicateEvent->SetPayload(addr(payload), static_cast<ULONG>(payloadSize));
    relogger->Inject(duplicateEvent);
    duplicateEvent->Release();
}

void InjectVolumeMapInner(ITraceRelogger* relogger, LARGE_INTEGER *qpc, const wchar_t* deviceName, const wchar_t* path)
{
    InjectVolumeMapInner(relogger, qpc, deviceName, path, VolumeMappingOpcode);
}

DECLSPEC_NOINLINE void InjectVolumeMap(ITraceRelogger *relogger, LARGE_INTEGER *qpc, const std::unordered_map<std::wstring, std::wstring> *volumeMap, LARGE_INTEGER timestamp)
{
    qpc->QuadPart = timestamp.QuadPart;

    if (qpc->QuadPart != 0 && volumeMap != nullptr)
    {
        WCHAR systemRoot[MAX_PATH] = L"";
        GetSystemDirectoryW(addr(systemRoot), MAX_PATH);

        WCHAR windowsRoot[MAX_PATH] = L"";
        GetWindowsDirectoryW(addr(windowsRoot), MAX_PATH);
        windowsRoot[wcslen(windowsRoot)] = L'\\';

        InjectVolumeMapInner(relogger, qpc, addr(systemRoot), addr(windowsRoot), SystemPathsOpCode);
        InjectVolumeMapInner(relogger, qpc, L"\\??\\", L"");

        for (auto const& x : *volumeMap)
        {
            auto deviceName = x.first.c_str();
            auto path = x.second.c_str();

            if (wcslen(path) == 0)
            {
                continue;
            }

            InjectVolumeMapInner(relogger, qpc, deviceName, path);
        }

        InjectVolumeMapInner(relogger, qpc, L"\\Device\\LanmanRedirector\\", L"\\\\");
        InjectVolumeMapInner(relogger, qpc, L"\\Device\\Mup\\", L"\\\\");
        InjectVolumeMapInner(relogger, qpc, L"\\Device\\vmsmb\\", L"\\\\?\\VMSMB\\");
        InjectVolumeMapInner(relogger, qpc, L"\\\\", L"\\\\");
    }
}

HRESULT STDMETHODCALLTYPE TraceEventCallback::OnEvent(ITraceEvent *traceEvent, ITraceRelogger *relogger)
{
    PEVENT_RECORD eventRecord;
    traceEvent->GetEventRecord(&eventRecord);

    if (this->qpc.QuadPart == 0)
    {
        InjectVolumeMap(relogger, &this->qpc, this->volumeMap, eventRecord->EventHeader.TimeStamp);
    }

    // depending on the event being classic (or manifest) the event id can be the opcode or the actual id
    const USHORT eventId = (eventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_CLASSIC_HEADER) != 0 ? eventRecord->EventHeader.EventDescriptor.Opcode : eventRecord->EventHeader.EventDescriptor.Id;

    // Check if the schema was logged already, if not log handle that case.
    if (this->enableMetadata && this->eventMetadataLogged.find(EventKey(eventRecord->EventHeader.ProviderId, eventId, eventRecord->EventHeader.EventDescriptor.Version)) == this->eventMetadataLogged.end())
    {
        InjectKernelTraceMetadataEvents(this->eventMetadataLogged, eventRecord, relogger);
    }

    // Image Load events don't carry enough information about the PE file needed for symbolic lookup, i.e. it doesn't contain the RSDS information
    if (this->enableSymbolServerInfo && IsEqualGUID(eventRecord->EventHeader.ProviderId, ImageLoadGuid))
    {
        InjectKernelTraceControlEvents(eventRecord, relogger);
    }

    return relogger->Inject(traceEvent);
}

HRESULT STDMETHODCALLTYPE TraceEventCallback::OnBeginProcessTrace(ITraceEvent *header, ITraceRelogger *relogger)
{
    UNREFERENCED_PARAMETER(relogger);
    UNREFERENCED_PARAMETER(header);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE TraceEventCallback::OnFinalizeProcessTrace(ITraceRelogger *relogger)
{
    UNREFERENCED_PARAMETER(relogger);
    return S_OK;
}

struct ThreadComms
{
    ThreadComms(int rollOverTimeMSec, const wchar_t* sessionName, HANDLE scdSignal, BOOL *readConfigurationOnRestart, HANDLE handleForMainThread)
    {
        this->RollOverTimeMSec = rollOverTimeMSec;
        this->SessionName = sessionName;
        this->ScdSignal = scdSignal;
        this->ReadConfigurationOnRestart = readConfigurationOnRestart;
        this->HandleForMainThread = handleForMainThread;
    }

    int RollOverTimeMSec;
    const wchar_t* SessionName;
    HANDLE ScdSignal;
    BOOL*ReadConfigurationOnRestart;
    HANDLE HandleForMainThread;
};

struct ThreadComms2
{
    ThreadComms2(const wchar_t* sessionName, const wchar_t* dataDirectory, const std::unordered_set<std::wstring> * clrProcessNamesSet, const wchar_t* clrProcessNames, const std::unordered_map<std::wstring, std::wstring> * volumeMap, HANDLE handleForMainThread)
    {
        this->SessionName = sessionName;
        this->DataDirectory = dataDirectory;
        this->ClrProcessNamesSet = clrProcessNamesSet;
        this->ClrProcessNames = clrProcessNames;
        this->VolumeMap = volumeMap;
        this->HandleForMainThread = handleForMainThread;
    }

    LPCWSTR SessionName;
    LPCWSTR DataDirectory;
    LPCWSTR ClrProcessNames;
    const std::unordered_set<std::wstring> * ClrProcessNamesSet;
    const std::unordered_map<std::wstring, std::wstring> * VolumeMap;
    HANDLE HandleForMainThread;
};

DWORD WINAPI LogRotationAndSCDWaitThread(LPVOID lpParameter)
{
    auto threadComms = (ThreadComms*)lpParameter;

    *threadComms->ReadConfigurationOnRestart = FALSE;
    if (WaitForSingleObject(threadComms->ScdSignal, threadComms->RollOverTimeMSec) == WAIT_OBJECT_0)
    {
        ResetEvent(threadComms->ScdSignal);
        wprintf(L"  New configuration file change detected, stopping current session and flushing data.\n\n");
        *threadComms->ReadConfigurationOnRestart = TRUE;
    }

    EVENT_TRACE_PROPERTIES properties;
    ZeroMemory(&properties, sizeof(EVENT_TRACE_PROPERTIES));
    properties.Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES);
    ControlTrace(0, threadComms->SessionName, &properties, EVENT_TRACE_CONTROL_STOP);

    SetEvent(threadComms->HandleForMainThread);

    return 0;
}

HRESULT RelogTraceFileETL(const wchar_t* outputFileName, int rollOverTimeMSec, BSTR sessionName, HANDLE scdSignal, BOOL *readConfigurationOnRestart, BOOL enableMetadata, BOOL enableSymbolServerInfo, const std::unordered_map<std::wstring, std::wstring> * volumeMap)
{
    ITraceRelogger *relogger;

    HRESULT hr = CoCreateInstance(CLSID_TraceRelogger, nullptr, CLSCTX_INPROC_SERVER, __uuidof(ITraceRelogger), reinterpret_cast<LPVOID *>(&relogger));
    if (FAILED(hr))
    {
        wprintf(L"Failed to instantiate ITraceRelogger object: 0x%08x.\n", hr);
        return hr;
    }

    TRACEHANDLE traceHandle;

    TraceEventCallback callback(enableMetadata, enableSymbolServerInfo, volumeMap);

    relogger->SetOutputFilename(BSTR(outputFileName));
    relogger->RegisterCallback(&callback);
    relogger->AddRealtimeTraceStream(sessionName, nullptr, &traceHandle);

    ThreadComms threadComms(rollOverTimeMSec, sessionName, scdSignal, readConfigurationOnRestart, INVALID_HANDLE_VALUE);

    if (rollOverTimeMSec != 0)
    {
        threadComms.HandleForMainThread = CreateEvent(nullptr, TRUE, FALSE, L"HandleForMainThreadETL");
        QueueUserWorkItem(&LogRotationAndSCDWaitThread, &threadComms, WT_EXECUTELONGFUNCTION);
    }

    relogger->ProcessTrace();

    if (rollOverTimeMSec != 0)
    {
        WaitForSingleObject(threadComms.HandleForMainThread, INFINITE);
        CloseHandle(threadComms.HandleForMainThread);
    }

    if (relogger != nullptr)
    {
        relogger->Release();
    }

    return hr;
}

HRESULT RelogTraceFileBTL(PEVENT_RECORD_CALLBACK eventRecordCallback, PBYTE buffer, PBYTE compressionBuffer, PBYTE workspace, const wchar_t *sessionName, const wchar_t *dataFileName, const wchar_t *indexFileName, const wchar_t *metadataFileName, FILETIME startTime, LARGE_INTEGER perfFreq, LARGE_INTEGER qpc, int rollOverTimeMSec, HANDLE scdSignal, BOOL *readConfigurationOnRestart, BOOL enableMetadata, BOOL enableSymbolServerInfo, const std::unordered_map<std::wstring, std::wstring> * volumeMap)
{
    const auto dataFile = SafeFileHandle(CreateFile(dataFileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (dataFile.Get() == INVALID_HANDLE_VALUE)
    {
        wprintf(L"CreateFile failed for %s with %lu\n", dataFileName, GetLastError());
        return E_HANDLE;
    }

    const auto indexFile = SafeFileHandle(CreateFile(indexFileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (indexFile.Get() == INVALID_HANDLE_VALUE)
    {
        wprintf(L"CreateFile failed for %s with %lu\n", indexFileName, GetLastError());
        return E_HANDLE;
    }

    LONGLONG zero = 0;

    WriteFile(indexFile.Get(), &startTime, sizeof(FILETIME), nullptr, nullptr);
    WriteFile(indexFile.Get(), &perfFreq, sizeof(LARGE_INTEGER), nullptr, nullptr);
    WriteFile(indexFile.Get(), &qpc, sizeof(LARGE_INTEGER), nullptr, nullptr);
    WriteFile(indexFile.Get(), &zero, sizeof(LONGLONG), nullptr, nullptr);

    const auto metadataFile = SafeFileHandle(CreateFile(metadataFileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (metadataFile.Get() == INVALID_HANDLE_VALUE)
    {
        wprintf(L"CreateFile failed for %s with %lu\n", metadataFileName, GetLastError());
        return E_HANDLE;
    }

    auto fakeRelogger = FakeTraceRelogger(static_cast<PEVENT_RECORD_CALLBACK>(eventRecordCallback));
    ContextInfo contextInfo(buffer, compressionBuffer, workspace, dataFile.Get(), indexFile.Get(), metadataFile.Get(), &fakeRelogger, enableMetadata, enableSymbolServerInfo, volumeMap);
    fakeRelogger.SetUserContext(&contextInfo);

    EVENT_TRACE_LOGFILE trace;

    ZeroMemory(&trace, sizeof(EVENT_TRACE_LOGFILE));
    trace.LoggerName = (LPWSTR)sessionName;
    trace.LogFileName = nullptr;
    trace.EventRecordCallback = static_cast<PEVENT_RECORD_CALLBACK>(eventRecordCallback);
    trace.BufferCallback = reinterpret_cast<PEVENT_TRACE_BUFFER_CALLBACK>(BufferCallback);
    trace.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_RAW_TIMESTAMP | PROCESS_TRACE_MODE_EVENT_RECORD;
    trace.Context = &contextInfo;

    TRACEHANDLE traceHandle = OpenTrace(&trace);
    if (INVALID_PROCESSTRACE_HANDLE == traceHandle)
    {
        wprintf(L"OpenTrace failed with %lu\n", GetLastError());
        return E_HANDLE;
    }

    contextInfo.PerfFreq = perfFreq.QuadPart;
    contextInfo.LastEventIndexedTimeStamp = qpc.QuadPart;

    EVENT_RECORD eventTraceHeader = {0};
    eventTraceHeader.EventHeader.TimeStamp = qpc;
    eventTraceHeader.EventHeader.Flags = 800;
    eventTraceHeader.EventHeader.EventDescriptor.Version = 2;
    eventTraceHeader.EventHeader.ProviderId = {0x68FDD900, 0x4A3E, 0x11D1, {0x84, 0xF4, 0x00, 0x00, 0xF8, 0x04, 0x64, 0xE3}};
    eventTraceHeader.UserContext = &contextInfo;

#pragma pack(push, 1)
    struct FAKE_TRACE_LOGFILE_HEADER
    {
        ULONG BufferSize; // Logger buffer size in Kbytes
        union {
            ULONG Version; // Logger version
            struct
            {
                UCHAR MajorVersion;
                UCHAR MinorVersion;
                UCHAR SubVersion;
                UCHAR SubMinorVersion;
            } VersionDetail;
        } DUMMYUNIONNAME;
        ULONG ProviderVersion;    // defaults to NT version
        ULONG NumberOfProcessors; // Number of Processors
        LARGE_INTEGER EndTime;    // Time when logger stops
        ULONG TimerResolution;    // assumes timer is constant!!!
        ULONG MaximumFileSize;    // Maximum in Mbytes
        ULONG LogFileMode;        // specify logfile mode
        ULONG BuffersWritten;     // used to file start of Circular File
        union {
            GUID LogInstanceGuid; // For RealTime Buffer Delivery
            struct
            {
                ULONG StartBuffers;  // Count of buffers written at start.
                ULONG PointerSize;   // Size of pointer type in bits
                ULONG EventsLost;    // Events losts during log session
                ULONG CpuSpeedInMHz; // Cpu Speed in MHz
            } ExtraInfo;
        } DUMMYUNIONNAME2;
#if defined(_WMIKM_)
        PWCHAR LoggerName;
        PWCHAR LogFileName;
        RTL_TIME_ZONE_INFORMATION TimeZone;
#else
        LONGLONG LoggerName;
        LONG LogFileName;
        TIME_ZONE_INFORMATION TimeZone;
#endif

        LARGE_INTEGER BootTime;
        LARGE_INTEGER PerfFreq;  // Reserved
        LARGE_INTEGER StartTime; // Reserved
        ULONG ReservedFlags;     // ClockType
        ULONG BuffersLost;
    };

    struct FakeHeader
    {
        FAKE_TRACE_LOGFILE_HEADER OrigHdr;
        CHAR SpaceForTwoEmptyUnicodeStrings[4];
    };
#pragma pack(pop)

    FakeHeader fakeHeader = {0};

    fakeHeader.OrigHdr.BufferSize = trace.LogfileHeader.BufferSize;
    fakeHeader.OrigHdr.Version = trace.LogfileHeader.Version;
    fakeHeader.OrigHdr.ProviderVersion = trace.LogfileHeader.ProviderVersion;
    fakeHeader.OrigHdr.NumberOfProcessors = trace.LogfileHeader.NumberOfProcessors;
    fakeHeader.OrigHdr.TimerResolution = trace.LogfileHeader.TimerResolution;
    fakeHeader.OrigHdr.ExtraInfo.PointerSize = trace.LogfileHeader.PointerSize;
    fakeHeader.OrigHdr.ExtraInfo.CpuSpeedInMHz = trace.LogfileHeader.CpuSpeedInMHz;
    fakeHeader.OrigHdr.TimeZone = trace.LogfileHeader.TimeZone;
    fakeHeader.OrigHdr.BootTime = trace.LogfileHeader.BootTime;
    fakeHeader.OrigHdr.PerfFreq = trace.LogfileHeader.PerfFreq;
    fakeHeader.OrigHdr.StartTime.HighPart = startTime.dwHighDateTime;
    fakeHeader.OrigHdr.StartTime.LowPart = startTime.dwLowDateTime;

    eventTraceHeader.UserData = &fakeHeader;
    eventTraceHeader.UserDataLength = sizeof(FakeHeader);

    ProcessEvent(&eventTraceHeader);

    LARGE_INTEGER fakeQpc = { 0 };
    InjectVolumeMap(&fakeRelogger, &fakeQpc, volumeMap, qpc);

    ThreadComms threadComms(rollOverTimeMSec, sessionName, scdSignal, readConfigurationOnRestart, INVALID_HANDLE_VALUE);

    if (rollOverTimeMSec != 0)
    {
        threadComms.HandleForMainThread = CreateEvent(nullptr, TRUE, FALSE, L"HandleForMainThreadBTL");
        QueueUserWorkItem(&LogRotationAndSCDWaitThread, &threadComms, WT_EXECUTELONGFUNCTION);
    }

    const DWORD status = ProcessTrace(&traceHandle, 1, nullptr, nullptr);
    if (status != ERROR_SUCCESS && status != ERROR_CANCELLED)
    {
        wprintf(L"ProcessTrace failed with %lu\n", status);
    }

    if(rollOverTimeMSec != 0)
    {
        WaitForSingleObject(threadComms.HandleForMainThread, INFINITE);
        CloseHandle(threadComms.HandleForMainThread);
    }

    if (INVALID_PROCESSTRACE_HANDLE != traceHandle)
    {
        CloseTrace(traceHandle);
    }

    return S_OK;
}

BOOL Step(PEVENT_TRACE_PROPERTIES sessionPropertiesBuffer, int sessionPropertiesBufferSize, PBYTE buffer, PBYTE compressionBuffer, PBYTE workspace, const Configuration &configuration, BOOL printProvidersEnabled, HANDLE scdSignal, BOOL *readConfigurationDataOnRestart, void (*fileMovedCallback)(const int length, const wchar_t *))
{
    EVENT_TRACE_TIME_PROFILE_INFORMATION timeInfo;
    timeInfo.EventTraceInformationClass = TraceStackTracingInfo;

    if (CtrlCTriggered)
    {
        timeInfo.ProfileInterval = static_cast<int>(10000.0 + .5); // set back to 1ms
        NtSetSystemInformation(SystemPerformanceTraceInformation, &timeInfo, sizeof(EVENT_TRACE_TIME_PROFILE_INFORMATION));

        wprintf(L"BPerf exiting in response to Ctrl+C.");
        return FALSE;
    }

    FillEventProperties(sessionPropertiesBuffer, sessionPropertiesBufferSize, configuration.BufferSizeInKB, (LPWSTR)configuration.SessionName.c_str(), EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_SYSTEM_LOGGER_MODE | EVENT_TRACE_INDEPENDENT_SESSION_MODE);

    TRACEHANDLE sessionHandle;

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    FILETIME startTime;
    GetSystemTimeAsFileTime(&startTime);

    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);

    HRESULT hr = StartSession(&sessionHandle, sessionPropertiesBuffer, (LPWSTR)configuration.SessionName.c_str());
    if (FAILED(HRESULT(hr)))
    {
        wprintf(L"StartTrace() failed with %lu\n", hr);
        return FALSE;
    }

    ULONG data = configuration.KernelEnableFlags;
    hr = TraceSetInformation(sessionHandle, TraceSystemTraceEnableFlagsInfo, &data, sizeof(ULONG));

    if (hr != ERROR_SUCCESS)
    {
        wprintf(L"Failed to set trace information: 0x%08x.\n", hr);
        return FALSE;
    }

    for (int i = 0; i < configuration.ProviderList.size(); ++i)
    {
        auto &provider = configuration.ProviderList[i];
        hr = EnableTraceEx2Shim(sessionHandle, &provider.Guid, provider.Level, provider.MatchAnyKeyword, provider.MatchAllKeyword, 0, provider.ExeNamesFilter.empty() ? nullptr : provider.ExeNamesFilter.c_str(), provider.StackEventIds.data(), (USHORT)provider.StackEventIds.size(), provider.ExcludeEventIds.data(), (USHORT)provider.ExcludeEventIds.size());

        if (hr != ERROR_SUCCESS)
        {
            wprintf(L"Failed to enable event provider '%s': ", provider.Name.c_str());
            printGuid(provider.Guid);
            wprintf(L". Error: 0x%08x.\n", hr);

            return FALSE;
        }
        else
        {
            if (printProvidersEnabled)
            {
                wprintf(L"  Successfully enabled '%s', with GUID: ", provider.Name.c_str());
                printGuid(provider.Guid);
                wprintf(L", Level: %d, MatchAnyKeyword: %d, ExeFilters: %s\n", provider.Level, provider.MatchAnyKeyword, provider.ExeNamesFilter.c_str());
            }
        }
    }

    printf("\n");

    hr = TraceSetInformation(sessionHandle, TraceStackTracingInfo, (PVOID)configuration.ClassicEventIdsStackList.data(), (ULONG)(sizeof(CLASSIC_EVENT_ID) * configuration.ClassicEventIdsStackList.size()));

    if (hr != ERROR_SUCCESS)
    {
        wprintf(L"Failed to set stack trace information: 0x%08x.\n", hr);
        return FALSE;
    }

    timeInfo.EventTraceInformationClass = TraceStackTracingInfo;
    timeInfo.ProfileInterval = static_cast<int>(configuration.CPUSamplingRateInMS * 10000.0 + .5);
    hr = NtSetSystemInformation(SystemPerformanceTraceInformation, &timeInfo, sizeof(EVENT_TRACE_TIME_PROFILE_INFORMATION)); // set interval to 10ms
    if (hr != ERROR_SUCCESS)
    {
        wprintf(L"Failed to set profile timer info\r\n");
    }

    tm begin;
    {
        auto in_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        localtime_s(&begin, &in_time_t);
    }

    if (configuration.OnDiskFormat == OnDiskFormat::BTL)
    {
        RelogTraceFileBTL(ProcessEvent, buffer, compressionBuffer, workspace, configuration.SessionName.c_str(), BTL_LOGFILE_NAME, BTL_LOGFILE_NAME_ID, BTL_LOGFILE_NAME_MD, startTime, frequency, qpc, configuration.LogRotationIntervalInSeconds * 1000, scdSignal, readConfigurationDataOnRestart, configuration.EnableEmbeddingEventMetadata, configuration.EnableEmbeddingSymbolServerKeyInfo, &configuration.VolumeMap);
    }
    else
    {
        RelogTraceFileETL(ETL_LOGFILE_NAME, configuration.LogRotationIntervalInSeconds * 1000, configuration.SessionNameBSTR, scdSignal, readConfigurationDataOnRestart, configuration.EnableEmbeddingEventMetadata, configuration.EnableEmbeddingSymbolServerKeyInfo, &configuration.VolumeMap);
    }

    tm end;
    {
        auto in_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        localtime_s(&end, &in_time_t);
    }

    WCHAR dest[1024]; // we make sure the Prefix is not greater than 512, so this is more than enough.
    StringCbPrintfW(addr(dest), size_of(dest), configuration.OnDiskFormat == OnDiskFormat::BTL ? DATE_FORMAT_BTL : DATE_FORMAT_ETL, configuration.FileNamePrefix.c_str(), begin.tm_year + 1900, begin.tm_mon + 1, begin.tm_mday, begin.tm_hour, begin.tm_min, begin.tm_sec, end.tm_year + 1900, end.tm_mon + 1, end.tm_mday, end.tm_hour, end.tm_min, end.tm_sec);

    if (configuration.OnDiskFormat == OnDiskFormat::BTL)
    {
        wprintf(L"  Writing '%s' ...", dest);
        if (MoveFileW(BTL_LOGFILE_NAME, dest) == 0)
        {
            wprintf(L"Error moving %s to %s during file move: %d\r\n", BTL_LOGFILE_NAME, dest, GetLastError());
        }

        StringCbPrintfW(addr(dest), size_of(dest), DATE_FORMAT_BTL_ID, configuration.FileNamePrefix.c_str(), begin.tm_year + 1900, begin.tm_mon + 1, begin.tm_mday, begin.tm_hour, begin.tm_min, begin.tm_sec, end.tm_year + 1900, end.tm_mon + 1, end.tm_mday, end.tm_hour, end.tm_min, end.tm_sec);
        if (MoveFileW(BTL_LOGFILE_NAME_ID, dest) == 0)
        {
            wprintf(L"Error moving %s to %s during file move: %d\r\n", BTL_LOGFILE_NAME_ID, dest, GetLastError());
        }

        StringCbPrintfW(addr(dest), size_of(dest), DATE_FORMAT_BTL_MD, configuration.FileNamePrefix.c_str(), begin.tm_year + 1900, begin.tm_mon + 1, begin.tm_mday, begin.tm_hour, begin.tm_min, begin.tm_sec, end.tm_year + 1900, end.tm_mon + 1, end.tm_mday, end.tm_hour, end.tm_min, end.tm_sec);
        if (MoveFileW(BTL_LOGFILE_NAME_MD, dest) == 0)
        {
            wprintf(L"Error moving %s to %s during file move: %d\r\n", BTL_LOGFILE_NAME_MD, dest, GetLastError());
        }
    }
    else
    {
        wprintf(L"  Writing '%s' ...", dest);

        if (MoveFileW(ETL_LOGFILE_NAME, dest) == 0)
        {
            wprintf(L"Error moving %s to %s during file move: %d\r\n", ETL_LOGFILE_NAME, dest, GetLastError());
        }
    }

    if (fileMovedCallback != nullptr)
    {
        fileMovedCallback(static_cast<int>(wcslen(dest)), dest);
    }

    wprintf(L" Done\n");

    return TRUE;
}

void InvokeBPerfStop()
{
    CtrlCTriggered = true;
    EVENT_TRACE_PROPERTIES properties;
    ZeroMemory(&properties, sizeof(EVENT_TRACE_PROPERTIES));
    properties.Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES);
    ControlTrace(0, SessionName, &properties, EVENT_TRACE_CONTROL_STOP);
}

HRESULT ReadAndSetupConfiguration(LPWSTR iniFilePathPtr, Configuration &configuration)
{
    auto providerList = ReadETWSessionStringListData(L"Providers", iniFilePathPtr);

    if (providerList.size() == 0)
    {
        wprintf(L"  ERROR: No provider list found. You must have at least one provider.\n");
        return E_FAIL;
    }

    auto classicEventIdsStackList = ReadETWSessionStringListData(L"ClassicEventIdsStack", iniFilePathPtr);

    configuration.SessionName = ReadETWSessionStringData(L"Name", iniFilePathPtr);

    if (configuration.SessionName.empty())
    {
        wprintf(L"  ERROR: The 'Name' property is required in the '[ETWSession]' section in your configuration file.\n");
        return E_FAIL;
    }

    configuration.SessionNameBSTR = SysAllocString(configuration.SessionName.c_str());

    configuration.FileNamePrefix = ReadETWSessionStringData(L"FileNamePrefix", iniFilePathPtr);
    if (configuration.FileNamePrefix.empty())
    {
        wprintf(L"  ERROR: The 'FileNamePrefix' property is required in the '[ETWSession]' section in your configuration file.\n");
        return E_FAIL;
    }

    if (configuration.FileNamePrefix.length() > 512)
    {
        wprintf(L"  ERROR: The 'FileNamePrefix' property is greater than the 512 max limit. Please consider a shorter prefix.\n");
        return E_FAIL;
    }

    auto onDiskFormat = ReadETWSessionStringData(L"OnDiskFormat", iniFilePathPtr);
    if (onDiskFormat == std::wstring(L"BTL"))
    {
        configuration.OnDiskFormat = OnDiskFormat::BTL;
    }
    else if (onDiskFormat == std::wstring(L"ETL"))
    {
        configuration.OnDiskFormat = OnDiskFormat::ETL;
    }
    else
    {
        wprintf(L"  ERROR: The 'OnDiskFormat' property must either be 'BTL' or 'ETL'.\n");
        return E_FAIL;
    }

    configuration.BufferSizeInKB = ReadETWSessionIntData(L"BufferSizeInKB", iniFilePathPtr, 64);
    configuration.LogRotationIntervalInSeconds = ReadETWSessionIntData(L"LogRotationIntervalInSeconds", iniFilePathPtr, 900);
    configuration.CPUSamplingRateInMS = ReadETWSessionIntData(L"CPUSamplingRateInMS", iniFilePathPtr, 1);
    configuration.KernelEnableFlags = ReadETWSessionIntData(L"KernelEnableFlags", iniFilePathPtr, 0);
    configuration.EnableEmbeddingEventMetadata = ReadETWSessionIntData(L"EnableEmbeddingEventMetadata", iniFilePathPtr, 1);
    configuration.EnableEmbeddingSymbolServerKeyInfo = ReadETWSessionIntData(L"EnableEmbeddingSymbolServerKeyInfo", iniFilePathPtr, 1);
    configuration.EnableMetadataSideStream = ReadETWSessionIntData(L"EnableMetadataSideStream", iniFilePathPtr, 1);
    configuration.DataDirectory = ReadETWSessionStringData(L"DataDirectory", iniFilePathPtr);
    configuration.EnableCLRSymbolCollection = ReadETWSessionIntData(L"EnableCLRSymbolCollection", iniFilePathPtr, 0);

    if (configuration.EnableCLRSymbolCollection)
    {
        std::unordered_set<std::wstring> set;

        configuration.CLRSymbolCollectionProcessNames = ReadETWSessionStringData(L"CLRSymbolCollectionProcessNames", iniFilePathPtr);

        for (auto &s : split(configuration.CLRSymbolCollectionProcessNames, ';'))
        {
            set.insert(s);
        }

        configuration.CLRSymbolCollectionProcessNamesSet = set;
    }

    std::wstring expandedDataDirectory(32767, '0');
    auto retSize = ExpandEnvironmentStrings(configuration.DataDirectory.c_str(), &expandedDataDirectory[0], 32767);
    expandedDataDirectory.resize(retSize);

    auto fileAttr = GetFileAttributes(expandedDataDirectory.c_str());
    if (0xFFFFFFFF == fileAttr)
    {
        auto err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
        {
            wprintf(L"  ERROR: Data Directory '%s' does not exist.", expandedDataDirectory.c_str());
        }
        else
        {
            wprintf(L"  ERROR: Data Directory '%s' unknown error getting file attributes: %d", expandedDataDirectory.c_str(), fileAttr);
        }

        return E_FAIL;
    }

    configuration.DataDirectory = expandedDataDirectory;

    if (configuration.DataDirectory.empty())
    {
        wprintf(L"  ERROR: The 'DataDirectory' property is required in the '[ETWSession]' section in your configuration file.\n");
        return E_FAIL;
    }

    for (size_t i = 0; i < providerList.size(); ++i)
    {
        auto providerSectionName = providerList[i].c_str();

        std::wstring temp(255, '0');

        if (GetPrivateProfileSection(providerSectionName, &temp[0], 255, iniFilePathPtr) == 0)
        {
            wprintf(L"  ERROR: Section '[%s]' not found in the configuration File.", providerSectionName);
            return E_FAIL;
        }

        Provider provider;

        auto guidStr = ReadStringData(providerSectionName, L"Guid", iniFilePathPtr);
        if (guidStr.size() != 38 || guidStr[0] != '{' || guidStr[37] != '}' || guidStr[9] != '-' || guidStr[14] != '-' || guidStr[19] != '-' || guidStr[24] != '-')
        {
            wprintf(L"  ERROR: Section '[%s]', Property 'Guid' is missing and/or does not appear to be a GUID. Must the be format of: {2CB15D1D-5FC1-11D2-ABE1-00A0C911F518}", providerSectionName);
            return E_FAIL;
        }

        provider.Name = providerList[i];
        provider.Guid = StringToGuid(guidStr);
        provider.Level = (BYTE)ReadIntData((LPWSTR)providerSectionName, L"Level", iniFilePathPtr, 5);
        provider.MatchAnyKeyword = ReadIntData((LPWSTR)providerSectionName, L"MatchAnyKeyword", iniFilePathPtr, 0);
        provider.MatchAllKeyword = ReadIntData((LPWSTR)providerSectionName, L"MatchAllKeyword", iniFilePathPtr, 0);
        provider.ExeNamesFilter = ReadStringData(providerSectionName, L"ExeNamesFilter", iniFilePathPtr);

        {
            std::wstringstream ss(ReadStringData(providerSectionName, L"StackEventIdsFilter", iniFilePathPtr));

            int x;

            while (ss >> x)
            {
                provider.StackEventIds.push_back((USHORT)x);

                if (ss.peek() == ',' || ss.peek() == ' ')
                {
                    ss.ignore();
                }
            }
        }

        {
            std::wstringstream ss(ReadStringData(providerSectionName, L"ExcludeEventIdsFilter", iniFilePathPtr));

            int x;

            while (ss >> x)
            {
                provider.ExcludeEventIds.push_back((USHORT)x);

                if (ss.peek() == ',' || ss.peek() == ' ')
                {
                    ss.ignore();
                }
            }
        }

        provider.EnableStacksForAllEventIds = ReadIntData((LPWSTR)providerSectionName, L"EnableStacksForAllEventIds", iniFilePathPtr, 0);
        configuration.ProviderList.push_back(provider);
    }

    for (size_t i = 0; i < classicEventIdsStackList.size(); ++i)
    {
        CLASSIC_EVENT_ID classicEventId;

        auto guidStr = ReadStringData(classicEventIdsStackList[i].c_str(), L"Guid", iniFilePathPtr);
        if (guidStr.size() != 38 || guidStr[0] != '{' || guidStr[37] != '}' || guidStr[9] != '-' || guidStr[14] != '-' || guidStr[19] != '-' || guidStr[24] != '-')
        {
            wprintf(L"  ERROR: Section '[%s]', Property 'Guid' is missing and/or does not appear to be a GUID. Must the be format of: {2CB15D1D-5FC1-11D2-ABE1-00A0C911F518}", classicEventIdsStackList[i].c_str());
            return E_FAIL;
        }

        classicEventId.EventGuid = StringToGuid(guidStr);
        auto typeVal = ReadIntData((LPWSTR)classicEventIdsStackList[i].c_str(), L"Type", iniFilePathPtr, 256);
        classicEventId.Type = (UCHAR)typeVal;

        if (typeVal > 255)
        {
            wprintf(L"  ERROR: Section '[%s]', Property 'Type' has an invalid value, must be less than 255", classicEventIdsStackList[i].c_str());
            return E_FAIL;
        }

        configuration.ClassicEventIdsStackList.push_back(classicEventId);
    }

    SessionName = (LPWSTR)configuration.SessionName.c_str(); // alive throughout the program

    /* set working dir */
    SetCurrentDirectoryW(configuration.DataDirectory.c_str());

    WCHAR systemRoot[MAX_PATH] = L"";
    GetWindowsDirectoryW(addr(systemRoot), MAX_PATH);
    systemRoot[wcslen(systemRoot)] = L'\\';
    configuration.VolumeMap.insert(std::make_pair(std::wstring(L"\\SystemRoot\\"), std::wstring(systemRoot)));

    WCHAR volumePathName[32768];
    WCHAR deviceName[MAX_PATH] = L"";
    WCHAR volumeName[MAX_PATH] = L"";
    auto findHandle = FindFirstVolumeW(addr(volumeName), (DWORD)size_of(volumeName));
    if (findHandle == INVALID_HANDLE_VALUE)
    {
        wprintf(L"FindFirstVolumeW failed with error code %d\n", GetLastError());
        return S_OK;
    }

    size_t index;
    for (;;)
    {
        index = wcslen(volumeName) - 1;

        if (volumeName[0] != L'\\' ||
            volumeName[1] != L'\\' ||
            volumeName[2] != L'?' ||
            volumeName[3] != L'\\' ||
            volumeName[index] != L'\\')
        {
            wprintf(L"FindFirstVolumeW/FindNextVolumeW returned a bad path: %d\n", ERROR_BAD_PATHNAME);
            break;
        }

        volumeName[index] = L'\0';

        if (QueryDosDeviceW(&volumeName[4], addr(deviceName), (DWORD)size_of(deviceName)) == 0)
        {
            wprintf(L"QueryDosDeviceW failed with error code %d\n", GetLastError());
            break;
        }

        volumeName[index] = L'\\';

        DWORD returnedLength = 0;
        if (GetVolumePathNamesForVolumeNameW(volumeName, addr(volumePathName), (DWORD)size_of(volumePathName), &returnedLength) == 0)
        {
            wprintf(L"GetVolumePathNamesForVolumeNameW failed with error code %d\n", GetLastError());
            continue;
        }

        deviceName[wcslen(deviceName)] = L'\\';

        configuration.VolumeMap.insert(std::make_pair(std::wstring(deviceName), std::wstring(volumePathName)));

        if (FindNextVolumeW(findHandle, addr(volumeName), (DWORD)size_of(volumePathName)) == 0)
        {
            break;
        }
    }

    FindVolumeClose(findHandle);

    return S_OK;
}

DWORD WINAPI StartCLRSymbolicDataSession(LPVOID lpParameter)
{
    auto threadComms = (ThreadComms2*)lpParameter;
    auto sessionName = threadComms->SessionName;
    auto dataDirectory = threadComms->DataDirectory;
    auto clrProcessNamesSet = threadComms->ClrProcessNamesSet;
    auto clrProcessNames = threadComms->ClrProcessNames;
    auto volumeMap = threadComms->VolumeMap;

    auto sessionNameSizeInBytes = (wcslen(sessionName) + 1) * sizeof(WCHAR);
    auto bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sessionNameSizeInBytes + sizeof(WCHAR); // for '\0'
    BYTE sessionPropertiesBuffer[1024];
    PEVENT_TRACE_PROPERTIES sessionProperties = (PEVENT_TRACE_PROPERTIES)addr(sessionPropertiesBuffer);

    FillEventProperties(sessionProperties, (ULONG)bufferSize, 64, sessionName, EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_INDEPENDENT_SESSION_MODE);

    TRACEHANDLE sessionHandle = 0;

    StartSession(&sessionHandle, sessionProperties, sessionName);

    HRESULT hr = EnableTraceEx2Shim(sessionHandle, &ClrProviderGuid, TRACE_LEVEL_VERBOSE, CLRJITANDLOADERKEYWORD, 0, 0, clrProcessNames, nullptr, 0, nullptr, 0);

    if (hr != ERROR_SUCCESS)
    {
        return hr;
    }

    hr = EnableTraceEx2Shim(sessionHandle, &KernelProcessGuid, TRACE_LEVEL_VERBOSE, PROCESSANDIMAGELOADKEYWORD, 0, 0, clrProcessNames, nullptr, 0, nullptr, 0);

    if (hr != ERROR_SUCCESS)
    {
        return hr;
    }

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    FILETIME startTime;
    GetSystemTimeAsFileTime(&startTime);

    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);

    ContextInfo2 contextInfo(startTime, qpc, dataDirectory, clrProcessNamesSet, volumeMap);

    EVENT_TRACE_LOGFILE trace;

    ZeroMemory(&trace, sizeof(EVENT_TRACE_LOGFILE));
    trace.LoggerName = (LPWSTR)sessionName;
    trace.LogFileName = nullptr;
    trace.EventRecordCallback = static_cast<PEVENT_RECORD_CALLBACK>(ProcessEventWithSymbols);
    trace.BufferCallback = reinterpret_cast<PEVENT_TRACE_BUFFER_CALLBACK>(BufferCallback);
    trace.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_RAW_TIMESTAMP | PROCESS_TRACE_MODE_EVENT_RECORD;
    trace.Context = &contextInfo;

    TRACEHANDLE traceHandle = OpenTrace(&trace);
    if (INVALID_PROCESSTRACE_HANDLE == traceHandle)
    {
        hr = GetLastError();
        wprintf(L"OpenTrace failed with %lu\n", hr);
        return hr;
    }

    const DWORD status = ProcessTrace(&traceHandle, 1, nullptr, nullptr);
    if (status != ERROR_SUCCESS && status != ERROR_CANCELLED)
    {
        wprintf(L"ProcessTrace failed with %lu\n", status);
    }

    if (INVALID_PROCESSTRACE_HANDLE != traceHandle)
    {
        CloseTrace(traceHandle);
    }

    SetEvent(threadComms->HandleForMainThread);

    return ERROR_SUCCESS;
}

int InvokeBPerfStart(const int argc, PWSTR *argv, bool setCtrlCHandler, void (*fileMovedCallback)(const int length, const wchar_t *))
{
    RtlCompressBuffer = reinterpret_cast<RtlCompressBuffer_Fn>(GetProcAddress(LoadLibrary(L"ntdll.dll"), "RtlCompressBuffer"));
    NtSetSystemInformation = reinterpret_cast<NtSetSystemInformation_Fn>(GetProcAddress(LoadLibrary(L"ntdll.dll"), "NtSetSystemInformation"));
    CtrlCTriggered = false;

    if (argc != 2)
    {
        printf("[ Usage                      : BPerfCPUSamplesCollector ConfigurationFilePath ]\n\n");
        printf("[ Example Configuration File : https://dev.azure.com/msasg/BPerf/_git/CPUSamplesCollector?path=/BPerfCPUSamplesCollector.ini&version=GBmaster\n");

        return ERROR_INVALID_PARAMETER;
    }

    if (setCtrlCHandler)
    {
        SetConsoleCtrlHandler(ConsoleControlHandler, true);
    }

    HRESULT hr;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        wprintf(L"ERROR: Failed to initialize COM: 0x%08x\n", hr);
        return HRESULT_CODE(hr);
    }

    if (!AcquireProfilePrivileges())
    {
        wprintf(L"ERROR: Failed to acquire profiling privileges. Are you running as admin?\n");
        return E_FAIL;
    }

    std::wstring iniFilePath(32767, '0');
    auto retSize = GetFullPathName(argv[1], 4096, &iniFilePath[0], nullptr);
    iniFilePath.resize(retSize);

    size_t pos = iniFilePath.find_last_of(L"\\");
    auto iniFilePathDirectory = std::wstring::npos == pos ? std::wstring(L"") : iniFilePath.substr(0, pos);

    LPWSTR iniFilePathPtr = (LPWSTR)iniFilePath.c_str();

    DWORD fileAttr;

    fileAttr = GetFileAttributes(iniFilePathPtr);
    if (0xFFFFFFFF == fileAttr && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        wprintf(L"ERROR: Configuration File '%s' does not exist", iniFilePathPtr);
        return E_FAIL;
    }
    else
    {
        wprintf(L"\n  Configuration File Path: '%s'\n", iniFilePathPtr);
    }

    hr = ReadAndSetupConfiguration(iniFilePathPtr, gConfiguration);

    if (hr != S_OK)
    {
        return hr;
    }

    wprintf(L"\n  BPerfCPUSamplesCollector is running ...\n");

    wprintf(L"  Profiling Timer Speed : %dms\n", gConfiguration.CPUSamplingRateInMS);
    wprintf(L"  File Rotation Period  : %ds\n", gConfiguration.LogRotationIntervalInSeconds);
    wprintf(L"  ETW Session Name: %s\n", gConfiguration.SessionName.c_str());
    wprintf(L"  Profiles Directory Location: %s\n", gConfiguration.DataDirectory.c_str());

    wprintf(L"\n  *** NOTE *** -> Profile file names start with ''");
    wprintf(gConfiguration.FileNamePrefix.c_str());
    wprintf(L"'' followed by timestamp\n\n");

    const auto buffer = static_cast<PBYTE>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressionBufferSize));
    const auto compressionBuffer = static_cast<PBYTE>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressionBufferSize));
    const auto workspace = static_cast<PBYTE>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressionBufferSize));

    BOOL printProvidersEnabled = TRUE;

    auto scdSignal = FindFirstChangeNotification(iniFilePathDirectory.c_str(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
    BOOL readConfigurationDataOnRestart;

    auto clrProcessNamesSet = gConfiguration.CLRSymbolCollectionProcessNamesSet;
    auto clrProcessNames = gConfiguration.CLRSymbolCollectionProcessNames;
    auto dataDirectory = gConfiguration.DataDirectory;

    ThreadComms2 threadComms2(BPerfSymbols, dataDirectory.c_str(), &clrProcessNamesSet, clrProcessNames.c_str(), &gConfiguration.VolumeMap, INVALID_HANDLE_VALUE);

    if (gConfiguration.EnableCLRSymbolCollection)
    {
        threadComms2.HandleForMainThread = CreateEvent(nullptr, TRUE, FALSE, L"HandleForMainThreadSymbols");
        QueueUserWorkItem(&StartCLRSymbolicDataSession, &threadComms2, WT_EXECUTELONGFUNCTION);
    }

    BYTE sessionPropertiesBuffer[1024];

    while (true)
    {
        readConfigurationDataOnRestart = FALSE;
        const auto continueIteration = Step((PEVENT_TRACE_PROPERTIES)sessionPropertiesBuffer, sizeof(EVENT_TRACE_PROPERTIES) + sizeof(LOGFILE_PATH) + (int)(gConfiguration.SessionName.size() + 1) * 2, buffer, compressionBuffer, workspace, gConfiguration, printProvidersEnabled, scdSignal, &readConfigurationDataOnRestart, fileMovedCallback);
        printProvidersEnabled = FALSE;
        if (!continueIteration)
        {
            break;
        }

        if (readConfigurationDataOnRestart)
        {
            Configuration newConfiguration;
            hr = ReadAndSetupConfiguration(iniFilePathPtr, newConfiguration);
            if (hr == S_OK)
            {
                gConfiguration = newConfiguration;
            }
            else
            {
                wprintf(L"  Non-Fatal ERROR: New Configuration Parsing failed. (%d)", hr);
            }

            FindNextChangeNotification(scdSignal);
        }
    }

    if (gConfiguration.EnableCLRSymbolCollection)
    {
        WaitForSingleObject(threadComms2.HandleForMainThread, INFINITE);
        CloseHandle(threadComms2.HandleForMainThread);
    }

    FindCloseChangeNotification(scdSignal);

    HeapFree(GetProcessHeap(), 0, buffer);
    HeapFree(GetProcessHeap(), 0, compressionBuffer);
    HeapFree(GetProcessHeap(), 0, workspace);

    EVENT_TRACE_PROPERTIES properties;
    ZeroMemory(&properties, sizeof(EVENT_TRACE_PROPERTIES));
    properties.Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES);
    ControlTrace(0, gConfiguration.SessionName.c_str(), &properties, EVENT_TRACE_CONTROL_STOP);

    EVENT_TRACE_PROPERTIES properties2;
    ZeroMemory(&properties2, sizeof(EVENT_TRACE_PROPERTIES));
    properties2.Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES);
    ControlTrace(0, BPerfSymbols, &properties2, EVENT_TRACE_CONTROL_STOP);

    CoUninitialize();
    return HRESULT_CODE(hr);
 }
