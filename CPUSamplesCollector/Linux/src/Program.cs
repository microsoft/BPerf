// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using System.Threading;
    using static NativeMethods;

    internal static class Program
    {
        internal const int RingBufferSize = 64 * 1024;

        private const int EPERM = 1;

        public static void Main()
        {
            const int MaxStackSize = 32768; // 32K is the max for a few things (like per cpu array)

            const int MaxEntries = 32768;

            if (!SetMaxResourceLimit())
            {
                ConsoleWriteLine(in Strings.SetRLimitFailed);
                return;
            }

            using var percpuarray = new LinuxFileDescriptor(BPFCreatePerCPUArray(MaxStackSize, 1));
            if (percpuarray.FileDescriptor == -EPERM)
            {
                ConsoleWriteLine(in Strings.BPFCreatePerCPUArrayError);
                return;
            }

            using var stackmap = new LinuxFileDescriptor(BPFCreateStackTraceMap(MaxEntries)); // 32K entries
            if (stackmap.FileDescriptor == -EPERM)
            {
                ConsoleWriteLine(in Strings.BPFCreateStackTraceMapError);
                return;
            }

            using var perfeventmap = new LinuxFileDescriptor(BPFCreatePerfEventArrayMap());
            if (perfeventmap.FileDescriptor == -EPERM)
            {
                ConsoleWriteLine(in Strings.BPFCreatePerfEventArrayMapError);
                return;
            }

            using var ringBuffers = new RingBuffers(
                new PerfEventAttr
                {
                    SampleType = PerfEventSampleFormat.PERF_SAMPLE_RAW,
                    Type = PerfTypeId.PERF_TYPE_SOFTWARE,
                    Config = PerfTypeSpecificConfig.PERF_COUNT_SW_BPF_OUTPUT,
                    SamplePeriod = 1,
                    WakeupEvents = 1000,
                }, stackalloc RingBuffer[GetNumberOfConfiguredPrcoessors()]);

            for (var i = 0; i < ringBuffers.Length; ++i)
            {
                AssociateBPFMapWithRingBuffer(perfeventmap.FileDescriptor, i, in ringBuffers[i]);
            }

            Span<int> dict = stackalloc int[7]; // TODO: FIXME Hard Coded number!

            dict[4] = perfeventmap.FileDescriptor;
            dict[5] = stackmap.FileDescriptor;
            dict[6] = percpuarray.FileDescriptor;

            using var file = new ReadOnlyLinuxFile(in Strings.StacksEBF);

            Span<byte> stacks = file.Contents;

            for (var i = 0; i < stacks.Length; i += 8)
            {
                if (stacks[i] == 0x18)
                {
                    if ((stacks[i + 1] & 0xF0) >> 4 == 1)
                    {
                        Span<byte> dest = stacks.Slice(i + 4, 4);
                        if (!BitConverter.TryWriteBytes(dest, dict[BitConverter.ToInt32(dest)]))
                        {
                            ConsoleWriteLine(in Strings.BPFMapFixupFailed);
                            return;
                        }
                    }
                }
            }

            Span<byte> log = stackalloc byte[MaxStackSize];

            using var bpfprog = new LinuxFileDescriptor(BPFProgLoad(stacks, log, in Strings.GPL));
            if (bpfprog.FileDescriptor == -EPERM)
            {
                ConsoleWriteLine(in Strings.BPFProgLoadFailed);
                return;
            }

            var attr = new PerfEventAttr
            {
                Type = PerfTypeId.PERF_TYPE_SOFTWARE,
                Config = PerfTypeSpecificConfig.PERF_COUNT_SW_CPU_CLOCK,
                Flags = PerfEventAttrFlags.Freq | PerfEventAttrFlags.MMap | PerfEventAttrFlags.MMap2,
                SampleFreq = 99,
            };

            using var perffds = new PerfEventDescriptors(in attr, bpfprog.FileDescriptor, stackalloc int[GetNumberOfConfiguredPrcoessors()]);

            using var ep = new LinuxFileDescriptor(CreateEPollMonitor());

            for (var i = 0; i < ringBuffers.Length; ++i)
            {
                AddEPollMonitor(ep.FileDescriptor, i, in ringBuffers[i]);
            }

            Span<EPollEvent> events = stackalloc EPollEvent[ringBuffers.Length];

            while (true)
            {
                var numberOfReadyFileDescriptors = EPollWait(ep.FileDescriptor, ref events[0], ringBuffers.Length, 1000);
                if (numberOfReadyFileDescriptors < 0)
                {
                    ConsoleWriteLine(in Strings.EPollWaitFailed);
                    break;
                }

                for (var i = 0; i < numberOfReadyFileDescriptors; ++i)
                {
                    var ptr = ringBuffers[events[i].Index].Address;
                    ProcessRingBuffer(ptr, 4096);
                }
            }

            perffds.KeepAlive();
        }

        private static unsafe void ProcessRingBuffer(IntPtr data, int length)
        {
            var copyMem = stackalloc byte[64 * 1024];

            ref var mmapPage = ref MemoryMarshal.AsRef<PerfEventMMapPage>(new Span<byte>((void*)data, length));

            var dataHead = Volatile.Read(ref mmapPage.DataHead);

            var dataTail = mmapPage.DataTail;
            var basePtr = (byte*)(data.ToInt64() + GetMemoryPageSize());

            long copySize = 0;

            while (dataHead != dataTail)
            {
                var eventHeader = (PerfEventHeader*)(basePtr + (dataTail & (RingBufferSize - 1)));
                long eventHeaderSize = eventHeader->Size;

                if ((byte*)eventHeader + eventHeaderSize > basePtr + RingBufferSize)
                {
                    var copyStart = (byte*)eventHeader;

                    if (copySize < eventHeaderSize)
                    {
                        copySize = eventHeaderSize;
                    }

                    var lenFirst = basePtr + RingBufferSize - copyStart;

                    Buffer.MemoryCopy(copyStart, copyMem, 64 * 1024, lenFirst);
                    Buffer.MemoryCopy(basePtr, copyMem + lenFirst, 64 * 1024, eventHeaderSize - lenFirst);
                    eventHeader = (PerfEventHeader*)copyMem;
                }

                ProcessPerfRecord(eventHeader);

                dataTail += (ulong)eventHeaderSize;
            }

            Volatile.Write(ref mmapPage.DataTail, dataTail);
        }

        private static unsafe void ProcessPerfRecord(PerfEventHeader* data)
        {
            var ptr = (byte*)data;
            ptr += Unsafe.SizeOf<PerfEventHeader>();
            var rawSize = *(int*)ptr;

            ptr += sizeof(int);

            ref readonly var bperfevent = ref MemoryMarshal.AsRef<BPerfEvent>(new ReadOnlySpan<byte>(ptr, rawSize));

            Console.WriteLine($"Type: {data->Type}, Size: {data->Size}, Misc: {data->Misc}, Raw Size: {rawSize}");
            Console.WriteLine(bperfevent);
        }

        // uprobe, tracepoint, bpf output set wakeup=1, sample_period=1
        // makese sense because we want all uprobes tracepoint and bpf outputs to wake up
        // maybe bpf output needs to be revisited, it should be sample period 1 but watermark more
        private static int CreateUProbePerfEvent(int cpu, ulong probeOffsetInElfFile, ulong probePath)
        {
            if (System.Buffers.Text.Utf8Parser.TryParse(new ReadOnlyLinuxFile(in Strings.UProbeType).Contents, out int dynamicPMU, out _))
            {
                var attr = new PerfEventAttr
                {
                    Type = (PerfTypeId)dynamicPMU,
                    SamplePeriod = 1,
                    WakeupEvents = 1,
                    UProbePath = probePath,
                    ProbeOffset = probeOffsetInElfFile,
                    Size = (uint)Unsafe.SizeOf<PerfEventAttr>(),
                };

                return PerfEventOpen(in attr, cpu);
            }

            return -1;
        }

        private static bool AddEPollMonitor(int epfd, int index, in RingBuffer r)
        {
            EPollEvent data;
            data.Events = EPollFlags.EPOLLIN;
            data.Index = index;
            data.Unused = 0;

            const int EPOLL_CTL_ADD = 1;
            if (EPollControl(epfd, EPOLL_CTL_ADD, r.FileDescriptor, in data) == -1)
            {
                ConsoleWriteLine(in Strings.EPollControlFailed);
                return false;
            }

            return true;
        }

        private static int CreateEPollMonitor()
        {
            const int EPOLL_CLOEXEC = 0x80000;

            var epfd = EPollCreate(EPOLL_CLOEXEC);
            if (epfd < 0)
            {
                ConsoleWriteLine(in Strings.EPollCreateFailed);
                return -1;
            }

            return epfd;
        }

        private static bool AssociateBPFMapWithRingBuffer(int mapfd, int cpu, in RingBuffer r)
        {
            if (BPFUpdateElem(mapfd, in cpu, in r.FileDescriptor) == -1)
            {
                ConsoleWriteLine(in Strings.BPFUpdateElemFailed);
                return false;
            }

            if (PerfEventEnable(r.FileDescriptor) == -1)
            {
                ConsoleWriteLine(in Strings.PerfEventEnableFailed);
                return false;
            }

            return true;

            // Look at https://github.com/netdata/kernel-collector/blob/51ea6ac98717b3c9b8b16ec7e7018ecd929accd2/library/trace_helpers.c
            // Look at https://github.com/netdata/kernel-collector/blob/efd908d13be811133339ef4fa87de532858869e6/user/process_user.c
            // Look at perf_event_mmap_header
            // talks about ksyms
            // loads perf_event_mmap_header
            // bpf_perf_event_print

            /*
             * // DebugFS is the filesystem type for debugfs.
DebugFS = "debugfs"

// TraceFS is the filesystem type for tracefs.
TraceFS = "tracefs"

// ProcMounts is the mount point for file systems in procfs.
ProcMounts = "/proc/mounts"

// PerfMaxStack is the mount point for the max perf event size.
PerfMaxStack = "/proc/sys/kernel/perf_event_max_stack"

// PerfMaxContexts is a sysfs mount that contains the max perf contexts.
PerfMaxContexts = "/proc/sys/kernel/perf_event_max_contexts_per_stack"

// SyscallsDir is a constant of the default tracing event syscalls directory.
SyscallsDir = "/sys/kernel/debug/tracing/events/syscalls/"

// TracingDir is a constant of the default tracing directory.
TracingDir = "/sys/kernel/debug/tracing"

             */
        }
    }
}
