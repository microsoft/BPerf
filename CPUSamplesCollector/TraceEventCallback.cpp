#include "TraceEventCallback.h"
#include <unordered_set>

unordered_set<ULONG> threadingTrackingTable;
unordered_set<ULONG> taskTrackingTable;

__declspec(noinline) void HandleRundowCompletion(PEVENT_RECORD eventRecord, ITraceRelogger *relogger, bool isCancelled)
{
    if (eventRecord->EventHeader.EventDescriptor.Opcode == RundownComplete && isCancelled)
    {
        relogger->Cancel();
    }
}

__declspec(noinline) void HandleBPerfStampingEvents(PEVENT_RECORD eventRecord)
{
    switch (eventRecord->EventHeader.EventDescriptor.Id)
    {
    case 1:
        threadingTrackingTable.insert(eventRecord->EventHeader.ThreadId);
        break;
    case 2:
        threadingTrackingTable.erase(eventRecord->EventHeader.ThreadId);
        break;
    }
}

__declspec(noinline) void HandleTplEvents(PEVENT_RECORD eventRecord)
{
    switch (eventRecord->EventHeader.EventDescriptor.Id)
    {
    case TaskScheduled:
    {
        if (threadingTrackingTable.find(eventRecord->EventHeader.ThreadId) != threadingTrackingTable.end())
        {
            auto taskId = *reinterpret_cast<ULONG *>(static_cast<PBYTE>(eventRecord->UserData) + 8);
            taskTrackingTable.insert(taskId);
        }
    }
    break;
    case TaskStart:
    {
        auto taskId = *reinterpret_cast<ULONG *>(static_cast<PBYTE>(eventRecord->UserData) + 8);
        if (taskTrackingTable.find(taskId) != taskTrackingTable.end())
        {
            // if yes, then we should track this thread also
            threadingTrackingTable.insert(eventRecord->EventHeader.ThreadId);
        }
    }
    break;
    case TaskStop:
    {
        auto taskId = *reinterpret_cast<ULONG *>(static_cast<PBYTE>(eventRecord->UserData) + 8);
        taskTrackingTable.erase(taskId);
        threadingTrackingTable.erase(eventRecord->EventHeader.ThreadId);
    }
    break;
    }
}

__declspec(noinline) void ResetThreadState(PEVENT_RECORD eventRecord)
{
    if (eventRecord->EventHeader.ProviderId == ThreadGuid ||
        eventRecord->EventHeader.ProviderId == ClrGuid &&
            (eventRecord->EventHeader.EventDescriptor.Id == ThreadPoolWorkerThreadStartOpCode ||
             eventRecord->EventHeader.EventDescriptor.Id == ThreadPoolWorkerThreadStopOpCode))
    {
        threadingTrackingTable.erase(eventRecord->EventHeader.ThreadId);
    }
}

HRESULT STDMETHODCALLTYPE TraceEventCallback::OnEvent(ITraceEvent *traceEvent, ITraceRelogger *relogger)
{
    PEVENT_RECORD eventRecord;
    traceEvent->GetEventRecord(&eventRecord);

    if (eventRecord->EventHeader.ProviderId == ThreadGuid && eventRecord->EventHeader.EventDescriptor.Opcode == CSwitchOpCode)
    {
        auto oldThreadId = *reinterpret_cast<ULONG *>(static_cast<PBYTE>(eventRecord->UserData) + 4);
        if (threadingTrackingTable.find(oldThreadId) != threadingTrackingTable.end())
        {
            relogger->Inject(traceEvent);
        }

        return S_OK;
    }

    if (eventRecord->EventHeader.ProviderId == TplEventSource)
    {
        HandleTplEvents(eventRecord);
    }

    if (eventRecord->EventHeader.ProviderId == BPerfStampingGuid)
    {
        HandleBPerfStampingEvents(eventRecord);
    }

    if (eventRecord->EventHeader.ProviderId == ThreadGuid || eventRecord->EventHeader.ProviderId == ClrGuid)
    {
        ResetThreadState(eventRecord);
    }

    if (eventRecord->EventHeader.ProviderId == ImageLoadGuid)
    {
        InjectKernelTraceControlEvents(eventRecord, relogger);
    }

    if (eventRecord->EventHeader.ProviderId == RundownGuid)
    {
        HandleRundowCompletion(eventRecord, relogger, this->isCancelled);
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
    if (this->jittedCodeSymbolicDataFile != nullptr)
    {
        HANDLE hFile = CreateFileW(this->jittedCodeSymbolicDataFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            LogJittedCodeSymbolicData(hFile, relogger);
        }
    }

    return S_OK;
}