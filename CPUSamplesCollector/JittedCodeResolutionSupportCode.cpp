#include <relogger.h>

#define MAX_EVENT_SIZE 65536
#define ULONG_SIZE 4
#define LARGE_INTEGER_SIZE 8
#define ULONGLONGSIZE 8
#define USHORT_SIZE 2
#define GUID_SIZE 16
#define UCHAR_SIZE 1

BYTE userData[MAX_EVENT_SIZE];

bool Read(PBYTE buf, LARGE_INTEGER &retVal, DWORD &bytesAvailable)
{
    if (bytesAvailable >= LARGE_INTEGER_SIZE)
    {
        buf -= bytesAvailable;
        retVal.LowPart = buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0];
        retVal.HighPart = buf[7] << 24 | buf[6] << 16 | buf[5] << 8 | buf[4];
        bytesAvailable -= LARGE_INTEGER_SIZE;
        return true;
    }

    return false;
}

bool Read(PBYTE buf, ULONGLONG &retVal, DWORD &bytesAvailable)
{
    if (bytesAvailable >= ULONGLONGSIZE)
    {
        buf -= bytesAvailable;
        retVal = static_cast<ULONGLONG>(buf[0]) | static_cast<ULONGLONG>(buf[1]) << 8 | static_cast<ULONGLONG>(buf[2]) << 16 | static_cast<ULONGLONG>(buf[3]) << 24 | static_cast<ULONGLONG>(buf[4]) << 32 | static_cast<ULONGLONG>(buf[5]) << 40 | static_cast<ULONGLONG>(buf[6]) << 48 | static_cast<ULONGLONG>(buf[7]) << 56;
        bytesAvailable -= ULONGLONGSIZE;
        return true;
    }

    return false;
}

bool Read(PBYTE buf, ULONG &retVal, DWORD &bytesAvailable)
{
    if (bytesAvailable >= ULONG_SIZE)
    {
        buf -= bytesAvailable;
        retVal = buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0];
        bytesAvailable -= ULONG_SIZE;
        return true;
    }

    return false;
}

bool Read(PBYTE buf, USHORT &retVal, DWORD &bytesAvailable)
{
    if (bytesAvailable >= USHORT_SIZE)
    {
        buf -= bytesAvailable;
        retVal = buf[1] << 8 | buf[0];
        bytesAvailable -= USHORT_SIZE;
        return true;
    }

    return false;
}

bool Read(PBYTE buf, UCHAR &retVal, DWORD &bytesAvailable)
{
    if (bytesAvailable >= UCHAR_SIZE)
    {
        buf -= bytesAvailable;
        retVal = buf[0];
        bytesAvailable -= UCHAR_SIZE;
        return true;
    }

    return false;
}

bool Read(PBYTE buf, GUID &retVal, DWORD &bytesAvailable)
{
    if (bytesAvailable >= GUID_SIZE)
    {
        buf -= bytesAvailable;
        retVal.Data1 = buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0];
        retVal.Data2 = buf[5] << 8 | buf[4];
        retVal.Data3 = buf[7] << 8 | buf[6];
        retVal.Data4[0] = buf[8];
        retVal.Data4[1] = buf[9];
        retVal.Data4[2] = buf[10];
        retVal.Data4[3] = buf[11];
        retVal.Data4[4] = buf[12];
        retVal.Data4[5] = buf[13];
        retVal.Data4[6] = buf[14];
        retVal.Data4[7] = buf[15];

        bytesAvailable -= GUID_SIZE;
        return true;
    }

    return false;
}

int i = 0;

bool ReadAndRelogEvent(ITraceRelogger *relogger, PBYTE buf, DWORD bytesAvailable, DWORD &bytesProcessed)
{
    bytesProcessed = 0;
    DWORD bytesAvailableABeginningOfFunction = bytesAvailable;
    buf += bytesAvailableABeginningOfFunction;

    LARGE_INTEGER timestamp;
    ULONG processId;
    ULONG threadId;
    GUID providerId;

    USHORT eventId;
    UCHAR version;
    UCHAR level;
    UCHAR channel;
    UCHAR opcode;
    USHORT task;
    ULONGLONG keyword;

    USHORT userDataLength;

    if (!Read(buf, timestamp, bytesAvailable))
    {
        return false;
    }

    if (!Read(buf, processId, bytesAvailable))
    {
        return false;
    }

    if (!Read(buf, threadId, bytesAvailable))
    {
        return false;
    }

    if (!Read(buf, providerId, bytesAvailable))
    {
        return false;
    }

    if (!Read(buf, eventId, bytesAvailable))
    {
        return false;
    }

    if (!Read(buf, version, bytesAvailable))
    {
        return false;
    }

    if (!Read(buf, level, bytesAvailable))
    {
        return false;
    }

    if (!Read(buf, channel, bytesAvailable))
    {
        return false;
    }

    if (!Read(buf, opcode, bytesAvailable))
    {
        return false;
    }

    if (!Read(buf, task, bytesAvailable))
    {
        return false;
    }

    if (!Read(buf, keyword, bytesAvailable))
    {
        return false;
    }

    if (!Read(buf, userDataLength, bytesAvailable))
    {
        return false;
    }

    if (bytesAvailable >= userDataLength)
    {
        buf -= bytesAvailable;
        CopyMemory(userData, buf, userDataLength);
        bytesAvailable -= userDataLength;
    }
    else
    {
        return false;
    }

    ITraceEvent *evt;
    relogger->CreateEventInstance(0, 0, &evt);

    EVENT_DESCRIPTOR eventDescriptor;
    eventDescriptor.Id = eventId;
    eventDescriptor.Version = version;
    eventDescriptor.Level = level;
    eventDescriptor.Channel = channel;
    eventDescriptor.Opcode = opcode;
    eventDescriptor.Task = task;
    eventDescriptor.Keyword = keyword;

    evt->SetProviderId(&providerId);
    evt->SetTimeStamp(&timestamp);
    evt->SetProcessId(processId);
    evt->SetThreadId(threadId);
    evt->SetEventDescriptor(&eventDescriptor);
    evt->SetPayload(userData, userDataLength);

    relogger->Inject(evt);

    evt->Release();

    bytesProcessed = bytesAvailableABeginningOfFunction - bytesAvailable;
    return true;
}

void LogJittedCodeSymbolicData(HANDLE hFile, ITraceRelogger *relogger)
{
    LARGE_INTEGER fileSize;
    auto disk_buffer = static_cast<PBYTE>(malloc(MAX_EVENT_SIZE));
    auto memory_buffer = static_cast<PBYTE>(malloc(MAX_EVENT_SIZE));

    if (GetFileSizeEx(hFile, &fileSize) == FALSE)
    {
        return;
    }

    LONGLONG remainingFileSize = fileSize.QuadPart;

    while (remainingFileSize > 0)
    {
        DWORD bytesToRead = static_cast<DWORD>(min(remainingFileSize, MAX_EVENT_SIZE));
        bool shouldExitAfterRetry = bytesToRead < MAX_EVENT_SIZE;

        {
            DWORD bytesRead = 0, offset = 0;
            while (ReadFile(hFile, disk_buffer, bytesToRead - offset, &bytesRead, nullptr))
            {
                if (bytesRead == 0)
                {
                    break;
                }

                CopyMemory(memory_buffer + offset, disk_buffer, bytesRead);
                offset += bytesRead;
            }
        }

        remainingFileSize -= bytesToRead;

        {
            DWORD bytesAvailableToProcess = bytesToRead, bytesProcessed = 0, offsetIntoBuffer = 0;
            while (ReadAndRelogEvent(relogger, memory_buffer + offsetIntoBuffer, bytesAvailableToProcess, bytesProcessed))
            {
                bytesAvailableToProcess -= bytesProcessed;
                offsetIntoBuffer += bytesProcessed;
            }

            if (!shouldExitAfterRetry)
            {
                SetFilePointer(hFile, 0 - bytesAvailableToProcess, nullptr, FILE_CURRENT);
                remainingFileSize += bytesAvailableToProcess;
            }
        }
    }

    free(disk_buffer);
    free(memory_buffer);
}