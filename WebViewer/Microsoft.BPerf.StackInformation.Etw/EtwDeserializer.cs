// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Etw
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using System.Text;
    using Microsoft.BPerf.ModuleInformation.Abstractions;
    using Microsoft.BPerf.StackAggregation;
    using Microsoft.Diagnostics.Tracing.Stacks;
    using static RawReaders;

    public sealed class EtwDeserializer : IStackSamplesProvider, IImageInformationProvider
    {
        private const int ILToNativeMap = 190;

        private const int ILToNativeMapDCStart = 149;

        private const int ILToNativeMapDCStop = 150;

        private const int ModuleLoad = 152;

        private const int ModuleDCStart = 153;

        private const int ModuleDCStop = 154;

        private const int LoadVerbose = 143;

        private const int DCStartVerbose = 143;

        private const int DCStopVerbose = 144;

        private static readonly Guid TcpIpEvent = new Guid("{2f07e2ee-15db-40f1-90ef-9d7ba282188a}");

        private static readonly Guid SampleEvent = new Guid("{ce1dbfb4-137e-4da6-87b0-3f59aa102cbc}");

        private static readonly Guid KernelStackWalkEvent = new Guid("{def2fe46-7bd6-4b80-bd94-f57fe20d0ce3}");

        private static readonly Guid ProcessLoadEvent = new Guid("{3d6fa8d0-fe05-11d0-9dda-00c04fd7ba7c}");

        private static readonly Guid ImageLoadEvent = new Guid("{2cb15d1d-5fc1-11d2-abe1-00a0c911f518}");

        private static readonly Guid DbgIdEvent = new Guid("{b3e675d7-2554-4f18-830b-2762732560de}");

        private static readonly Guid ClrProviderGuid = new Guid("{e13c0d23-ccbc-4e12-931b-d9cc2eee27e4}");

        private static readonly Guid TplProviderGuid = new Guid("{2e5dba47-a3d2-4d16-8ee0-6671ffdcd7b5}");

        private static readonly Guid ClrRundownProviderGuid = new Guid("{a669021c-c450-4609-a035-5af59af4df18}");

        private readonly Dictionary<Frame, int> pidEipToFrameIndexTable;

        private readonly Dictionary<StackInfo, int> stackIndexTable;

        private readonly Dictionary<uint, int> threadPseudoFrameMappingTable;

        private readonly Dictionary<uint, int> processPseudoFrameMappingTable;

        private readonly Dictionary<CorrelatedStackEvent, int> samplesTimeStampMap;

        private readonly Dictionary<EtwProviderInfo, StackEventType> etwProviderInfoMap;

        private readonly HashSet<int> callerIdsNeedingUpdates;

        private readonly long perfFreq;

        private int etwProviderInfoMapIndex;

        private long sessionStartTimeQPC;

        public EtwDeserializer(string fileName)
        {
            this.MethodILNativeMap = new Dictionary<ulong, ManagedMethodILToNativeMapping>();
            this.ImageToDebugInfoMap = new Dictionary<ProcessImageId, DbgId>();
            this.ManagedModuleInfoMap = new Dictionary<ProcessModuleId, ManagedModuleInfo>();
            this.ImageLoadMap = new Dictionary<uint, List<ImageInfo>>();
            this.MethodLoadMap = new Dictionary<uint, List<ManagedMethodInfo>>();
            this.Stacks = new List<StackInfo>();
            this.Frames = new List<Frame>();
            this.Samples = new List<StackSourceSample>();
            this.PseudoFrames = new List<string>();
            this.EventStacks = new Dictionary<int, StackEventType>();
            this.ProcessIdToNameMap = new Dictionary<uint, string>();
            var fileSessions = new EVENT_TRACE_LOGFILEW[1];
            var handles = new ulong[1];

            unsafe
            {
                fileSessions[0] = new EVENT_TRACE_LOGFILEW
                {
                    LogFileName = fileName,
                    EventRecordCallback = this.Deserialize,
                    BufferCallback = this.BufferCallback,
                    LogFileMode = Etw.PROCESS_TRACE_MODE_EVENT_RECORD | Etw.PROCESS_TRACE_MODE_RAW_TIMESTAMP
                };
            }

            handles[0] = Etw.OpenTrace(ref fileSessions[0]);

            unchecked
            {
                if (handles[0] == (ulong)~0)
                {
                    switch (Marshal.GetLastWin32Error())
                    {
                        case 0x57:
                            throw new Exception($"ERROR: For file: {fileName} Windows returned 0x57 -- The Logfile parameter is NULL.");
                        case 0xA1:
                            throw new Exception($"ERROR: For file: {fileName} Windows returned 0xA1 -- The specified path is invalid.");
                        case 0x5:
                            throw new Exception($"ERROR: For file: {fileName} Windows returned 0x5 -- Access is denied.");
                        default:
                            throw new Exception($"ERROR: For file: {fileName} Windows returned an unknown error.");
                    }
                }
            }

            this.perfFreq = fileSessions[0].LogfileHeader.PerfFreq;

            this.pidEipToFrameIndexTable = new Dictionary<Frame, int>();
            this.stackIndexTable = new Dictionary<StackInfo, int>();
            this.threadPseudoFrameMappingTable = new Dictionary<uint, int>();
            this.processPseudoFrameMappingTable = new Dictionary<uint, int>();
            this.callerIdsNeedingUpdates = new HashSet<int>();
            this.samplesTimeStampMap = new Dictionary<CorrelatedStackEvent, int>();
            this.etwProviderInfoMap = new Dictionary<EtwProviderInfo, StackEventType>();

            EtwDeserializerEventSource.Logger.BeginProcessTrace();
            Etw.ProcessTrace(handles, (uint)handles.Length, IntPtr.Zero, IntPtr.Zero);
            EtwDeserializerEventSource.Logger.EndProcessTrace();

            Etw.CloseTrace(handles[0]);

            unsafe
            {
                fileSessions[0] = new EVENT_TRACE_LOGFILEW
                {
                    LogFileName = fileName,
                    EventRecordCallback = this.Deserialize2ndPass,
                    BufferCallback = this.BufferCallback,
                    LogFileMode = Etw.PROCESS_TRACE_MODE_EVENT_RECORD | Etw.PROCESS_TRACE_MODE_RAW_TIMESTAMP
                };
            }

            handles[0] = Etw.OpenTrace(ref fileSessions[0]);

            EtwDeserializerEventSource.Logger.BeginProcessTrace();
            Etw.ProcessTrace(handles, (uint)handles.Length, IntPtr.Zero, IntPtr.Zero);
            EtwDeserializerEventSource.Logger.EndProcessTrace();

            Etw.CloseTrace(handles[0]);

            GC.KeepAlive(fileSessions);

            EtwDeserializerEventSource.Logger.BeginMapProcessIdToNames();

            foreach (var pseudoFrameInfo in this.processPseudoFrameMappingTable)
            {
                this.PseudoFrames[pseudoFrameInfo.Value] = this.ProcessIdToNameMap.TryGetValue(pseudoFrameInfo.Key, out var processName) ? $"Process {processName} ({pseudoFrameInfo.Key})" : $"Process ({pseudoFrameInfo.Key})";
            }

            foreach (var pseudoFrameInfo in this.threadPseudoFrameMappingTable)
            {
                this.PseudoFrames[pseudoFrameInfo.Value] = $"Thread ({pseudoFrameInfo.Key})";
            }

            EtwDeserializerEventSource.Logger.EndMapProcessIdToNames();

            this.pidEipToFrameIndexTable = null;
            this.stackIndexTable = null;
            this.threadPseudoFrameMappingTable = null;
            this.processPseudoFrameMappingTable = null;
            this.samplesTimeStampMap = null;
            this.etwProviderInfoMap = null;

            this.PseudoFramesStartOffset = this.Frames.Count;

            EtwDeserializerEventSource.Logger.BeginFixupCallerIndices();

            foreach (var callerId in this.callerIdsNeedingUpdates)
            {
                var frameIndex = this.Frames.Count;
                var stack = this.Stacks[callerId];
                this.Stacks[callerId] = new StackInfo(stack.CallerID, frameIndex);
                this.Frames.Add(new Frame((uint)stack.FrameID, 0));  // hack alert: we reuse the pid field as a pseudo frame id
            }

            this.callerIdsNeedingUpdates = null;

            EtwDeserializerEventSource.Logger.EndFixupCallerIndices();

            EtwDeserializerEventSource.Logger.BeginSortImages();

            foreach (var elem in this.ImageLoadMap.Values)
            {
                elem.Sort();
            }

            EtwDeserializerEventSource.Logger.EndSortImages();

            EtwDeserializerEventSource.Logger.BeginSortMethods();

            foreach (var elem in this.MethodLoadMap.Values)
            {
                elem.Sort();
            }

            EtwDeserializerEventSource.Logger.EndSortMethods();

            EtwDeserializerEventSource.Logger.BeginPopulateInstructionPointers();

            var frames = this.Frames;
            foreach (var frame in frames)
            {
                if (this.ImageLoadMap.TryGetValue(frame.ProcessId, out var imageList))
                {
                    var eip = frame.InstructionPointer;
                    var result = RangeBinarySearch(imageList, eip);
                    if (result >= 0)
                    {
                        imageList[result].InstructionPointers.Add(eip);
                    }
                }
            }

            /* special process */
            if (this.ImageLoadMap.TryGetValue(0, out var pidZeroList))
            {
                foreach (var frame in frames)
                {
                    var eip = frame.InstructionPointer;
                    var result = RangeBinarySearch(pidZeroList, eip);
                    if (result >= 0)
                    {
                        // add eips encountered in other processes to this one.
                        // we do this so when we lookup pdbs we have all eips
                        // for these images
                        pidZeroList[result].InstructionPointers.Add(eip);
                    }
                }
            }

            EtwDeserializerEventSource.Logger.EndPopulateInstructionPointers();
        }

        public Dictionary<ulong, ManagedMethodILToNativeMapping> MethodILNativeMap { get; }

        public Dictionary<ProcessModuleId, ManagedModuleInfo> ManagedModuleInfoMap { get; }

        public Dictionary<uint, List<ImageInfo>> ImageLoadMap { get; }

        public Dictionary<uint, List<ManagedMethodInfo>> MethodLoadMap { get; }

        public Dictionary<ProcessImageId, DbgId> ImageToDebugInfoMap { get; }

        public List<StackSourceSample> Samples { get; }

        public Dictionary<int, StackEventType> EventStacks { get; }

        public Dictionary<uint, string> ProcessIdToNameMap { get; }

        public List<Frame> Frames { get; }

        public List<StackInfo> Stacks { get; }

        public int PseudoFramesStartOffset { get; }

        public List<string> PseudoFrames { get; }

        public double MaxTimeRelativeMSec { get; private set; }

        private static unsafe void ProcessInfoEventV4(ref byte* data, int pointerSize, out uint pid, out string imageFileName)
        {
            SkipPointer(ref data, pointerSize); // uniqueProcessKey
            pid = ReadAlignedUInt32(ref data);
            SkipUInt32(ref data); // parentId
            SkipUInt32(ref data); // sessionId
            SkipUInt32(ref data); // exitStatus
            SkipPointer(ref data, pointerSize); // directoryTableBase
            SkipUInt32(ref data); // flags
            SkipUserSID(ref data, pointerSize);
            imageFileName = ReadAnsiNullTerminatedString(ref data);
        }

        private static unsafe void ProcessInfoEventV3(ref byte* data, int pointerSize, out uint pid, out string imageFileName)
        {
            SkipPointer(ref data, pointerSize); // uniqueProcessKey
            pid = ReadAlignedUInt32(ref data);
            SkipUInt32(ref data); // parentId
            SkipUInt32(ref data); // sessionId
            SkipUInt32(ref data); // exitStatus
            SkipPointer(ref data, pointerSize); // directoryTableBase
            SkipUserSID(ref data, pointerSize);
            imageFileName = ReadAnsiNullTerminatedString(ref data);
        }

        private static unsafe void ImgLoadV3(ref byte* data, int pointerSize, out ulong imageBase, out uint size, out uint pid, out string imageFileName)
        {
            imageBase = ReadAlignedPointer(ref data, pointerSize);
            size = (uint)ReadAlignedPointer(ref data, pointerSize);
            pid = ReadAlignedUInt32(ref data);
            SkipUInt32(ref data); // imageCheckSum
            SkipUInt32(ref data); // timeDateStamp
            SkipUInt8(ref data); // signatureLevel
            SkipUInt8(ref data); // signatureType
            SkipUInt16(ref data); // Reserved0
            SkipPointer(ref data, pointerSize); // defaultBase
            SkipUInt32(ref data); // Reserved1
            SkipUInt32(ref data); // Reserved2
            SkipUInt32(ref data); // Reserved3
            SkipUInt32(ref data); // Reserved4
            imageFileName = ReadWideNullTerminatedString(ref data);
        }

        private static unsafe void ImgLoadV2(ref byte* data, int pointerSize, out ulong imageBase, out uint size, out uint pid, out string imageFileName)
        {
            imageBase = ReadAlignedPointer(ref data, pointerSize);
            size = (uint)ReadAlignedPointer(ref data, pointerSize);
            pid = ReadAlignedUInt32(ref data);
            SkipUInt32(ref data); // imageCheckSum
            SkipUInt32(ref data); // timeDateStamp
            SkipUInt16(ref data); // Reserved0
            SkipPointer(ref data, pointerSize); // defaultBase
            SkipUInt32(ref data); // Reserved1
            SkipUInt32(ref data); // Reserved2
            SkipUInt32(ref data); // Reserved3
            SkipUInt32(ref data); // Reserved4
            imageFileName = ReadWideNullTerminatedString(ref data);
        }

        private static string TdhGetManifestEventInformation(Guid providerId, int eventId)
        {
            if (providerId == ProcessLoadEvent)
            {
                switch (eventId)
                {
                    case 1:
                        return "Event(Windows Kernel/Process/Start)";
                    case 2:
                        return "Event(Windows Kernel/Process/Stop)";
                    case 3:
                        return "Event(Windows Kernel/Process/DCStart)";
                    case 4:
                        return "Event(Windows Kernel/Process/DCStop)";
                }
            }

            if (providerId == ImageLoadEvent)
            {
                switch (eventId)
                {
                    case 10:
                        return "Event(Windows Kernel/Image/Load)";
                    case 2:
                        return "Event(Windows Kernel/Image/Unload)";
                    case 3:
                        return "Event(Windows Kernel/Image/DCStart)";
                    case 4:
                        return "Event(Windows Kernel/Image/DCStop)";
                }
            }

            if (providerId == ClrProviderGuid)
            {
                switch (eventId)
                {
                    case 10:
                        return "Event(Microsoft-Windows-DotNETRuntime/GC/AllocationTick)";
                    case 80:
                        return "Event(Microsoft-Windows-DotNETRuntime/Exception/Start)";
                    case 143:
                        return "Event(Microsoft-Windows-DotNETRuntime/Method/LoadVerbose)";
                    case 250:
                        return "Event(Microsoft-Windows-DotNETRuntime/ExceptionCatch/Start)";
                    case 252:
                        return "Event(Microsoft-Windows-DotNETRuntime/ExceptionFinally/Start)";
                }
            }

            if (providerId == TplProviderGuid)
            {
                switch (eventId)
                {
                    case 7:
                        return "Event(System.Threading.Tasks.TplEventSource/TaskSchdeduled/Send)";
                    case 10:
                        return "Event(System.Threading.Tasks.TplEventSource/TaskWait/Send)";
                    case 12:
                        return "Event(System.Threading.Tasks.TplEventSource/TaskContinuationScheduled/Send)";
                }
            }

            if (providerId == SampleEvent && eventId == 46)
            {
                return "Event(Windows Kernel/PerfInfo/Sample)";
            }

            if (providerId == TcpIpEvent && eventId == 1013)
            {
                return "Event(Microsoft-Windows-TCPIP/TcpCreateEndpointComplete)";
            }

            return $"Event({providerId})/{eventId}";
        }

        private static int RangeBinarySearch<T>(List<T> sortedArray, ulong frame)
            where T : SearchableInfo
        {
            return RangeBinarySearch(sortedArray, 0, sortedArray.Count - 1, frame);
        }

        private static int RangeBinarySearch<T>(List<T> sortedArray, int first, int last, ulong frame)
            where T : SearchableInfo
        {
            if (first <= last)
            {
                int mid = (first + last) / 2;

                // if the frame is imageBase or within its size
                if (frame >= sortedArray[mid].Begin && frame <= sortedArray[mid].End)
                {
                    return mid;
                }

                if (frame < sortedArray[mid].Begin)
                {
                    return RangeBinarySearch(sortedArray, first, mid - 1, frame);
                }

                return RangeBinarySearch(sortedArray, mid + 1, last, frame);
            }

            return -1;
        }

        private bool BufferCallback(IntPtr logfile)
        {
            return true;
        }

        private unsafe void Deserialize2ndPass(EVENT_RECORD* eventRecord)
        {
            uint threadId = eventRecord->ThreadId;
            int pointerSize = (eventRecord->Flags & Etw.EVENT_HEADER_FLAG_64_BIT_HEADER) != 0 ? 8 : 4;

            if (eventRecord->ProviderId == SampleEvent && eventRecord->Opcode == 46)
            {
                threadId = *(uint*)(eventRecord->UserData + pointerSize);
            }

            if (this.samplesTimeStampMap.TryGetValue(new CorrelatedStackEvent(eventRecord->TimeStamp, threadId), out var index))
            {
                var providerId = eventRecord->ProviderId;
                var eventId = (eventRecord->Flags & Etw.EVENT_HEADER_FLAG_CLASSIC_HEADER) != 0 ? eventRecord->Opcode : eventRecord->Id;
                var key = new EtwProviderInfo(eventRecord->ProviderId, eventRecord->Id, eventRecord->Opcode);

                if (!this.etwProviderInfoMap.TryGetValue(key, out var stackEventType))
                {
                    var id = this.etwProviderInfoMapIndex++;
                    stackEventType = new StackEventType(id, TdhGetManifestEventInformation(providerId, eventId));
                    this.etwProviderInfoMap.Add(key, stackEventType);
                    this.EventStacks.Add(id, stackEventType);
                }

                stackEventType.SampleIndices.Add(index);
            }
        }

        private unsafe void Deserialize(EVENT_RECORD* eventRecord)
        {
            var providerId = eventRecord->ProviderId;
            var userData = eventRecord->UserData; // 8-byte aligned
            int pointerSize = (eventRecord->Flags & Etw.EVENT_HEADER_FLAG_64_BIT_HEADER) != 0 ? 8 : 4;

            if (this.sessionStartTimeQPC == 0)
            {
                this.sessionStartTimeQPC = eventRecord->TimeStamp;
            }

            /* https://msdn.microsoft.com/en-us/library/windows/desktop/dd392323(v=vs.85).aspx
            [EventType{32}, EventTypeName{"Stack"}]
            class StackWalk_Event : StackWalk
            {
              uint64 EventTimeStamp;
              uint32 StackProcess;
              uint32 StackThread;
              uint32 Stack1;
              uint32 Stack192;
            }; */
            if (eventRecord->ProviderId == KernelStackWalkEvent && eventRecord->Opcode == 32)
            {
                long eventTimestamp = ReadAlignedInt64(ref userData);
                uint pid = ReadAlignedUInt32(ref userData);
                uint tid = ReadAlignedUInt32(ref userData);
                int frameCount = (eventRecord->UserDataLength - sizeof(long) - sizeof(uint) - sizeof(uint)) / pointerSize;

                this.HandleStacks(userData, frameCount, eventTimestamp, pointerSize, pid, tid);

                return;
            }

            if (eventRecord->ExtendedDataCount > 0)
            {
                this.HandleExtendedData(eventRecord);
            }

            if ((providerId == ClrProviderGuid && eventRecord->Id == LoadVerbose) || (providerId == ClrRundownProviderGuid && (eventRecord->Id == DCStartVerbose || eventRecord->Id == DCStopVerbose)))
            {
                this.HandleMethodLoadEvent(eventRecord, userData);
                return;
            }

            if ((providerId == ClrProviderGuid && eventRecord->Id == ILToNativeMap) || (providerId == ClrRundownProviderGuid && (eventRecord->Id == ILToNativeMapDCStart || eventRecord->Id == ILToNativeMapDCStop)))
            {
                this.HandleILToNativeMappingEvent(userData);
                return;
            }

            if (providerId == ProcessLoadEvent)
            {
                this.HandleProcessEvents(eventRecord, userData, pointerSize);
                return;
            }

            if (providerId == ImageLoadEvent)
            {
                this.HandleImageLoadEvent(eventRecord, userData, pointerSize);
                return;
            }

            if (providerId == DbgIdEvent)
            {
                this.HandleDbgIdEvent(eventRecord, userData, pointerSize);
                return;
            }

            if ((providerId == ClrProviderGuid && eventRecord->Id == ModuleLoad) || (providerId == ClrRundownProviderGuid && (eventRecord->Id == ModuleDCStart || eventRecord->Id == ModuleDCStop)))
            {
                this.HandleModuleLoadEvent(eventRecord, userData);
                return;
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private int AddOrUpdateCallerIndex(int callerIndex, int frameIndex)
        {
            var stackInfo = new StackInfo(callerIndex, frameIndex);
            if (this.stackIndexTable.TryGetValue(stackInfo, out int tmp))
            {
                callerIndex = tmp;
            }
            else
            {
                callerIndex = this.Stacks.Count;
                this.stackIndexTable.Add(stackInfo, callerIndex);
                this.Stacks.Add(stackInfo);
            }

            return callerIndex;
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private unsafe void HandleExtendedData(EVENT_RECORD* eventRecord)
        {
            for (ushort i = 0; i < eventRecord->ExtendedDataCount; ++i)
            {
                if (eventRecord->ExtendedData[i].ExtType == Etw.EVENT_HEADER_EXT_TYPE_STACK_TRACE32)
                {
                    int frameCount = (eventRecord->ExtendedData[i].DataSize - sizeof(ulong)) / sizeof(uint);
                    this.HandleStacks((byte*)eventRecord->ExtendedData[i].DataPtr, frameCount, eventRecord->TimeStamp, 4, eventRecord->ProcessId, eventRecord->ThreadId);
                }

                if (eventRecord->ExtendedData[i].ExtType == Etw.EVENT_HEADER_EXT_TYPE_STACK_TRACE64)
                {
                    int frameCount = (eventRecord->ExtendedData[i].DataSize - sizeof(ulong)) / sizeof(ulong);
                    this.HandleStacks((byte*)eventRecord->ExtendedData[i].DataPtr + sizeof(ulong), frameCount, eventRecord->TimeStamp, 8, eventRecord->ProcessId, eventRecord->ThreadId);
                }
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private unsafe void HandleStacks(byte* userData, int frameCount, long eventTimestamp, int pointerSize, uint pid, uint tid)
        {
            int callerIndex = -1;

            if (!this.processPseudoFrameMappingTable.TryGetValue(pid, out var offset))
            {
                offset = this.PseudoFrames.Count;
                this.processPseudoFrameMappingTable.Add(pid, offset);
                this.PseudoFrames.Add(string.Empty); // 2nd-pass will fill in Process w3wp (pid)
            }

            callerIndex = this.AddOrUpdateCallerIndex(callerIndex, offset);
            this.callerIdsNeedingUpdates.Add(callerIndex);

            if (!this.threadPseudoFrameMappingTable.TryGetValue(tid, out offset))
            {
                offset = this.PseudoFrames.Count;
                this.threadPseudoFrameMappingTable.Add(tid, offset);
                this.PseudoFrames.Add(string.Empty); // 2nd-pass will fill in Thread (tid)
            }

            callerIndex = this.AddOrUpdateCallerIndex(callerIndex, offset);
            this.callerIdsNeedingUpdates.Add(callerIndex);

            for (int i = frameCount - 1; i > -1; i--)
            {
                var eip = pointerSize == 8 ? *(ulong*)(userData + (i * 8)) : (ulong)*(int*)(userData + (i * 4));

                var key = new Frame(pid, eip);
                if (!this.pidEipToFrameIndexTable.TryGetValue(key, out int frameIndex))
                {
                    frameIndex = this.Frames.Count;
                    this.pidEipToFrameIndexTable.Add(key, frameIndex);
                    this.Frames.Add(key);
                }

                callerIndex = this.AddOrUpdateCallerIndex(callerIndex, frameIndex);
            }

            StackSource dummy = null;
            var sample = new StackSourceSample(dummy)
            {
                Scenario = (int)pid, // hack alert: we use scenario as pid, it's just so that I can take source integrates for Stacks.cs
                TimeRelativeMSec = (eventTimestamp - this.sessionStartTimeQPC) * 1000.0 / this.perfFreq,
                Metric = 1.0F,
                StackIndex = (StackSourceCallStackIndex)callerIndex,
                SampleIndex = (StackSourceSampleIndex)this.Samples.Count
            };

            var endTime = sample.TimeRelativeMSec + 1.0;
            if (endTime > this.MaxTimeRelativeMSec)
            {
                this.MaxTimeRelativeMSec = endTime;
            }

            // TODO: BUG: yes, it turns out we can multiple stacks for the same event, we just throw older ones.
            // primarily because with whom do we associate it?
            var key2 = new CorrelatedStackEvent(eventTimestamp, tid);
            if (!this.samplesTimeStampMap.ContainsKey(key2))
            {
                this.samplesTimeStampMap.Add(key2, this.Samples.Count);
            }

            this.Samples.Add(sample);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private unsafe void HandleImageLoadEvent(EVENT_RECORD* eventRecord, byte* userData, int pointerSize)
        {
            switch (eventRecord->Opcode)
            {
                case 3: // DCStart
                case 4: // DCStop
                case 10: // Load
                {
                    var data = userData;

                    ulong imageBase;
                    uint size;
                    uint pid;
                    string imageFileName;

                    switch (eventRecord->Version)
                    {
                        case 3:
                            ImgLoadV3(ref data, pointerSize, out imageBase, out size, out pid, out imageFileName);
                            break;
                        case 2:
                            ImgLoadV2(ref data, pointerSize, out imageBase, out size, out pid, out imageFileName);
                            break;
                        default:
                            return;
                    }

                    if (!this.ImageLoadMap.TryGetValue(pid, out List<ImageInfo> imageList))
                    {
                        imageList = new List<ImageInfo>();
                        this.ImageLoadMap.Add(pid, imageList);
                    }

                    bool duplicate = false;
                    foreach (var t in imageList)
                    {
                        if (t.Begin == imageBase)
                        {
                            duplicate = true;
                            break;
                        }
                    }

                    if (!duplicate)
                    {
                        imageList.Add(new ImageInfo(imageBase, imageBase + size, imageFileName));
                    }

                    break;
                }
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private unsafe void HandleDbgIdEvent(EVENT_RECORD* eventRecord, byte* userData, int pointerSize)
        {
            if (eventRecord->Opcode == 36 && eventRecord->Version == 2)
            {
                var data = userData;

                ulong imageBase = ReadAlignedPointer(ref data, pointerSize);
                var key = new ProcessImageId(eventRecord->ProcessId, imageBase);

                if (!this.ImageToDebugInfoMap.ContainsKey(key))
                {
                    SkipUInt32(ref data); // always ff ff
                    var signature = ReadAlignedGuid(ref data);
                    uint age = ReadAlignedUInt32(ref data);
                    this.ImageToDebugInfoMap.Add(key, new DbgId(signature, age, Encoding.ASCII.GetString(data, eventRecord->UserDataLength - 32 - 1)));
                }
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private unsafe void HandleModuleLoadEvent(EVENT_RECORD* eventRecord, byte* userData)
        {
            if (eventRecord->Version == 2)
            {
                var data = userData;

                ulong moduleId = ReadAlignedUInt64(ref data);
                var processModuleId = new ProcessModuleId(eventRecord->ProcessId, moduleId);

                if (!this.ManagedModuleInfoMap.ContainsKey(processModuleId))
                {
                    SkipUInt64(ref data); // assemblyId
                    SkipUInt32(ref data); // moduleFlags
                    SkipUInt32(ref data); // reserved
                    string moduleILPath = ReadWideNullTerminatedString(ref data);
                    string moduleNativePath = ReadWideNullTerminatedString(ref data);
                    SkipUInt16(ref data); // clrInstanceId
                    Guid managedSignature = ReadGuid(ref data);
                    uint managedPdbAge = ReadUInt32(ref data);
                    string managedPdbPath = ReadWideNullTerminatedString(ref data);
                    Guid nativeSignature = ReadGuid(ref data);
                    uint nativePdbAge = ReadUInt32(ref data);
                    string nativePdbPath = ReadWideNullTerminatedString(ref data);

                    this.ManagedModuleInfoMap.Add(processModuleId, new ManagedModuleInfo(moduleILPath, moduleNativePath, managedSignature, managedPdbAge, nativeSignature, nativePdbAge, managedPdbPath, nativePdbPath));
                }
            }
        }

        private unsafe void HandleMethodLoadEvent(EVENT_RECORD* eventRecord, byte* userData)
        {
            if (eventRecord->Version <= 2)
            {
                var data = userData;

                ulong methodId = ReadAlignedUInt64(ref data);
                ulong moduleId = ReadAlignedUInt64(ref data);
                ulong methodStartAddress = ReadAlignedUInt64(ref data);
                uint methodSize = ReadAlignedUInt32(ref data);
                uint methodToken = ReadAlignedUInt32(ref data);
                SkipUInt32(ref data); // method flags
                string methodNamespace = ReadWideNullTerminatedString(ref data);
                string methodName = ReadWideNullTerminatedString(ref data);
                string methodSignature = ReadWideNullTerminatedString(ref data);

                var sig = string.Empty;
                var sigIndexOf = methodSignature.IndexOf('(');
                if (sigIndexOf > -1)
                {
                    sig = methodSignature.Substring(sigIndexOf);
                }

                var methodInfo = new ManagedMethodInfo(methodToken, methodId, moduleId, methodStartAddress, methodStartAddress + methodSize, methodNamespace + "." + methodName + sig);

                uint pid = eventRecord->ProcessId;
                if (!this.MethodLoadMap.TryGetValue(pid, out List<ManagedMethodInfo> methodList))
                {
                    methodList = new List<ManagedMethodInfo>();
                    this.MethodLoadMap.Add(pid, methodList);
                }

                methodList.Add(methodInfo);
            }
        }

        private unsafe void HandleProcessEvents(EVENT_RECORD* eventRecord, byte* userData, int pointerSize)
        {
            switch (eventRecord->Opcode)
            {
                case 1: // Start
                case 3: // DCStart
                case 4: // DCStop
                {
                    uint pid;
                    string imageFileName;
                    var data = userData;

                    switch (eventRecord->Version)
                    {
                        case 4:
                            ProcessInfoEventV4(ref data, pointerSize, out pid, out imageFileName);
                            break;
                        case 3:
                            ProcessInfoEventV3(ref data, pointerSize, out pid, out imageFileName);
                            break;
                        default:
                            return;
                    }

                    // TODO: BUG: yes, it turns out we can pid exiting and starting and so it's possible this will be broken but is good enough
                    if (!this.ProcessIdToNameMap.ContainsKey(pid))
                    {
                        this.ProcessIdToNameMap.Add(pid, imageFileName);
                    }

                    break;
                }
            }
        }

        private unsafe void HandleILToNativeMappingEvent(byte* userData)
        {
            var data = userData;

            ulong methodId = ReadAlignedUInt64(ref data);
            SkipUInt64(ref data); // rejitId
            SkipUInt8(ref data); // methodExtent
            ushort count = ReadUInt16(ref data);

            var iloffsets = new int[count];
            for (ushort i = 0; i < count; ++i)
            {
                iloffsets[i] = ReadInt32(ref data);
            }

            var nativeOffsets = new int[count];
            for (ushort i = 0; i < count; ++i)
            {
                nativeOffsets[i] = ReadInt32(ref data);
            }

            if (!this.MethodILNativeMap.ContainsKey(methodId))
            {
                this.MethodILNativeMap.Add(methodId, new ManagedMethodILToNativeMapping(methodId, iloffsets, nativeOffsets));
            }
        }

        private struct EtwProviderInfo : IEquatable<EtwProviderInfo>
        {
            private readonly Guid providerId;

            private readonly int eventId;

            private readonly byte opcode;

            public EtwProviderInfo(Guid providerId, int eventId, byte opcode)
            {
                this.providerId = providerId;
                this.eventId = eventId;
                this.opcode = opcode;
            }

            public bool Equals(EtwProviderInfo other)
            {
                return this.providerId == other.providerId && this.eventId == other.eventId & this.opcode == other.opcode;
            }

            public override int GetHashCode()
            {
                int hash = 17;
                hash = (hash * 31) + this.providerId.GetHashCode();
                hash = (hash * 31) + this.eventId.GetHashCode();
                hash = (hash * 31) + this.opcode.GetHashCode();
                return hash;
            }

            public override bool Equals(object other)
            {
                return this.Equals((EtwProviderInfo)other);
            }
        }

        private struct CorrelatedStackEvent : IEquatable<CorrelatedStackEvent>
        {
            private readonly long timestamp;

            private readonly uint tid;

            public CorrelatedStackEvent(long timestamp, uint tid)
            {
                this.timestamp = timestamp;
                this.tid = tid;
            }

            public bool Equals(CorrelatedStackEvent other)
            {
                return this.timestamp == other.timestamp && this.tid == other.tid;
            }

            public override int GetHashCode()
            {
                unchecked
                {
                    return (this.timestamp.GetHashCode() * 397) ^ (int)this.tid;
                }
            }

            public override bool Equals(object obj)
            {
                return this.Equals((CorrelatedStackEvent)obj);
            }
        }
    }
}
