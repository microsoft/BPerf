#include <windows.h>
#include <evntcons.h>
#include <string>
#include <relogger.h>
#include <evntrace.h>
#include <strsafe.h>
#include <tdh.h>

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

constexpr UCHAR DCStartOpCode = 3;
constexpr UCHAR DCStopOpCode = 4;

#define SystemRoot L"\\SystemRoot"
#define GlobalRoot L"\\\\?\\GLOBALROOT"

#define MAX_EVENT_SIZE 65536
#define HALF_EVENT_SIZE 32768

#define addr(x) &x[0]

template <typename T, size_t N>
constexpr size_t countOf(T const (&)[N]) noexcept
{
    return N;
}

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
    wchar_t* FileName;

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
        this->FileName = reinterpret_cast<wchar_t *>(data);
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
constexpr GUID EventMetadataGuid = {0xbbccf6c1, 0x6cd1, 0x48c4, {0x80, 0xff, 0x83, 0x94, 0x82, 0xe3, 0x76, 0x71}};

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
    BYTE payload[MAX_EVENT_SIZE];

    ITraceEvent *duplicateEvent;
    relogger->CreateEventInstance(0, flags, &duplicateEvent);

    EVENT_DESCRIPTOR eventDescriptor;
    eventDescriptor.Id = 0xFFFF;
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
    const auto payloadSize = eventPointerSize + pdbFileNameBufferLength + 2 * sizeof(int) + sizeof(GUID);
    auto ptr = addr(payload);

    CopyMemory(ptr, eventRecord->UserData, eventPointerSize);
    ptr += eventPointerSize;
    CopyMemory(ptr, &processId, sizeof(int));
    ptr += sizeof(int);
    CopyMemory(ptr, &data->Signature, sizeof(GUID));
    ptr += sizeof(GUID);
    CopyMemory(ptr, &data->Age, sizeof(int));
    ptr += sizeof(int);
    CopyMemory(ptr, data->PdbFileName, pdbFileNameBufferLength);

    duplicateEvent->SetPayload(addr(payload), static_cast<ULONG>(payloadSize));
    relogger->Inject(duplicateEvent);
    duplicateEvent->Release();
}

void InjectImgIdEvent(ITraceRelogger *relogger, PEVENT_RECORD eventRecord, PCWCHAR originalFileName, DWORD imageSize, DWORD timestamp, int eventPointerSize, int flags, DWORD processId)
{
    BYTE payload[MAX_EVENT_SIZE];

    ITraceEvent *duplicateEvent;
    relogger->CreateEventInstance(0, flags, &duplicateEvent);

    EVENT_DESCRIPTOR eventDescriptor;
    eventDescriptor.Id = 0xFFFF;
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
    const auto payloadSize = eventPointerSize + originalFileNameBufferLength + 4 * sizeof(int);
    auto ptr = addr(payload);

    DWORD *zeroAddr = nullptr;
    CopyMemory(ptr, eventRecord->UserData, eventPointerSize);
    ptr += eventPointerSize;
    CopyMemory(ptr, &imageSize, sizeof(int));
    ptr += sizeof(int);
    CopyMemory(ptr, &zeroAddr, sizeof(int));
    ptr += sizeof(int);
    CopyMemory(ptr, &processId, sizeof(int));
    ptr += sizeof(int);
    CopyMemory(ptr, &timestamp, sizeof(int));
    ptr += sizeof(int);
    CopyMemory(ptr, originalFileName, originalFileNameBufferLength);

    duplicateEvent->SetPayload(addr(payload), static_cast<ULONG>(payloadSize));
    relogger->Inject(duplicateEvent);
    duplicateEvent->Release();
}

HRESULT InjectKernelTraceControlEventsInner(PCWCHAR peFileName, PCWCHAR originalFileName, ITraceRelogger *relogger, PEVENT_RECORD eventRecord, DWORD processId)
{
    const auto file = CreateFile(peFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return ERROR_INVALID_HANDLE;
    }

    const auto mapFile = CreateFileMapping(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (mapFile == nullptr)
    {
        CloseHandle(file);
        return ERROR_INVALID_HANDLE;
    }

    const auto imageBase = reinterpret_cast<PBYTE>(MapViewOfFile(mapFile, FILE_MAP_READ, 0, 0, 0));
    const auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(imageBase);
    auto ntHeader = reinterpret_cast<PIMAGE_NT_HEADERS32>(imageBase + dosHeader->e_lfanew);

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE || ntHeader->Signature != IMAGE_NT_SIGNATURE)
    {
        UnmapViewOfFile(imageBase);
        CloseHandle(mapFile);
        CloseHandle(file);
        return ERROR_BAD_ARGUMENTS;
    }

    auto flags = EVENT_HEADER_FLAG_CLASSIC_HEADER;
    int eventPointerSize;
    DWORD sizeOfImage;
    IMAGE_DATA_DIRECTORY debugDescriptorDirectory;
    if ((eventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) != 0)
    {
        auto ntHeader64 = reinterpret_cast<PIMAGE_NT_HEADERS64>(imageBase + dosHeader->e_lfanew);
        sizeOfImage = ntHeader64->OptionalHeader.SizeOfImage;
        debugDescriptorDirectory = ntHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
        eventPointerSize = 8;
        flags |= EVENT_HEADER_FLAG_64_BIT_HEADER;
    }
    else
    {
        sizeOfImage = ntHeader->OptionalHeader.SizeOfImage;
        debugDescriptorDirectory = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
        eventPointerSize = 4;
        flags |= EVENT_HEADER_FLAG_32_BIT_HEADER;
    }

    InjectImgIdEvent(relogger, eventRecord, originalFileName, sizeOfImage, ntHeader->FileHeader.TimeDateStamp, eventPointerSize, flags, processId);

    if (debugDescriptorDirectory.VirtualAddress != 0)
    {
        auto debugDirectories = (PIMAGE_DEBUG_DIRECTORY)(imageBase + RVAToFileOffset(ntHeader, debugDescriptorDirectory.VirtualAddress));
        ULONG numberOfDebugDirectories = debugDescriptorDirectory.Size / sizeof(IMAGE_DEBUG_DIRECTORY);

        for (PIMAGE_DEBUG_DIRECTORY current = debugDirectories; numberOfDebugDirectories != 0; --numberOfDebugDirectories, ++current)
        {
            if (current->Type == IMAGE_DEBUG_TYPE_CODEVIEW)
            {
                const auto data = reinterpret_cast<PCV_INFO_PDB70>(imageBase + current->PointerToRawData);

                if (data->CvSignature == 0x53445352) // RSDS in ASCII
                {
                    InjectDbgIdEvent(relogger, eventRecord, data, eventPointerSize, flags, processId);
                }

                break;
            }
        }
    }

    UnmapViewOfFile(imageBase);
    CloseHandle(mapFile);
    CloseHandle(file);
    return S_OK;
}

template <typename T>
void PEFileNameAndProcessIdFromEvent(PEVENT_RECORD eventRecord, DWORD *processId, wchar_t* ptr, size_t size)
{
    ImageLoad<T> imgLoadEvent(static_cast<PBYTE>(eventRecord->UserData));
    *processId = imgLoadEvent.ProcessId;

    if (_wcsnicmp(imgLoadEvent.FileName, SystemRoot, length_of(SystemRoot)) == 0)
    {
        WCHAR windowsDirectory[(MAX_PATH + 1) * 2];
        GetWindowsDirectory(addr(windowsDirectory), MAX_PATH);

        StringCbPrintfW(ptr, size, L"%s%s", addr(windowsDirectory), imgLoadEvent.FileName + length_of(SystemRoot));
    }
    else
    {
        StringCbPrintfW(ptr, size, L"%s%s", GlobalRoot, imgLoadEvent.FileName);
    }
}

__declspec(noinline) HRESULT InjectKernelTraceControlEvents(PEVENT_RECORD eventRecord, ITraceRelogger *relogger)
{
    {
        WCHAR peFileNamePtr[HALF_EVENT_SIZE];
        BYTE buffer[HALF_EVENT_SIZE];

        DWORD processId = 0;
        if ((eventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) != 0)
        {
            PEFileNameAndProcessIdFromEvent<uint64_t>(eventRecord, &processId, addr(peFileNamePtr), size_of(peFileNamePtr));
        }
        else
        {
            PEFileNameAndProcessIdFromEvent<uint32_t>(eventRecord, &processId, addr(peFileNamePtr), size_of(peFileNamePtr));
        }

        DWORD handle;
        const DWORD versionSize = GetFileVersionInfoSize(peFileNamePtr, &handle);
        if (versionSize > 0)
        {
            if (GetFileVersionInfo(peFileNamePtr, handle, versionSize, addr(buffer)) != 0)
            {
                struct LANGANDCODEPAGE
                {
                    WORD wLanguage;
                    WORD wCodePage;
                } * lpTranslate;

                UINT cbTranslate;

                VerQueryValue(addr(buffer), TEXT("\\VarFileInfo\\Translation"), reinterpret_cast<LPVOID *>(&lpTranslate), &cbTranslate);

                for (unsigned i = 0; i < cbTranslate / sizeof(LANGANDCODEPAGE); ++i)
                {
                    WCHAR dest[MAX_PATH + 1];
                    PWCHAR lpBuffer;
                    UINT dwBytes;

                    StringCchPrintf(dest, MAX_PATH, TEXT("\\StringFileInfo\\%04x%04x\\OriginalFilename"), lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
                    VerQueryValue(addr(buffer), dest, reinterpret_cast<LPVOID *>(&lpBuffer), &dwBytes);

                    if (dwBytes == 0)
                    {
                        StringCchPrintf(dest, MAX_PATH, TEXT("\\StringFileInfo\\%04x%04x\\InternalName"), lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
                        VerQueryValue(addr(buffer), dest, reinterpret_cast<LPVOID *>(&lpBuffer), &dwBytes);

                        if (dwBytes == 0)
                        {
                            lpBuffer = const_cast<PWCHAR>(peFileNamePtr);
                        }
                    }

                    const LPWSTR lastDot = wcsrchr(lpBuffer, L'.');

                    if (lastDot == nullptr)
                    {
                        auto lpBuffer2 = lpBuffer + wcslen(lpBuffer);
                        lpBuffer2[0] = L'.';
                        lpBuffer2[1] = L'd';
                        lpBuffer2[2] = L'l';
                        lpBuffer2[3] = L'l';
                        lpBuffer2[4] = L'\0';
                    }
                    else
                    {
                        if (lastDot && !_wcsicmp(lastDot, L".MUI"))
                        {
                            *lastDot = UNICODE_NULL;
                        }
                    }

                    auto strippedPeFile = wcsrchr(lpBuffer, '\\');
                    if (strippedPeFile == nullptr)
                    {
                        strippedPeFile = lpBuffer;
                    }
                    else
                    {
                        strippedPeFile += 1;
                    }

                    InjectKernelTraceControlEventsInner(peFileNamePtr, strippedPeFile, relogger, eventRecord, processId);
                }
            }
        }
        else
        {
            auto strippedPeFile = wcsrchr(peFileNamePtr, '\\');
            if (strippedPeFile == nullptr)
            {
                strippedPeFile = peFileNamePtr;
            }
            else
            {
                strippedPeFile += 1;
            }

            InjectKernelTraceControlEventsInner(peFileNamePtr, strippedPeFile, relogger, eventRecord, processId);
        }
    }

    return S_OK;
}

__declspec(noinline) HRESULT InjectTraceEventInfoEvent(PEVENT_RECORD eventRecord, ITraceRelogger *relogger)
{
    ULONG bufferSize = 0;
    TDHSTATUS status;
    BYTE buffer[MAX_EVENT_SIZE];

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
        auto flags = EVENT_HEADER_FLAG_CLASSIC_HEADER;
        if ((eventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) != 0)
        {
            flags |= EVENT_HEADER_FLAG_64_BIT_HEADER;
        }
        else
        {
            flags |= EVENT_HEADER_FLAG_32_BIT_HEADER;
        }

        ITraceEvent *duplicateEvent;
        relogger->CreateEventInstance(0, flags, &duplicateEvent);

        EVENT_DESCRIPTOR eventDescriptor;
        eventDescriptor.Id = 0xFFFF;
        eventDescriptor.Version = 0;
        eventDescriptor.Channel = 0;
        eventDescriptor.Level = 0;
        eventDescriptor.Opcode = 32;
        eventDescriptor.Task = 0;
        eventDescriptor.Keyword = 0;

        duplicateEvent->SetEventDescriptor(&eventDescriptor);

        duplicateEvent->SetProcessorIndex(GetEventProcessorIndex(eventRecord));
        duplicateEvent->SetProviderId(&EventMetadataGuid);
        duplicateEvent->SetTimeStamp(&eventRecord->EventHeader.TimeStamp);
        duplicateEvent->SetProcessId(eventRecord->EventHeader.ProcessId);
        duplicateEvent->SetThreadId(eventRecord->EventHeader.ThreadId);

        duplicateEvent->SetPayload(addr(buffer), static_cast<ULONG>(bufferSize));
        relogger->Inject(duplicateEvent);
        duplicateEvent->Release();
    }

    return S_OK;
}