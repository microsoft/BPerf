#pragma once

#include <windef.h>
#include <evntprov.h>

REGHANDLE RegHandle;

const char* Manifest = "<instrumentationManifest xmlns=\"http://schemas.microsoft.com/win/2004/08/events\">\n"
" <instrumentation xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:win=\"http://manifests.microsoft.com/win/2004/08/windows/events\">\n"
"  <events xmlns=\"http://schemas.microsoft.com/win/2004/08/events\">\n"
"<provider name=\"BPerfCoreCLRProfilerEventSource\" guid=\"{5a0f8851-25a1-54b6-6a29-300728cd9b65}\" resourceFileName=\"BPerfCoreCLRProfilerEventSource\" messageFileName=\"BPerfCoreCLRProfilerEventSource\" symbol=\"BPerfCoreCLRProfilerEventSource\">\n"
" <tasks>\n"
"  <task name=\"ObjectAllocation\" message=\"$(string.task_Log)\" value=\"1\"/>\n"
"  <task name=\"EventSourceMessage\" message=\"$(string.task_EventSourceMessage)\" value=\"65534\"/>\n"
" </tasks>\n"
" <opcodes>\n"
" </opcodes>\n"
" <keywords>\n"
"  <keyword name=\"Session3\" message=\"$(string.keyword_Session3)\" mask=\"0x100000000000\"/>\n"
"  <keyword name=\"Session2\" message=\"$(string.keyword_Session2)\" mask=\"0x200000000000\"/>\n"
"  <keyword name=\"Session1\" message=\"$(string.keyword_Session1)\" mask=\"0x400000000000\"/>\n"
"  <keyword name=\"Session0\" message=\"$(string.keyword_Session0)\" mask=\"0x800000000000\"/>\n"
" </keywords>\n"
" <events>\n"
"  <event value=\"0\" version=\"0\" level=\"win:LogAlways\" symbol=\"EventSourceMessage\" task=\"EventSourceMessage\" template=\"EventSourceMessageArgs\"/>\n"
"  <event value=\"1\" version=\"0\" level=\"win:LogAlways\" symbol=\"ObjectAllocation\" task=\"ObjectAllocation\" template=\"ObjectAllocationArgs\"/>\n"
"  <event value=\"2\" version=\"0\" level=\"win:LogAlways\" symbol=\"TypeLoaded\" task=\"Log\" template=\"TypeLoadedArgs\"/>\n"
" </events>\n"
" <templates>\n"
"  <template tid=\"EventSourceMessageArgs\">\n"
"   <data name=\"message\" inType=\"win:UnicodeString\"/>\n"
"  </template>\n"
"  <template tid=\"ObjectAllocationArgs\">\n"
"   <data name=\"moduleId\" inType=\"win:Pointer\"/>\n"
"   <data name=\"typeDef\" inType=\"win:UInt32\"/>\n"
"   <data name=\"size\" inType=\"win:UInt64\"/>\n"
"  </template>\n"
"  <template tid=\"TypeLoadedArgs\">\n"
"   <data name=\"typeNameLength\" inType=\"win:UInt32\"/>\n"
"   <data name=\"typeName\" inType=\"win:UnicodeString\" length=\"typeNameLength\" />\n"
"  </template>\n"
" </templates>\n"
"</provider>\n"
"</events>\n"
"</instrumentation>\n"
"<localization>\n"
" <resources culture=\"en-US\">\n"
"  <stringTable>\n"
"   <string id=\"keyword_Session0\" value=\"Session0\"/>\n"
"   <string id=\"keyword_Session1\" value=\"Session1\"/>\n"
"   <string id=\"keyword_Session2\" value=\"Session2\"/>\n"
"   <string id=\"keyword_Session3\" value=\"Session3\"/>\n"
"   <string id=\"task_EventSourceMessage\" value=\"EventSourceMessage\"/>\n"
"   <string id=\"task_Log\" value=\"Log\"/>\n"
"  </stringTable>\n"
" </resources>\n"
"</localization>\n"
"</instrumentationManifest>\r\n";

static ULONG WriteManifestEvent(REGHANDLE handle)
{
    EVENT_DESCRIPTOR descriptor;
    descriptor.Id = 65534;
    descriptor.Task = 65534;
    descriptor.Opcode = 254;
    descriptor.Channel = 0;
    descriptor.Level = 0;
    descriptor.Version = 1;
    descriptor.Keyword = 0x00FFFFFFFFFFFFFF;

    EVENT_DATA_DESCRIPTOR dataDescriptors[7];

    BYTE format = 1;

    dataDescriptors[0].Ptr = reinterpret_cast<ULONGLONG>(&format);
    dataDescriptors[0].Size = sizeof(BYTE);
    dataDescriptors[0].Reserved = 0;

    BYTE majorVersion = 1;

    dataDescriptors[1].Ptr = reinterpret_cast<ULONGLONG>(&majorVersion);
    dataDescriptors[1].Size = sizeof(BYTE);
    dataDescriptors[1].Reserved = 0;

    BYTE minorVersion = 0;

    dataDescriptors[2].Ptr = reinterpret_cast<ULONGLONG>(&minorVersion);
    dataDescriptors[2].Size = sizeof(BYTE);
    dataDescriptors[2].Reserved = 0;

    BYTE magic = 91;

    dataDescriptors[3].Ptr = reinterpret_cast<ULONGLONG>(&magic);
    dataDescriptors[3].Size = sizeof(BYTE);
    dataDescriptors[3].Reserved = 0;

    USHORT totalChunks = 1;

    dataDescriptors[4].Ptr = reinterpret_cast<ULONGLONG>(&totalChunks);
    dataDescriptors[4].Size = sizeof(USHORT);
    dataDescriptors[4].Reserved = 0;

    USHORT chunkNumber = 0;

    dataDescriptors[5].Ptr = reinterpret_cast<ULONGLONG>(&chunkNumber);
    dataDescriptors[5].Size = sizeof(USHORT);
    dataDescriptors[5].Reserved = 0;

    dataDescriptors[6].Ptr = reinterpret_cast<ULONGLONG>(Manifest);
    dataDescriptors[6].Size = static_cast<ULONG>(strlen(Manifest));
    dataDescriptors[6].Reserved = 0;
    
    return EventWriteEx(handle, &descriptor, 0, 0, nullptr, nullptr, 7, &dataDescriptors[0]);
}

static void BPerfEventEnableCallback(LPCGUID sourceId, ULONG isEnabled, UCHAR level, ULONGLONG matchAnyKeyword, ULONGLONG matchAllKeyword, EVENT_FILTER_DESCRIPTOR* filterData, void* callbackContext)
{
    WriteManifestEvent(RegHandle);
}

static ULONG ReportObjectAllocation(UINT_PTR moduleId, unsigned typeDef, size_t size)
{
    EVENT_DESCRIPTOR descriptor;
    descriptor.Id = 1;
    descriptor.Task = 1;
    descriptor.Opcode = 1;
    descriptor.Channel = 0;
    descriptor.Level = 0;
    descriptor.Version = 0;
    descriptor.Keyword = 0x0000F00000000000;

    EVENT_DATA_DESCRIPTOR dataDescriptors[3];
    dataDescriptors[0].Ptr = reinterpret_cast<ULONGLONG>(&moduleId);
    dataDescriptors[0].Size = sizeof(UINT_PTR);
    dataDescriptors[0].Reserved = 0;

    dataDescriptors[1].Ptr = reinterpret_cast<ULONGLONG>(&typeDef);
    dataDescriptors[1].Size = sizeof(unsigned);
    dataDescriptors[1].Reserved = 0;

    dataDescriptors[2].Ptr = reinterpret_cast<ULONGLONG>(&size);
    dataDescriptors[2].Size = sizeof(size_t);
    dataDescriptors[2].Reserved = 0;

    return EventWriteEx(RegHandle, &descriptor, 0, 0, nullptr, nullptr, 3, &dataDescriptors[0]);
}

static ULONG RegisterToETW()
{
    // {5a0f8851-25a1-54b6-6a29-300728cd9b65}
    const GUID BPerfProfilerETWProviderGuid = { 0x5a0f8851, 0x25a1, 0x54b6, {0x6a, 0x29, 0x30, 0x07, 0x28, 0xcd, 0x9b, 0x65} };
    EventRegister(&BPerfProfilerETWProviderGuid, &BPerfEventEnableCallback, nullptr, &RegHandle);
    return WriteManifestEvent(RegHandle);
}

static ULONG UnregisterFromETW()
{
    return EventUnregister(RegHandle);
}
