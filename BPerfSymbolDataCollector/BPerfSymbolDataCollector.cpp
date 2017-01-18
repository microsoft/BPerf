#include <thread>
#include <Windows.h>
#include <Evntprov.h>
#include <Evntcons.h>
#include <Evntrace.h>
#include <strsafe.h>

#define USAGE L"Usage: BPerfSymbolDataCollector LogsDirectory [ProcesNameFilters]\n"
#define EXAMPLE L"Example: BPerfSymbolDataCollector D:\\Data\\BPerfSymbols w3wp.exe\n"
#define EXAMPLE2 L"Example: BPerfSymbolDataCollector D:\\Data\\BPerfSymbols w3wp.exe;sql.exe\n"

#define CTRLCHANDLDED L"Ctrl+C handled, stopping session.\n"
#define LOGSESSION_NAME L"BPerfSymbols"
#define SYMBOLS_FILE_NAME L"Symbols.bin"
#define MAX_EVENT_SIZE 65536
#define UCHAR_SIZE 1
#define USHORT_SIZE 2
#define ULONG_SIZE 4
#define LARGE_INTEGER_SIZE 8
#define ULONGLONGSIZE 8
#define GUID_SIZE 16
#define CLRJITANDLOADERKEYWORD 0x00000018

constexpr GUID ClrRundownProviderGuid = {0xA669021C, 0xC450, 0x4609, {0xA0, 0x35, 0x5A, 0xF5, 0x9A, 0xF4, 0xDF, 0x18}}; // {A669021C-C450-4609-A035-5AF59AF4DF18}
constexpr GUID ClrProviderGuid = {0xE13C0D23, 0xCCBC, 0x4E12, {0x93, 0x1B, 0xD9, 0xCC, 0x2E, 0xEE, 0x27, 0xE4}};        // {E13C0D23-CCBC-4E12-931B-D9CC2EEE27E4}
constexpr GUID CoreClrProviderGuid = {0x319DC449, 0xADA5, 0x50F7, {0x42, 0x8E, 0x95, 0x7D, 0xB6, 0x79, 0x16, 0x68}};    // {319DC449-ADA5-50F7-428E-957DB6791668}

constexpr USHORT DCStopComplete = 146;

/* Begin Global Data */

TRACEHANDLE sessionHandle;
HANDLE outputFile;
BYTE writeBuffer[MAX_EVENT_SIZE];
bool ctrlCTriggered = false;

/* End Global Data */

BOOL WINAPI ConsoleControlHandler(DWORD ctrlType)
{
    switch (ctrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        wprintf(CTRLCHANDLDED);
        ctrlCTriggered = true;
        EVENT_TRACE_PROPERTIES properties;
        ZeroMemory(&properties, sizeof(EVENT_TRACE_PROPERTIES));
        properties.Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES);
        ControlTrace(0, LOGSESSION_NAME, &properties, EVENT_TRACE_CONTROL_STOP);
    default:
        break;
    }

    return FALSE;
}

ULONG WINAPI BufferCallback(_In_ PEVENT_TRACE_LOGFILEW callBack)
{
    UNREFERENCED_PARAMETER(callBack);
    return 1;
}

VOID WINAPI EventCallback(_In_ PEVENT_RECORD eventRecord)
{
    CopyMemory(writeBuffer, &eventRecord->EventHeader.TimeStamp, LARGE_INTEGER_SIZE);
    CopyMemory(writeBuffer + LARGE_INTEGER_SIZE, &eventRecord->EventHeader.ProcessId, ULONG_SIZE);
    CopyMemory(writeBuffer + LARGE_INTEGER_SIZE + ULONG_SIZE, &eventRecord->EventHeader.ThreadId, ULONG_SIZE);
    CopyMemory(writeBuffer + LARGE_INTEGER_SIZE + ULONG_SIZE + ULONG_SIZE, &eventRecord->EventHeader.ProviderId, GUID_SIZE);
    CopyMemory(writeBuffer + LARGE_INTEGER_SIZE + ULONG_SIZE + ULONG_SIZE + GUID_SIZE, &eventRecord->EventHeader.EventDescriptor.Id, USHORT_SIZE);
    CopyMemory(writeBuffer + LARGE_INTEGER_SIZE + ULONG_SIZE + ULONG_SIZE + GUID_SIZE + USHORT_SIZE, &eventRecord->EventHeader.EventDescriptor.Version, UCHAR_SIZE);
    CopyMemory(writeBuffer + LARGE_INTEGER_SIZE + ULONG_SIZE + ULONG_SIZE + GUID_SIZE + USHORT_SIZE + UCHAR_SIZE, &eventRecord->EventHeader.EventDescriptor.Level, UCHAR_SIZE);
    CopyMemory(writeBuffer + LARGE_INTEGER_SIZE + ULONG_SIZE + ULONG_SIZE + GUID_SIZE + USHORT_SIZE + UCHAR_SIZE + UCHAR_SIZE, &eventRecord->EventHeader.EventDescriptor.Channel, UCHAR_SIZE);
    CopyMemory(writeBuffer + LARGE_INTEGER_SIZE + ULONG_SIZE + ULONG_SIZE + GUID_SIZE + USHORT_SIZE + UCHAR_SIZE + UCHAR_SIZE + UCHAR_SIZE, &eventRecord->EventHeader.EventDescriptor.Opcode, UCHAR_SIZE);
    CopyMemory(writeBuffer + LARGE_INTEGER_SIZE + ULONG_SIZE + ULONG_SIZE + GUID_SIZE + USHORT_SIZE + UCHAR_SIZE + UCHAR_SIZE + UCHAR_SIZE + UCHAR_SIZE, &eventRecord->EventHeader.EventDescriptor.Task, USHORT_SIZE);
    CopyMemory(writeBuffer + LARGE_INTEGER_SIZE + ULONG_SIZE + ULONG_SIZE + GUID_SIZE + USHORT_SIZE + UCHAR_SIZE + UCHAR_SIZE + UCHAR_SIZE + UCHAR_SIZE + USHORT_SIZE, &eventRecord->EventHeader.EventDescriptor.Keyword, ULONGLONGSIZE);
    CopyMemory(writeBuffer + LARGE_INTEGER_SIZE + ULONG_SIZE + ULONG_SIZE + GUID_SIZE + USHORT_SIZE + UCHAR_SIZE + UCHAR_SIZE + UCHAR_SIZE + UCHAR_SIZE + USHORT_SIZE + ULONGLONGSIZE, &eventRecord->UserDataLength, USHORT_SIZE);
    CopyMemory(writeBuffer + LARGE_INTEGER_SIZE + ULONG_SIZE + ULONG_SIZE + GUID_SIZE + USHORT_SIZE + UCHAR_SIZE + UCHAR_SIZE + UCHAR_SIZE + UCHAR_SIZE + USHORT_SIZE + ULONGLONGSIZE + USHORT_SIZE, eventRecord->UserData, eventRecord->UserDataLength);

    DWORD numbBytesToWrite = LARGE_INTEGER_SIZE + ULONG_SIZE + ULONG_SIZE + GUID_SIZE + USHORT_SIZE + UCHAR_SIZE + UCHAR_SIZE + UCHAR_SIZE + UCHAR_SIZE + USHORT_SIZE + ULONGLONGSIZE + USHORT_SIZE + eventRecord->UserDataLength, offset = 0;
    DWORD numberOfBytesWritten = 0;

    while (WriteFile(outputFile, writeBuffer + offset, numbBytesToWrite, &numberOfBytesWritten, nullptr))
    {
        if (numberOfBytesWritten == 0)
        {
            break;
        }

        offset += numberOfBytesWritten;
        numbBytesToWrite -= numberOfBytesWritten;
    }

    if (eventRecord->EventHeader.ProviderId == ClrRundownProviderGuid && eventRecord->EventHeader.EventDescriptor.Id == DCStopComplete)
    {
        EnableTraceEx2(sessionHandle, &ClrRundownProviderGuid, EVENT_CONTROL_CODE_DISABLE_PROVIDER, TRACE_LEVEL_VERBOSE, 0, 0, 0, nullptr);
    }
}

HRESULT StartSymbolicDataSession(_In_ LPWSTR sessionName, _In_ LPCWSTR outputFileName, _In_opt_ LPCWSTR exeNamesFilter)
{
    auto sessionNameSizeInBytes = (wcslen(sessionName) + 1) * sizeof(WCHAR);
    auto bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sessionNameSizeInBytes + sizeof(WCHAR); // for '\0'
    auto sessionProperties = static_cast<PEVENT_TRACE_PROPERTIES>(malloc(bufferSize));

    if (sessionProperties == nullptr)
    {
        return E_FAIL;
    }

    ZeroMemory(sessionProperties, bufferSize);

    sessionProperties->Wnode.BufferSize = static_cast<ULONG>(bufferSize);
    sessionProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    sessionProperties->Wnode.ClientContext = 1;

    sessionProperties->MinimumBuffers = max(64, std::thread::hardware_concurrency() * 2);
    sessionProperties->MaximumBuffers = sessionProperties->MinimumBuffers * 2;

    sessionProperties->BufferSize = 1024; // 1MB
    sessionProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_INDEPENDENT_SESSION_MODE;
    sessionProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    sessionProperties->LogFileNameOffset = 0; // real time session has no file name
    sessionProperties->FlushTimer = 1;        // 1 second

    StringCbCopy(reinterpret_cast<LPWSTR>(reinterpret_cast<char *>(sessionProperties) + sessionProperties->LoggerNameOffset), sessionNameSizeInBytes, sessionName);

    HRESULT hr;
    auto retryCount = 1;

    while (retryCount < 10)
    {
        hr = StartTrace(&sessionHandle, sessionName, sessionProperties);
        if (hr != ERROR_SUCCESS)
        {
            if (hr == ERROR_ALREADY_EXISTS || hr == ERROR_INVALID_PARAMETER)
            {
                hr = ControlTrace(sessionHandle, sessionName, sessionProperties, EVENT_TRACE_CONTROL_STOP);
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

    outputFile = CreateFile(outputFileName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (outputFile == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    EVENT_TRACE_LOGFILE logFile;
    ZeroMemory(&logFile, sizeof(EVENT_TRACE_LOGFILE));
    logFile.LogFileName = nullptr;
    logFile.LoggerName = sessionName;
    logFile.LogFileMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
    logFile.BufferCallback = BufferCallback;
    logFile.EventRecordCallback = EventCallback;

    TRACEHANDLE traceHandle = OpenTrace(&logFile);

    if (traceHandle == reinterpret_cast<TRACEHANDLE>(INVALID_HANDLE_VALUE))
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    ENABLE_TRACE_PARAMETERS enableParameters;
    ZeroMemory(&enableParameters, sizeof(ENABLE_TRACE_PARAMETERS));
    enableParameters.FilterDescCount = 0;
    enableParameters.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;

    EVENT_FILTER_DESCRIPTOR descriptor[1];

    if (exeNamesFilter != nullptr)
    {
        descriptor[0].Ptr = ULONGLONG(exeNamesFilter);
        descriptor[0].Size = static_cast<ULONG>((wcslen(exeNamesFilter) + 1) * sizeof(WCHAR));
        descriptor[0].Type = EVENT_FILTER_TYPE_EXECUTABLE_NAME;

        enableParameters.EnableFilterDesc = descriptor;
        enableParameters.FilterDescCount = 1;
    }

    hr = EnableTraceEx2(sessionHandle, &ClrProviderGuid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_VERBOSE, CLRJITANDLOADERKEYWORD, 0, 0, &enableParameters);
    if (hr != ERROR_SUCCESS)
    {
        return hr;
    }

    hr = EnableTraceEx2(sessionHandle, &CoreClrProviderGuid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_VERBOSE, CLRJITANDLOADERKEYWORD, 0, 0, &enableParameters);
    if (hr != ERROR_SUCCESS)
    {
        return hr;
    }

    hr = EnableTraceEx2(sessionHandle, &ClrRundownProviderGuid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_VERBOSE, 0x00020118, 0, 0, &enableParameters);
    if (hr != ERROR_SUCCESS)
    {
        return hr;
    }

    ProcessTrace(&traceHandle, 1, nullptr, nullptr);
    CloseHandle(outputFile);

    return ERROR_SUCCESS;
}

int wmain(_In_ int argc, _In_ PWSTR *argv)
{
    if (argc < 2)
    {
        wprintf(USAGE);
        wprintf(EXAMPLE);
        wprintf(EXAMPLE2);
        return ERROR_INVALID_PARAMETER;
    }

    auto flushThread = std::thread([] {
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            FlushFileBuffers(outputFile);
        }
    });

    SetConsoleCtrlHandler(ConsoleControlHandler, true);
    SetCurrentDirectory(argv[1]);

    wprintf(L"\n  BPerfSymbolDataCollector is running ...\n");
    wprintf(L"  ETW Session Name: ");
    wprintf(LOGSESSION_NAME);
    wprintf(L"\n  Symbol File: ");
    wprintf(argv[1]);
    wprintf(L"\\");
    wprintf(SYMBOLS_FILE_NAME);
    wprintf(L"\n\n  ** Provide this file location when starting up BPerfCPUSampleCollector **\n");

    StartSymbolicDataSession(LOGSESSION_NAME, SYMBOLS_FILE_NAME, argc == 3 ? argv[2] : nullptr);

    flushThread.detach(); // let process tear down take care of this
}