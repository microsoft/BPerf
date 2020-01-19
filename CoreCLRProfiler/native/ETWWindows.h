#pragma once

#include <evntprov.h>
#include <guiddef.h>

REGHANDLE RegHandle;

static void BPerfEventEnableCallback(LPCGUID sourceId, ULONG isEnabled, UCHAR level, ULONGLONG matchAnyKeyword, ULONGLONG matchAllKeyword, EVENT_FILTER_DESCRIPTOR* filterData, void* callbackContext)
{
}

static ULONG ReportLOHAllocation()
{
    EVENT_DESCRIPTOR descriptor;
    descriptor.Id = 1;
    descriptor.Level = 4;
    descriptor.Channel = 0;
    descriptor.Opcode = 0;
    descriptor.Task = 0;
    descriptor.Version = 1;
    descriptor.Keyword = 0;
    return EventWriteEx(RegHandle, &descriptor, 0, 0, nullptr, nullptr, 0, nullptr);
}

static ULONG RegisterToETW()
{
    // {D46E5565-D624-4C4D-89CC-7A82887D3628}
    const GUID BPerfProfilerETWProviderGuid = { 0xD46E5565, 0xD624, 0x4C4D, {0x89, 0xCC, 0x7A, 0x82, 0x88, 0x7D, 0x36, 0x28} };
    return EventRegister(&BPerfProfilerETWProviderGuid, &BPerfEventEnableCallback, nullptr, &RegHandle);
}

static ULONG UnregisterFromETW()
{
    return EventUnregister(RegHandle);
}
