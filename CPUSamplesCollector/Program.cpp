#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

#include <relogger.h>
#include <windows.h>
#include <strsafe.h>
#include "TraceEventCallback.h"
#include <evntprov.h>

#define LOGFILE_PATH L""
#define LOGSESSION_NAME L"BPerfProfiles"
#define LOGFILE_NAME L"Relogger.etl"

typedef struct _STACK_TRACING_EVENT_ID
{
    GUID EventGuid;
    UCHAR Type;
    UCHAR Reserved[7];
} STACK_TRACING_EVENT_ID, *PSTACK_TRACING_EVENT_ID;

enum EVENT_TRACE_INFORMATION_CLASS
{
    EventTraceTimeProfileInformation = 3,
    EventTraceStackCachingInformation = 16
};

enum SYSTEM_INFORMATION_CLASS
{
    SystemPerformanceTraceInformation = 31
};

typedef struct _EVENT_TRACE_TIME_PROFILE_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    ULONG ProfileInterval;
} EVENT_TRACE_TIME_PROFILE_INFORMATION;

bool ctrlCTriggered = false;
typedef int(__stdcall *NtSetSystemInformation)(int SystemInformationClass, void *SystemInformation, int SystemInformationLength);
auto ntdll = LoadLibrary(L"ntdll.dll");
auto addr = NtSetSystemInformation(GetProcAddress(ntdll, "NtSetSystemInformation"));

ULONG EnableTraceEx2Shim(_In_ TRACEHANDLE traceHandle, _In_ LPCGUID guid, _In_ BYTE level, _In_ ULONG matchAnyKeyword, _In_ ULONG matchAllKeyword, _In_ ULONG timeout, _In_opt_ LPCWSTR exeNamesFilter, _In_opt_ const USHORT *stackEventIdsFilter, _In_ USHORT stackEventIdsCount)
{
    int index = 0;

    EVENT_FILTER_DESCRIPTOR descriptor[2];
    ENABLE_TRACE_PARAMETERS enableParameters;
    ZeroMemory(&enableParameters, sizeof(ENABLE_TRACE_PARAMETERS));
    enableParameters.FilterDescCount = 0;
    enableParameters.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
    enableParameters.EnableFilterDesc = descriptor;

    auto eventIdsBufferSize = ((stackEventIdsCount - 1) * 2) + sizeof(EVENT_FILTER_EVENT_ID);
    PEVENT_FILTER_EVENT_ID eventIdsBuffer;

    eventIdsBuffer = (PEVENT_FILTER_EVENT_ID)malloc(eventIdsBufferSize);
    ZeroMemory(eventIdsBuffer, eventIdsBufferSize);

    eventIdsBuffer->FilterIn = TRUE;
    eventIdsBuffer->Reserved = 0;
    eventIdsBuffer->Count = stackEventIdsCount;

    auto eventIdsPtr = &eventIdsBuffer->Events[0];
    for (int i = 0; i < stackEventIdsCount; ++i)
    {
        *eventIdsPtr++ = stackEventIdsFilter[i];
    }

    if (exeNamesFilter != nullptr)
    {
        descriptor[index].Ptr = (ULONGLONG)exeNamesFilter;
        descriptor[index].Size = static_cast<ULONG>((wcslen(exeNamesFilter) + 1) * sizeof(WCHAR));
        descriptor[index].Type = EVENT_FILTER_TYPE_EXECUTABLE_NAME;
        index++;
    }

    if (stackEventIdsCount > 0)
    {
        descriptor[index].Ptr = (ULONGLONG)eventIdsBuffer;
        descriptor[index].Size = static_cast<ULONG>(eventIdsBufferSize);
        descriptor[index].Type = EVENT_FILTER_TYPE_STACKWALK;
        index++;

        enableParameters.EnableProperty = EVENT_ENABLE_PROPERTY_STACK_TRACE;
    }

    enableParameters.FilterDescCount = index; // set the right size

    auto retVal = EnableTraceEx2(traceHandle, guid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, level, matchAnyKeyword, matchAllKeyword, timeout, &enableParameters);

    free(eventIdsBuffer);

    return retVal;
}

void FillEventProperties(PEVENT_TRACE_PROPERTIES sessionProperties, ULONG bufferSize)
{
    ZeroMemory(sessionProperties, bufferSize);

    sessionProperties->Wnode.BufferSize = bufferSize;
    sessionProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    sessionProperties->Wnode.ClientContext = 1;

    sessionProperties->MinimumBuffers = max(64, std::thread::hardware_concurrency() * 2);
    sessionProperties->MaximumBuffers = sessionProperties->MinimumBuffers * 2;

    sessionProperties->BufferSize = 1024;
    sessionProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_SYSTEM_LOGGER_MODE | EVENT_TRACE_INDEPENDENT_SESSION_MODE;
    sessionProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    sessionProperties->LogFileNameOffset = 0;
    sessionProperties->FlushTimer = 1;

    StringCbCopy(reinterpret_cast<LPWSTR>(reinterpret_cast<char *>(sessionProperties) + sessionProperties->LoggerNameOffset), (wcslen(LOGSESSION_NAME) + 1) * 2, LOGSESSION_NAME);
}

HRESULT StartSession(PTRACEHANDLE sessionHandle, PEVENT_TRACE_PROPERTIES sessionProperties)
{
    HRESULT hr = ERROR_SUCCESS;
    int retryCount = 1;
    while (retryCount < 10)
    {
        hr = StartTrace(static_cast<PTRACEHANDLE>(sessionHandle), LOGSESSION_NAME, sessionProperties);
        if (hr != ERROR_SUCCESS)
        {
            if (hr == ERROR_ALREADY_EXISTS)
            {
                hr = ControlTrace(*sessionHandle, LOGSESSION_NAME, sessionProperties, EVENT_TRACE_CONTROL_STOP);
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

HRESULT RelogTraceFile(TRACEHANDLE sessionHandle, PWSTR outputFileName, int rollOverTimeMSec, PWSTR jittedCodeSymbolicDataFile)
{
    HRESULT hr;
    ITraceRelogger *relogger = nullptr;
    TraceEventCallback *callback = nullptr;

    hr = CoCreateInstance(CLSID_TraceRelogger, nullptr, CLSCTX_INPROC_SERVER, __uuidof(ITraceRelogger), reinterpret_cast<LPVOID *>(&relogger));

    if (FAILED(hr))
    {
        wprintf(L"Failed to instantiate ITraceRelogger object: 0x%08x.\n", hr);
        goto cleanup;
    }

    relogger->SetOutputFilename(outputFileName);
    if (FAILED(hr))
    {
        wprintf(L"Failed to set output log file: 0x%08x.\n", hr);
        goto cleanup;
    }

    TRACEHANDLE traceHandle;

    hr = relogger->AddRealtimeTraceStream(LOGSESSION_NAME, nullptr, &traceHandle);
    if (FAILED(hr))
    {
        wprintf(L"Failed to set log file input stream: 0x%08x.\n", hr);
        goto cleanup;
    }

    callback = new TraceEventCallback(sessionHandle, relogger, rollOverTimeMSec, jittedCodeSymbolicDataFile);

    if (callback == nullptr)
    {
        wprintf(L"Failed to allocate callback: 0x%08x.\n", hr);
        goto cleanup;
    }

    hr = relogger->RegisterCallback(callback);
    if (FAILED(hr))
    {
        wprintf(L"Failed to register callback: 0x%08x.\n", hr);
        goto cleanup;
    }

    DoStartRundown(sessionHandle);

    hr = relogger->ProcessTrace();
    if (FAILED(hr))
    {
        wprintf(L"Failed to process trace: 0x%08x.\n", hr);
    }

cleanup:

    if (relogger != nullptr)
    {
        relogger->Release();
    }

    if (callback != nullptr)
    {
        callback->Release();
    }

    return hr;
}

wstring FormatProfileSting()
{
    auto in_time_t = chrono::system_clock::to_time_t(chrono::system_clock::now());
    tm nt;
    localtime_s(&nt, &in_time_t);

    wstringstream ss;
    ss << put_time(&nt, L"%Y-%m-%d-%H-%M-%S");
    return ss.str();
}

BOOL WINAPI ConsoleControlHandler(DWORD ctrlType)
{
    switch (ctrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        wprintf(L"Ctrl+C handled, stopping session.\n");
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

int wmain(const int argc, PWSTR *argv)
{
    if (argc < 4)
    {
        printf("Usage: BPerfCPUSamplesCollector CpuTimerInMilliSeconds FileRolloverPeriodSec ProfileLogsDirectory [JittedCodeSymbolicDataFile] [ProcessNameFilters]\n");
        printf("Example 1: BPerfCPUSamplesCollector 10 900 D:\\Data\\BPerfLogs D:\\Data\\BPerfSymbols\\Symbols.bin\n");
        printf("Example 2: BPerfCPUSamplesCollector 10 900 D:\\Data\\BPerfLogs D:\\Data\\BPerfSymbols\\Symbols.bin w3wp.exe\n");
        printf("Example 3: BPerfCPUSamplesCollector 10 900 D:\\Data\\BPerfLogs D:\\Data\\BPerfSymbols\\Symbols.bin w3wp.exe;sql.exe\n");
        return ERROR_INVALID_PARAMETER;
    }

    SetConsoleCtrlHandler(ConsoleControlHandler, true);

    HRESULT hr;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        wprintf(L"Failed to initialize COM: 0x%08x\n", hr);
        return HRESULT_CODE(hr);
    }

    /* set working dir */
    SetCurrentDirectory(argv[3]);

    auto cpuSampleMSec = _wtoi(argv[1]);
    auto rolloverTimeMSec = _wtoi(argv[2]) * 1000;

    if (!AcquireProfilePrivileges())
    {
        wprintf(L"Failed to acquire profiling privileges. Are you running as admin?\n");
        return E_FAIL;
    }

    int bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(LOGFILE_PATH) + sizeof(LOGSESSION_NAME);
    auto sessionProperties = static_cast<PEVENT_TRACE_PROPERTIES>(malloc(bufferSize));

    if (sessionProperties == nullptr)
    {
        wprintf(L"Unable to allocate %d bytes for properties structure.\n", bufferSize);
        return E_FAIL;
    }

    FillEventProperties(sessionProperties, bufferSize);

    TRACEHANDLE sessionHandle;

    hr = StartSession(&sessionHandle, sessionProperties);
    if (FAILED(HRESULT(hr)))
    {
        wprintf(L"StartTrace() failed with %lu\n", hr);
        goto cleanup;
    }

    ULONG data = EVENT_TRACE_FLAG_PROCESS | EVENT_TRACE_FLAG_IMAGE_LOAD | EVENT_TRACE_FLAG_THREAD | EVENT_TRACE_FLAG_PROFILE;
    hr = TraceSetInformation(sessionHandle, TraceSystemTraceEnableFlagsInfo, &data, sizeof(ULONG));

    if (hr != ERROR_SUCCESS)
    {
        wprintf(L"Failed to set trace information: 0x%08x.\n", hr);
        goto cleanup;
    }

    auto exeNamesFilter = argc == 6 ? argv[5] : nullptr;

    EnableTraceEx2Shim(sessionHandle, &AspNetEventSource, TRACE_LEVEL_INFORMATION, 0, 0, 0, exeNamesFilter, nullptr, 0);
    EnableTraceEx2Shim(sessionHandle, &TplEventSource, TRACE_LEVEL_INFORMATION, 0x42, 0, 0, exeNamesFilter, nullptr, 0);
    EnableTraceEx2Shim(sessionHandle, &BPerfStampingGuid, TRACE_LEVEL_INFORMATION, 0, 0, 0, exeNamesFilter, nullptr, 0);
    EnableTraceEx2Shim(sessionHandle, &MonitoredScopeEventSource, TRACE_LEVEL_INFORMATION, 0, 0, 0, exeNamesFilter, nullptr, 0);

    USHORT stacksForTcpIp[1] = {1013}; // Tcp Create Endpoint
    EnableTraceEx2Shim(sessionHandle, &TcpIpProviderGuid, TRACE_LEVEL_INFORMATION, 0x1, 0, 0, exeNamesFilter, stacksForTcpIp, 1);

    USHORT stacksForClr[3] = {80, 250, 252}; // Exception Start, Catch, Finally
    EnableTraceEx2Shim(sessionHandle, &ClrGuid, TRACE_LEVEL_INFORMATION, 0x18011, 0, 0, exeNamesFilter, stacksForClr, 3);
    EnableTraceEx2Shim(sessionHandle, &CoreClrGuid, TRACE_LEVEL_INFORMATION, 0x18011, 0, 0, exeNamesFilter, stacksForClr, 3);

    if (hr != ERROR_SUCCESS)
    {
        wprintf(L"Failed to enable bperf stamping guid: 0x%08x.\n", hr);
        goto cleanup;
    }

    STACK_TRACING_EVENT_ID stacks;
    stacks.EventGuid = {0xce1dbfb4, 0x137e, 0x4da6, {0x87, 0xb0, 0x3f, 0x59, 0xaa, 0x10, 0x2c, 0xbc}};
    stacks.Type = 0x2e;
    hr = TraceSetInformation(sessionHandle, TraceStackTracingInfo, &stacks, sizeof(STACK_TRACING_EVENT_ID));

    if (hr != ERROR_SUCCESS)
    {
        wprintf(L"Failed to set stack trace information: 0x%08x.\n", hr);
        goto cleanup;
    }

    EVENT_TRACE_TIME_PROFILE_INFORMATION timeInfo;
    timeInfo.EventTraceInformationClass = EventTraceTimeProfileInformation;
    timeInfo.ProfileInterval = static_cast<int>(cpuSampleMSec * 10000.0 + .5);
    hr = addr(SystemPerformanceTraceInformation, &timeInfo, sizeof(EVENT_TRACE_TIME_PROFILE_INFORMATION)); // set interval to 10ms
    if (hr != ERROR_SUCCESS)
    {
        wprintf(L"Failed to set profile timer info\r\n");
    }

    wprintf(L"\n  BPerfCPUSamplesCollector is running ...\n");

    wprintf(L"  Profiling Timer Speed : %dms\n", cpuSampleMSec);
    wprintf(L"  File Rotation Period  : %ds\n", rolloverTimeMSec / 1000);
    wprintf(L"  ETW Session Name: %s\n", LOGSESSION_NAME);
    wprintf(L"  Profiles Directory Location: %s\n", argv[3]);

    if (argc >= 5)
    {
        wprintf(L"  Using Symbol Data File: %s\n", argv[4]);
    }

    if (argc == 6)
    {
        wprintf(L"  Process Name Filters: %s\n", argv[5]);
    }

    wprintf(L"\n  *** NOTE *** -> Profile file names start with ''BPerfProfiles'' followed by timestamp\n\n");

    while (true)
    {
        if (ctrlCTriggered)
        {
            wprintf(L"BPerf exiting in response to Ctrl+C.");
            break;
        }

        auto begin = FormatProfileSting();

        hr = RelogTraceFile(sessionHandle, LOGFILE_NAME, rolloverTimeMSec, argc >= 5 ? argv[4] : nullptr);

        if (FAILED(hr))
        {
            wprintf(L"Exiting because Relogging failed.");
            break;
        }

        auto end = FormatProfileSting();

        auto destWString = (L"BPerfProfiles." + begin + L"_" + end + L".etl");
        auto dest = destWString.c_str();
        wprintf(L"  Writing '%s' ...", dest);
        MoveFileW(LOGFILE_NAME, dest);
        wprintf(L" Done\n");
    }

    timeInfo.ProfileInterval = static_cast<int>(10000.0 + .5); // set back to 1ms
    addr(SystemPerformanceTraceInformation, &timeInfo, sizeof(EVENT_TRACE_TIME_PROFILE_INFORMATION));

    EVENT_TRACE_PROPERTIES properties;
    ZeroMemory(&properties, sizeof(EVENT_TRACE_PROPERTIES));
    properties.Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES);
    ControlTrace(0, LOGSESSION_NAME, &properties, EVENT_TRACE_CONTROL_STOP);

cleanup:
    if (sessionProperties != nullptr)
    {
        free(sessionProperties);
    }

    CoUninitialize();
    return HRESULT_CODE(hr);
}