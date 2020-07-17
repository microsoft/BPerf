#pragma once

#include <objbase.h>
#include <relogger.h>
#include <evntrace.h>
#include <strsafe.h>
#include <evntcons.h>
#include <unordered_set>

struct EventKey
{
    GUID ProviderGuid;
    USHORT EventId;
    UCHAR Version;

    EventKey(const GUID &providerId, USHORT eventId, UCHAR version) : ProviderGuid(providerId), EventId(eventId), Version(version)
    {
    }

    bool operator==(const EventKey &other) const noexcept
    {
        return InlineIsEqualGUID(this->ProviderGuid, other.ProviderGuid) && this->EventId == other.EventId && this->Version == other.Version;
    }
};

struct EventKeyHasher
{
    size_t operator()(const EventKey &k) const noexcept
    {
        using std::size_t;
        using std::string;

        const uint64_t *p = reinterpret_cast<const uint64_t *>(&k.ProviderGuid);
        const std::hash<uint64_t> hash;

        return hash(p[0]) ^ hash(p[1]) ^ k.EventId ^ k.Version;
    }
};

struct ContextInfo
{
    ContextInfo(PBYTE buffer, PBYTE compressionBuffer, PBYTE workspace, HANDLE dataFile, HANDLE indexFile, HANDLE metadataFile, ITraceRelogger *relogger, BOOL enableMetadata, BOOL enableSymbolServerInfo, const std::unordered_map<std::wstring, std::wstring> * volumeMap)
    {
        this->Buffer = buffer;
        this->CompressionBuffer = compressionBuffer;
        this->Workspace = workspace;
        this->DataFile = dataFile;
        this->IndexFile = indexFile;
        this->MetadataFile = metadataFile;

        this->Offset = 0;
        this->LastEventIndexedTimeStamp = 0;
        this->PerfFreq = 0;
        this->Relogger = relogger;
        this->EnableMetadata = enableMetadata;
        this->EnableSymbolServerInfo = enableSymbolServerInfo;
        this->VolumeMap = volumeMap;
#ifdef DEBUG
        this->TotalEvents = 0;
#endif
    }

    std::unordered_set<EventKey, EventKeyHasher> EventMetadataWrittenMap;
    PBYTE Buffer;
    DWORD Offset;
    LONGLONG LastEventIndexedTimeStamp;
    LONGLONG PerfFreq;

    HANDLE DataFile;
    HANDLE IndexFile;
    HANDLE MetadataFile;

    PBYTE CompressionBuffer;
    PBYTE Workspace;

    ITraceRelogger *Relogger;
    BOOL EnableMetadata;
    BOOL EnableSymbolServerInfo;
    const std::unordered_map<std::wstring, std::wstring> * VolumeMap;

#ifdef DEBUG
    ULONG TotalEvents;
#endif
};

struct ContextInfo2
{
    ContextInfo2(FILETIME startTime, LARGE_INTEGER startQPC, const wchar_t * dataDirectory, const std::unordered_set<std::wstring> * clrProcessNamesSet, const std::unordered_map<std::wstring, std::wstring> * volumeMap)
    {
        this->StartTime = startTime;
        this->StartQPC = startQPC;
        this->DataDirectory = dataDirectory;
        this->CLRProcessNamesSet = clrProcessNamesSet;
        this->VolumeMap = volumeMap;
    }

    const std::unordered_set<std::wstring> * CLRProcessNamesSet;

    FILETIME StartTime;
    LARGE_INTEGER StartQPC;
    const wchar_t * DataDirectory;
    const std::unordered_map<std::wstring, std::wstring> * VolumeMap;

    std::unordered_map<ULONG, SafeFileHandle> ManagedMethodMapFile;
    std::unordered_map<ULONG, SafeFileHandle> ManagedILToNativeMapFile;
    std::unordered_map<ULONG, SafeFileHandle> ManagedModuleMapFile;
    std::unordered_map<ULONG, SafeFileHandle> NativeModuleMapFile;
    std::unordered_map<ULONG, SafeFileHandle> DataFile;
    std::unordered_map<ULONG, LONG> OffsetIntoDataFile;
};

__declspec(noinline) HRESULT InjectKernelTraceControlEvents(PEVENT_RECORD eventRecord, ITraceRelogger *relogger);
__declspec(noinline) HRESULT InjectTraceEventInfoEvent(PEVENT_RECORD eventRecord, ITraceRelogger *relogger);

class TraceEventCallback : public ITraceEventCallback
{
  public:
    HRESULT STDMETHODCALLTYPE OnEvent(ITraceEvent *traceEvent, ITraceRelogger *relogger) override;
    HRESULT STDMETHODCALLTYPE OnBeginProcessTrace(ITraceEvent *header, ITraceRelogger *relogger) override;
    HRESULT STDMETHODCALLTYPE OnFinalizeProcessTrace(ITraceRelogger *relogger) override;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppObj) override
    {
        UNREFERENCED_PARAMETER(riid);
        UNREFERENCED_PARAMETER(ppObj);
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return 0;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        return 0;
    }

    TraceEventCallback(BOOL enableMetadata, BOOL enableSymbolServerInfo, const std::unordered_map<std::wstring, std::wstring>* volumeMap)
    {
        this->enableMetadata = enableMetadata;
        this->enableSymbolServerInfo = enableSymbolServerInfo;
        this->volumeMap = volumeMap;
        this->qpc = { 0 };
    }

    virtual ~TraceEventCallback()
    {
    }

  private:
    BOOL enableMetadata;
    BOOL enableSymbolServerInfo;
    std::unordered_set<EventKey, EventKeyHasher> eventMetadataLogged;
    LARGE_INTEGER qpc;
    const std::unordered_map<std::wstring, std::wstring>* volumeMap;
};

class FakeTraceEvent : public ITraceEvent
{
    HRESULT STDMETHODCALLTYPE Clone(ITraceEvent **NewEvent) override
    {
        UNREFERENCED_PARAMETER(NewEvent);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetUserContext(void **UserContext) override
    {
        *UserContext = &this->eventRecord.UserContext;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetEventRecord(PEVENT_RECORD *EventRecord) override
    {
        *EventRecord = &this->eventRecord;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetPayload(BYTE *Payload, ULONG PayloadSize) override
    {
        this->eventRecord.UserData = Payload;
        this->eventRecord.UserDataLength = (USHORT)PayloadSize;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetEventDescriptor(PCEVENT_DESCRIPTOR EventDescriptor) override
    {
        this->eventRecord.EventHeader.EventDescriptor = *EventDescriptor;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetProcessId(ULONG ProcessId) override
    {
        this->eventRecord.EventHeader.ProcessId = ProcessId;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetProcessorIndex(ULONG ProcessorIndex) override
    {
        this->eventRecord.BufferContext.ProcessorIndex = (USHORT)ProcessorIndex;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetThreadId(ULONG ThreadId) override
    {
        this->eventRecord.EventHeader.ThreadId = ThreadId;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetThreadTimes(ULONG KernelTime, ULONG UserTime) override
    {
        this->eventRecord.EventHeader.KernelTime = KernelTime;
        this->eventRecord.EventHeader.UserTime = UserTime;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetActivityId(LPCGUID ActivityId) override
    {
        this->eventRecord.EventHeader.ActivityId = *ActivityId;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetTimeStamp(LARGE_INTEGER *TimeStamp) override
    {
        this->eventRecord.EventHeader.TimeStamp = *TimeStamp;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetProviderId(LPCGUID ProviderId) override
    {
        this->eventRecord.EventHeader.ProviderId = *ProviderId;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppObj) override
    {
        UNREFERENCED_PARAMETER(riid);
        UNREFERENCED_PARAMETER(ppObj);
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return 0;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        return 0;
    }

  private:
    EVENT_RECORD eventRecord;

  public:
      FakeTraceEvent()
      {
      }

      void Initialize(USHORT flags)
      {
          this->eventRecord = { 0 };

          this->eventRecord.EventHeader.Flags = flags;

          if ((flags & EVENT_HEADER_FLAG_CLASSIC_HEADER) != 0)
          {
              this->eventRecord.EventHeader.EventProperty = EVENT_HEADER_PROPERTY_LEGACY_EVENTLOG;
              this->eventRecord.EventHeader.HeaderType = 0;
              this->eventRecord.EventHeader.Size = (USHORT)sizeof(EVENT_HEADER);
          }
          else
          {
              this->eventRecord.EventHeader.EventProperty = EVENT_HEADER_PROPERTY_XML;
              this->eventRecord.EventHeader.HeaderType = 1;
              this->eventRecord.EventHeader.Size = (USHORT)sizeof(EVENT_HEADER);
          }
      }
};

class FakeTraceRelogger : public ITraceRelogger
{
  public:
    HRESULT SetUserContext(ContextInfo *context)
    {
        this->userContext = context;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE AddLogfileTraceStream(BSTR LogfileName, void *UserContext, TRACEHANDLE *TraceHandle) override
    {
        UNREFERENCED_PARAMETER(LogfileName);
        UNREFERENCED_PARAMETER(UserContext);
        UNREFERENCED_PARAMETER(TraceHandle);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE AddRealtimeTraceStream(BSTR LoggerName, void *UserContext, TRACEHANDLE *TraceHandle) override
    {
        UNREFERENCED_PARAMETER(LoggerName);
        UNREFERENCED_PARAMETER(UserContext);
        UNREFERENCED_PARAMETER(TraceHandle);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE RegisterCallback(ITraceEventCallback *Callback) override
    {
        UNREFERENCED_PARAMETER(Callback);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Inject(ITraceEvent *Event) override
    {
        PEVENT_RECORD eventRecord;
        Event->GetEventRecord(&eventRecord);
        eventRecord->UserContext = this->userContext;

        BOOL enableMetadata = this->userContext->EnableMetadata;
        BOOL enableSymbolServerInfo = this->userContext->EnableSymbolServerInfo;

        this->userContext->EnableMetadata = FALSE;
        this->userContext->EnableSymbolServerInfo = FALSE;

        this->callback(eventRecord);

        this->userContext->EnableMetadata = enableMetadata;
        this->userContext->EnableSymbolServerInfo = enableSymbolServerInfo;

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CreateEventInstance(TRACEHANDLE TraceHandle, ULONG Flags, ITraceEvent **Event) override
    {
        UNREFERENCED_PARAMETER(TraceHandle);
        this->fakeTraceEvent.Initialize((USHORT)Flags);
        *Event = &this->fakeTraceEvent; // NOTE: This is valid because all calls to CreateEventInstance are within the lifetime of this Relogger, and there is no recursion.
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ProcessTrace(void) override
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetOutputFilename(BSTR LogfileName) override
    {
        UNREFERENCED_PARAMETER(LogfileName);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetCompressionMode(BOOLEAN CompressionMode) override
    {
        UNREFERENCED_PARAMETER(CompressionMode);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Cancel(void) override
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppObj) override
    {
        UNREFERENCED_PARAMETER(riid);
        UNREFERENCED_PARAMETER(ppObj);
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return 0;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        return 0;
    }

  private:
    PEVENT_RECORD_CALLBACK callback;
    ContextInfo *userContext;
    FakeTraceEvent fakeTraceEvent;

  public:
    FakeTraceRelogger(PEVENT_RECORD_CALLBACK callback)
    {
        this->callback = callback;
    }
};