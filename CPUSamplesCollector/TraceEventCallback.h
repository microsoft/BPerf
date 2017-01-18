#pragma once

#include <thread>
#include <objbase.h>
#include <relogger.h>
#include <atomic>
#include <agents.h>
#include <ppltasks.h>
#include <evntrace.h>
#include <strsafe.h>
#include <unordered_map>
#include <evntcons.h>

constexpr GUID PerfInfoGuid = {0xce1dbfb4, 0x137e, 0x4da6, {0x87, 0xb0, 0x3f, 0x59, 0xaa, 0x10, 0x2c, 0xbc}};
constexpr GUID BPerfStampingGuid = {0xD46E5565, 0xD624, 0x4C4D, {0x89, 0xCC, 0x7A, 0x82, 0x88, 0x7D, 0x36, 0x27}};
constexpr GUID ThreadGuid = {0x3d6fa8d1, 0xfe05, 0x11d0, {0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c}};
constexpr GUID ProcessGuid = {0x3d6fa8d0, 0xfe05, 0x11d0, {0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c}};
constexpr GUID RundownGuid = {0x68fdd900, 0x4a3e, 0x11d1, {0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3}};
constexpr GUID ImageLoadGuid = {0x2cb15d1d, 0x5fc1, 0x11d2, {0xab, 0xe1, 0x00, 0xa0, 0xc9, 0x11, 0xf5, 0x18}};
constexpr GUID ClrGuid = {0xE13C0D23, 0xCCBC, 0x4E12, {0x93, 0x1B, 0xD9, 0xCC, 0x2E, 0xEE, 0x27, 0xE4}};
constexpr GUID CoreClrGuid = {0x319DC449, 0xADA5, 0x50F7, {0x42, 0x8E, 0x95, 0x7D, 0xB6, 0x79, 0x16, 0x68}};
constexpr GUID TplEventSource = {0x2e5dba47, 0xa3d2, 0x4d16, {0x8e, 0xe0, 0x66, 0x71, 0xff, 0xdc, 0xd7, 0xb5}};
constexpr GUID AspNetEventSource = {0xee799f41, 0xcfa5, 0x550b, {0xbf, 0x2c, 0x34, 0x47, 0x47, 0xc1, 0xc6, 0x68}};
constexpr GUID TcpIpProviderGuid = {0x2f07e2ee, 0x15db, 0x40f1, {0x90, 0xef, 0x9d, 0x7b, 0xa2, 0x82, 0x18, 0x8a}};
constexpr GUID KernelProcessGuid = {0x22fb2cd6, 0x0e7b, 0x422b, {0xa0, 0xc7, 0x2f, 0xad, 0x1f, 0xd0, 0xe7, 0x16}};
constexpr GUID MonitoredScopeEventSource = {0x2e185d98, 0x344d, 0x5ef7, {0x06, 0x30, 0xaf, 0xe3, 0x22, 0xf0, 0x1b, 0x98}};

constexpr UCHAR CSwitchOpCode = 36;
constexpr UCHAR StackWalkOpCode = 32;
constexpr UCHAR DCStopOpCode = 4;
constexpr UCHAR ThreadPoolWorkerThreadStartOpCode = 50;
constexpr UCHAR ThreadPoolWorkerThreadStopOpCode = 51;

constexpr USHORT TaskScheduled = 7;
constexpr USHORT TaskStart = 8;
constexpr USHORT TaskStop = 9;

constexpr USHORT RundownComplete = 8;

__declspec(noinline) HRESULT InjectKernelTraceControlEvents(PEVENT_RECORD eventRecord, ITraceRelogger *relogger);
void LogJittedCodeSymbolicData(HANDLE hFile, ITraceRelogger *relogger);

using namespace concurrency;
using namespace std;

inline void DoRundown(TRACEHANDLE tracehandle, ULONG type)
{
    EVENT_FILTER_DESCRIPTOR descriptor[1];
    descriptor[0].Type = EVENT_FILTER_TYPE_TRACEHANDLE;
    descriptor[0].Ptr = reinterpret_cast<ULONGLONG>(&tracehandle);
    descriptor[0].Size = sizeof(TRACEHANDLE);

    ENABLE_TRACE_PARAMETERS enableParameters;
    ZeroMemory(&enableParameters, sizeof(ENABLE_TRACE_PARAMETERS));
    enableParameters.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
    enableParameters.EnableFilterDesc = descriptor;
    enableParameters.FilterDescCount = 1;

    EnableTraceEx2(tracehandle, &AspNetEventSource, EVENT_CONTROL_CODE_CAPTURE_STATE, 0, 0, 0, 0, &enableParameters);
    EnableTraceEx2(tracehandle, &TplEventSource, EVENT_CONTROL_CODE_CAPTURE_STATE, 0, 0, 0, 0, &enableParameters);
    EnableTraceEx2(tracehandle, &MonitoredScopeEventSource, EVENT_CONTROL_CODE_CAPTURE_STATE, 0, 0, 0, 0, &enableParameters);

    GUID systemGuid = {0x9e814aad, 0x3204, 0x11d2, {0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39}};
    EnableTraceEx2(tracehandle, &systemGuid, EVENT_CONTROL_CODE_CAPTURE_STATE, 0, type, 0, INFINITE, &enableParameters);
}

inline void DoStartRundown(TRACEHANDLE tracehandle)
{
    DoRundown(tracehandle, 1);
}

inline void DoStopRundown(TRACEHANDLE tracehandle)
{
    DoRundown(tracehandle, 2);
}

// Creates a task that completes after the specified delay.
inline task<void> complete_after(unsigned int timeout)
{
    // A task completion event that is set when a timer fires.
    task_completion_event<void> tce;

    // Create a non-repeating timer.
    auto fire_once = new timer<int>(timeout, 0, nullptr, false);
    // Create a call object that sets the completion event after the timer fires.
    auto callback = new call<int>([tce](int) {
        tce.set();
    });

    // Connect the timer to the callback and start the timer.
    fire_once->link_target(callback);
    fire_once->start();

    // Create a task that completes after the completion event is set.
    task<void> event_set(tce);

    // Create a continuation task that cleans up resources and
    // and return that continuation task.
    return event_set.then([callback, fire_once]() {
        delete callback;
        delete fire_once;
    });
}

class TraceEventCallback : public ITraceEventCallback
{
  public:
    HRESULT STDMETHODCALLTYPE OnEvent(ITraceEvent *traceEvent, ITraceRelogger *relogger) override;
    HRESULT STDMETHODCALLTYPE OnBeginProcessTrace(ITraceEvent *header, ITraceRelogger *relogger) override;
    HRESULT STDMETHODCALLTYPE OnFinalizeProcessTrace(ITraceRelogger *relogger) override;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppObj) override
    {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ITraceEventCallback))
        {
            *ppObj = this;
        }
        else
        {
            *ppObj = nullptr;
            return E_NOINTERFACE;
        }

        this->AddRef();
        return ERROR_SUCCESS;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return atomic_fetch_add(&this->refCount, 1) + 1;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        int count = atomic_fetch_sub(&this->refCount, 1) - 1;
        if (count == 0)
        {
            delete this;
        }

        return count;
    }

    TraceEventCallback(TRACEHANDLE tracehandle, ITraceRelogger *relogger, int rollOverTimeMSec, PWSTR jittedCodeSymbolicDataFile)
    {
        this->isCancelled = false;
        this->refCount = 0;
        this->jittedCodeSymbolicDataFile = jittedCodeSymbolicDataFile;
        this->reloggerCancellationTask = complete_after(rollOverTimeMSec).then([this, relogger, tracehandle] {
            this->isCancelled = true;
            DoStopRundown(tracehandle);
        });
    }

    virtual ~TraceEventCallback()
    {
    }

  private:
    bool isCancelled;
    atomic<int> refCount;
    task<void> reloggerCancellationTask;
    PWSTR jittedCodeSymbolicDataFile;
};