#include <windows.h>
#include <evntcons.h>
#include <dbghelp.h>
#include "TraceEventCallback.h"

#define SystemRoot L"\\SystemRoot"
#define GlobalRoot L"\\\\?\\GLOBALROOT"

std::wstring InitWindowsDir()
{
    auto dest = static_cast<PWCHAR>(malloc((MAX_PATH + 1) * 2));
    GetWindowsDirectory(dest, MAX_PATH);
    std::wstring windowsDirectory(dest);
    free(dest);
    return windowsDirectory;
}

static std::wstring &&windowsDirectory = InitWindowsDir();

auto ReadInt32(PBYTE &data)
{
    uint32_t retVal;
    char *p = reinterpret_cast<char *>(&retVal);

    p[0] = data[0];
    p[1] = data[1];
    p[2] = data[2];
    p[3] = data[3];

    data += 4;

    return retVal;
}

auto ReadInt64(PBYTE &data)
{
    uint64_t retVal;
    char *p = reinterpret_cast<char *>(&retVal);

    p[0] = data[0];
    p[1] = data[1];
    p[2] = data[2];
    p[3] = data[3];
    p[4] = data[4];
    p[5] = data[5];
    p[6] = data[6];
    p[7] = data[7];

    data += 8;

    return retVal;
}

template <class T>
T ReadPointer(PBYTE &data)
{
    UNREFERENCED_PARAMETER(data);
    throw std::logic_error("T must be uint64_t or uint32_t");
}

template <>
uint64_t ReadPointer<uint64_t>(PBYTE &data)
{
    return ReadInt64(data);
}

template <>
uint32_t ReadPointer<uint32_t>(PBYTE &data)
{
    return ReadInt32(data);
}

template <class PtrSizeType>
class ImageLoad
{
  public:
    PtrSizeType ImageBase;
    PtrSizeType ImageSize;
    uint32_t ProcessId;
    uint32_t ImageChecksum;
    uint32_t TimeDateStamp;
    uint32_t Reserved0;
    PtrSizeType DefaultBase;
    uint32_t Reserved1;
    uint32_t Reserved2;
    uint32_t Reserved3;
    uint32_t Reserved4;
    std::wstring FileName;

    ImageLoad(PBYTE data)
    {
        this->ImageBase = ReadPointer<PtrSizeType>(data);
        this->ImageSize = ReadPointer<PtrSizeType>(data);
        this->ProcessId = ReadInt32(data);
        this->ImageChecksum = ReadInt32(data);
        this->TimeDateStamp = ReadInt32(data);
        this->Reserved0 = ReadInt32(data);
        this->DefaultBase = ReadPointer<PtrSizeType>(data);
        this->Reserved1 = ReadInt32(data);
        this->Reserved2 = ReadInt32(data);
        this->Reserved3 = ReadInt32(data);
        this->Reserved4 = ReadInt32(data);
        this->FileName = wstring(reinterpret_cast<const wchar_t *>(data));
    }
};

typedef struct _CV_INFO_PDB70
{
    DWORD CvSignature;
    GUID Signature;
    DWORD Age;
    char PdbFileName[1];
} CV_INFO_PDB70, *PCV_INFO_PDB70;

constexpr GUID KernelTraceControlGuid = {0xB3E675D7, 0x2554, 0x4F18, {0x83, 0x0B, 0x27, 0x62, 0x73, 0x25, 0x60, 0xDE}};

template <typename T, size_t N>
constexpr size_t count_of(T (&)[N])
{
    return N;
}

template <typename T, size_t N>
constexpr size_t size_of(T (&)[N])
{
    return N * sizeof(T);
}

template <typename T, size_t N>
constexpr size_t length_of(T (&)[N])
{
    return N - 1;
}

void InjectDbgIdEvent(ITraceRelogger *relogger, PEVENT_RECORD eventRecord, PCV_INFO_PDB70 data, int eventPointerSize, int flags, DWORD processId)
{
    ITraceEvent *duplicateEvent;
    relogger->CreateEventInstance(0, flags, &duplicateEvent);

    EVENT_DESCRIPTOR eventDescriptor;
    eventDescriptor.Id = -1;
    eventDescriptor.Version = 2;
    eventDescriptor.Channel = 0;
    eventDescriptor.Level = 1;
    eventDescriptor.Opcode = 36;
    eventDescriptor.Task = 0;
    eventDescriptor.Keyword = 0;

    duplicateEvent->SetEventDescriptor(&eventDescriptor);

    duplicateEvent->SetTimeStamp(&eventRecord->EventHeader.TimeStamp);
    duplicateEvent->SetProcessorIndex(GetEventProcessorIndex(eventRecord));
    duplicateEvent->SetProviderId(&KernelTraceControlGuid);
    duplicateEvent->SetProcessId(processId);
    duplicateEvent->SetThreadId(eventRecord->EventHeader.ThreadId);

    auto pdbFileNameBufferLength = static_cast<int>(strlen(data->PdbFileName)) + 1;
    auto payloadSize = eventPointerSize + pdbFileNameBufferLength + 2 * sizeof(int) + sizeof(GUID);
    auto ptr = static_cast<PBYTE>(malloc(payloadSize));
    auto payload = ptr;

    CopyMemory(ptr, eventRecord->UserData, eventPointerSize);
    ptr += eventPointerSize;
    CopyMemory(ptr, &eventRecord->EventHeader.ProcessId, sizeof(int));
    ptr += sizeof(int);
    CopyMemory(ptr, &data->Signature, sizeof(GUID));
    ptr += sizeof(GUID);
    CopyMemory(ptr, &data->Age, sizeof(int));
    ptr += sizeof(int);
    CopyMemory(ptr, data->PdbFileName, pdbFileNameBufferLength);

    duplicateEvent->SetPayload(payload, static_cast<ULONG>(payloadSize));
    relogger->Inject(duplicateEvent);
    duplicateEvent->Release();
    free(payload);
}

void InjectImgIdEvent(ITraceRelogger *relogger, PEVENT_RECORD eventRecord, PCWCHAR originalFileName, DWORD imageSize, DWORD timestamp, int eventPointerSize, int flags, DWORD processId)
{
    ITraceEvent *duplicateEvent;
    relogger->CreateEventInstance(0, flags, &duplicateEvent);

    EVENT_DESCRIPTOR eventDescriptor;
    eventDescriptor.Id = -1;
    eventDescriptor.Version = 2;
    eventDescriptor.Channel = 0;
    eventDescriptor.Level = 1;
    eventDescriptor.Opcode = 0;
    eventDescriptor.Task = 0;
    eventDescriptor.Keyword = 0;

    duplicateEvent->SetEventDescriptor(&eventDescriptor);

    duplicateEvent->SetTimeStamp(&eventRecord->EventHeader.TimeStamp);
    duplicateEvent->SetProcessorIndex(GetEventProcessorIndex(eventRecord));
    duplicateEvent->SetProviderId(&KernelTraceControlGuid);
    duplicateEvent->SetProcessId(processId);
    duplicateEvent->SetThreadId(eventRecord->EventHeader.ThreadId);

    auto originalFileNameBufferLength = (wcslen(originalFileName) + 1) * 2;
    auto payloadSize = eventPointerSize + originalFileNameBufferLength + 4 * sizeof(int);
    auto ptr = static_cast<PBYTE>(malloc(payloadSize));
    auto payload = ptr;

    DWORD *zeroAddr = nullptr;
    CopyMemory(ptr, eventRecord->UserData, eventPointerSize);
    ptr += eventPointerSize;
    CopyMemory(ptr, &imageSize, sizeof(int));
    ptr += sizeof(int);
    CopyMemory(ptr, &zeroAddr, sizeof(int));
    ptr += sizeof(int);
    CopyMemory(ptr, &eventRecord->EventHeader.ProcessId, sizeof(int));
    ptr += sizeof(int);
    CopyMemory(ptr, &timestamp, sizeof(int));
    ptr += sizeof(int);
    CopyMemory(ptr, originalFileName, originalFileNameBufferLength);

    duplicateEvent->SetPayload(payload, static_cast<ULONG>(payloadSize));
    relogger->Inject(duplicateEvent);
    duplicateEvent->Release();
    free(payload);
}

HRESULT InjectKernelTraceControlEventsInner(PCWCHAR peFileName, PCWCHAR originalFileName, ITraceRelogger *relogger, PEVENT_RECORD eventRecord, DWORD processId)
{
    auto file = CreateFile(peFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return ERROR_INVALID_HANDLE;
    }

    auto mapFile = CreateFileMapping(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (mapFile == nullptr)
    {
        CloseHandle(file);
        return ERROR_INVALID_HANDLE;
    }

    auto imageBase = reinterpret_cast<PBYTE>(MapViewOfFile(mapFile, FILE_MAP_READ, 0, 0, 0));
    auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(imageBase);
    auto ntHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(imageBase + dosHeader->e_lfanew);
    auto header = &ntHeader->OptionalHeader;

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE || ntHeader->Signature != IMAGE_NT_SIGNATURE)
    {
        CloseHandle(mapFile);
        CloseHandle(file);
        return ERROR_BAD_ARGUMENTS;
    }

    auto flags = EVENT_HEADER_FLAG_CLASSIC_HEADER;
    int eventPointerSize;
    if ((eventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) != 0)
    {
        eventPointerSize = 8;
        flags |= EVENT_HEADER_FLAG_64_BIT_HEADER;
    }
    else
    {
        eventPointerSize = 4;
        flags |= EVENT_HEADER_FLAG_32_BIT_HEADER;
    }

    InjectImgIdEvent(relogger, eventRecord, originalFileName, header->SizeOfImage, ntHeader->FileHeader.TimeDateStamp, eventPointerSize, flags, processId);

    ULONG numberOfDebugDirectories;
    auto debugDirectories = static_cast<PIMAGE_DEBUG_DIRECTORY>(ImageDirectoryEntryToData(imageBase, FALSE, IMAGE_DIRECTORY_ENTRY_DEBUG, &numberOfDebugDirectories));
    numberOfDebugDirectories /= sizeof(*debugDirectories);

    PIMAGE_DEBUG_DIRECTORY current;
    for (current = debugDirectories; numberOfDebugDirectories != 0; --numberOfDebugDirectories, ++current)
    {
        if (current->Type == IMAGE_DEBUG_TYPE_CODEVIEW)
        {
            auto data = reinterpret_cast<PCV_INFO_PDB70>(imageBase + current->PointerToRawData);

            if (data->CvSignature == 0x53445352) // RSDS in ASCII
            {
                InjectDbgIdEvent(relogger, eventRecord, data, eventPointerSize, flags, processId);
            }

            break;
        }
    }

    CloseHandle(mapFile);
    CloseHandle(file);
    return S_OK;
}

template <typename T>
wstring PEFileNameAndProcessIdFromEvent(PEVENT_RECORD eventRecord, DWORD *processId)
{
    ImageLoad<T> imgLoadEvent(static_cast<PBYTE>(eventRecord->UserData));
    *processId = imgLoadEvent.ProcessId;
    wstring peFileName(imgLoadEvent.FileName.compare(0, length_of(SystemRoot), SystemRoot) == 0 ? windowsDirectory + imgLoadEvent.FileName.substr(length_of(SystemRoot)) : GlobalRoot + imgLoadEvent.FileName);
    return peFileName;
}

__declspec(noinline) HRESULT InjectKernelTraceControlEvents(PEVENT_RECORD eventRecord, ITraceRelogger *relogger)
{
    if (eventRecord->EventHeader.EventDescriptor.Opcode != DCStopOpCode)
    {
        return S_OK;
    }

    DWORD processId = 0;
    wstring peFileName((eventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) != 0 ? PEFileNameAndProcessIdFromEvent<uint64_t>(eventRecord, &processId) : PEFileNameAndProcessIdFromEvent<uint32_t>(eventRecord, &processId));
    auto peFileNamePtr = peFileName.c_str();

    DWORD handle;
    DWORD versionSize = GetFileVersionInfoSize(peFileNamePtr, &handle);
    if (versionSize > 0)
    {
        auto buffer = malloc(versionSize);

        if (GetFileVersionInfo(peFileNamePtr, handle, versionSize, buffer) != 0)
        {
            struct LANGANDCODEPAGE
            {
                WORD wLanguage;
                WORD wCodePage;
            } * lpTranslate;

            UINT cbTranslate;

            VerQueryValue(buffer, TEXT("\\VarFileInfo\\Translation"), reinterpret_cast<LPVOID *>(&lpTranslate), &cbTranslate);

            for (unsigned i = 0; i < cbTranslate / sizeof(LANGANDCODEPAGE); ++i)
            {
                WCHAR dest[MAX_PATH + 1];
                PWCHAR lpBuffer;
                UINT dwBytes;

                StringCchPrintf(dest, MAX_PATH, TEXT("\\StringFileInfo\\%04x%04x\\OriginalFilename"), lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
                VerQueryValue(buffer, dest, reinterpret_cast<LPVOID *>(&lpBuffer), &dwBytes);

                if (dwBytes == 0)
                {
                    StringCchPrintf(dest, MAX_PATH, TEXT("\\StringFileInfo\\%04x%04x\\InternalName"), lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
                    VerQueryValue(buffer, dest, reinterpret_cast<LPVOID *>(&lpBuffer), &dwBytes);

                    if (dwBytes == 0)
                    {
                        lpBuffer = (PWCHAR)peFileNamePtr;
                    }
                }

                const LPWSTR lastDot = wcsrchr(lpBuffer, L'.');
                if (lastDot && !_wcsicmp(lastDot, L".MUI"))
                {
                    *lastDot = UNICODE_NULL;
                }

                InjectKernelTraceControlEventsInner(peFileNamePtr, lpBuffer, relogger, eventRecord, processId);
            }
        }

        free(buffer);
    }
    else
    {
        InjectKernelTraceControlEventsInner(peFileNamePtr, peFileNamePtr, relogger, eventRecord, processId);
    }

    return S_OK;
}