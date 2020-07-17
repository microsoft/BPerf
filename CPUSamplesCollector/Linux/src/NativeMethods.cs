// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace BPerfCPUSamplesCollector
{
    using System;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;

    internal static class NativeMethods
    {
        private const int PERF_EVENT_IOC_ENABLE = 9216;

        private const int PERF_MAX_STACK_DEPTH = 127;

        private const long PERF_EVENT_IOC_SET_BPF = 0x40042408;

        public static unsafe int BPFProgLoad(ReadOnlySpan<byte> instructions, ReadOnlySpan<byte> logBuf, in byte license)
        {
            var attr = new BPFProgLoadAttr(BPFProgType.BPF_PROG_TYPE_PERF_EVENT, instructions.Length / 8, Unsafe.AsPointer(ref Unsafe.AsRef(in instructions.GetPinnableReference())), Unsafe.AsPointer(ref Unsafe.AsRef(in license)), 1, logBuf.Length, Unsafe.AsPointer(ref Unsafe.AsRef(in logBuf.GetPinnableReference())), 0);
            return BPF(BPFCmd.BPF_PROG_LOAD, in attr);
        }

        public static int PerfEventOpen(in PerfEventAttr attr, long cpu, long pid = -1)
        {
            var syscallNumber = RuntimeInformation.OSArchitecture switch
            {
                Architecture.Arm64 => 241,
                Architecture.X64 => 298,
                _ => throw new NotSupportedException()
            };

            const long PERF_FLAG_FD_CLOEXEC = 8;
            var fd = (int)SysCallPerfEventOpen(syscallNumber, in attr, pid, cpu, -1, PERF_FLAG_FD_CLOEXEC);
            if (fd == -1)
            {
                throw new Exception($"perf_event_open failed. errno: {Marshal.GetLastWin32Error()}");
            }

            return fd;
        }

        public static int PerfEventAssociateBPFProgram(int perffd, int progfd)
        {
            return IOControl(perffd, PERF_EVENT_IOC_SET_BPF, progfd);
        }

        public static int PerfEventEnable(int fd)
        {
            return IOControl(fd, PERF_EVENT_IOC_ENABLE, 0);
        }

        public static bool SetMaxResourceLimit()
        {
            var r = new RLimit(ulong.MaxValue, ulong.MaxValue);

            const int RLIMIT_MEMLOCK = 8;
            if (SetResourceLimit(RLIMIT_MEMLOCK, in r) != 0)
            {
                return false;
            }

            return true;
        }

        public static unsafe int BPFPinObject(in byte filePath, int fd)
        {
            var attr = new BPFObjAttr(Unsafe.AsPointer(ref Unsafe.AsRef(in filePath)), fd, 0);
            return BPF(BPFCmd.BPF_OBJ_PIN, in attr);
        }

        public static int BPFCreatePerCPUArray(int valueSize, int maxEntries)
        {
            var attr = new BPFCreateMapAttr((int)BPFMapType.BPF_MAP_TYPE_PERCPU_ARRAY, sizeof(int), valueSize, maxEntries, 0);
            return BPF(BPFCmd.BPF_MAP_CREATE, in attr);
        }

        public static int BPFCreateStackTraceMap(int maxEntries)
        {
            var attr = new BPFCreateMapAttr((int)BPFMapType.BPF_MAP_TYPE_STACK_TRACE, sizeof(int), PERF_MAX_STACK_DEPTH * IntPtr.Size /* this is the max value size possible */, maxEntries, 0);
            return BPF(BPFCmd.BPF_MAP_CREATE, in attr);
        }

        public static int BPFCreatePerfEventArrayMap()
        {
            var attr = new BPFCreateMapAttr((int)BPFMapType.BPF_MAP_TYPE_PERF_EVENT_ARRAY, sizeof(int), sizeof(int), GetNumberOfConfiguredPrcoessors(), 0);
            return BPF(BPFCmd.BPF_MAP_CREATE, in attr);
        }

        public static int BPFUpdateElem<TK, TV>(int fd, in TK key, in TV value)
            where TK : unmanaged
            where TV : unmanaged
        {
            unsafe
            {
                fixed (TK* k = &key)
                {
                    fixed (TV* v = &value)
                    {
                        var attr = new BPFUpdateMapAttr(fd, k, v, 0);
                        return BPF(BPFCmd.BPF_MAP_UPDATE_ELEM, in attr);
                    }
                }
            }
        }

        public static void ConsoleWriteLine(in byte content)
        {
            if (PrintF(in content) != -1)
            {
                if (PrintF(in Strings.NewLine) != -1)
                {
                    return;
                }
            }
        }

        public static int GetFileSize(in byte filename)
        {
            StatX statx = default;
            var retVal = StatXSize(in statx, in filename);
            if (retVal < 0)
            {
                ConsoleWriteLine(in Strings.StatXError);
                return 0;
            }

            return (int)statx.Size;
        }

        public static int CloseFileDescriptor(int fd)
        {
            return Close(fd);
        }

        [DllImport("libc", EntryPoint = "get_nprocs_conf", SetLastError = true)]
        public static extern int GetNumberOfConfiguredPrcoessors();

        [DllImport("libc", EntryPoint = "getpagesize", SetLastError = true)]
        public static extern int GetMemoryPageSize();

        [DllImport("libc", EntryPoint = "open", SetLastError = true)]
        public static extern int OpenFileDescriptor(in byte filepath, int flags);

        [DllImport("libc", EntryPoint = "epoll_create1", SetLastError = true)]
        public static extern int EPollCreate(int flags);

        [DllImport("libc", EntryPoint = "epoll_ctl", SetLastError = true)]
        public static extern int EPollControl(int epfd, int op, int fd, in EPollEvent @event);

        [DllImport("libc", EntryPoint = "epoll_wait", SetLastError = true)]
        public static extern int EPollWait(int epfd, ref EPollEvent events, int maxevents, int timeout);

        [DllImport("libc", EntryPoint = "mmap", SetLastError = true)]
        public static extern IntPtr MemoryMap(IntPtr addr, long length, int prot, int flags, int fd, long offset);

        [DllImport("libc", EntryPoint = "munmap", SetLastError = true)]
        public static extern IntPtr MemoryUnmap(IntPtr addr, long length);

        private static int BPF<T>(BPFCmd cmd, in T attr)
            where T : unmanaged
        {
            var number = RuntimeInformation.OSArchitecture switch
            {
                Architecture.Arm64 => 280,
                Architecture.X64 => 321,
                _ => throw new NotSupportedException()
            };

            unsafe
            {
                fixed (T* t = &attr)
                {
                    return (int)SysCallBPF(number, cmd, t, Unsafe.SizeOf<T>());
                }
            }
        }

        private static int StatXSize(in StatX statx, in byte filename)
        {
            var number = RuntimeInformation.OSArchitecture switch
            {
                Architecture.Arm64 => 291,
                Architecture.X64 => 332,
                _ => throw new NotSupportedException()
            };

            const int AT_FDCWD = -100;
            const uint STATX_SIZE = 0x00000200U;
            const int AT_SYMLINK_NOFOLLOW = 0x100;

            return (int)SysCallStatX(number, AT_FDCWD, in filename, AT_SYMLINK_NOFOLLOW, STATX_SIZE, in statx);
        }

        [DllImport("libc", EntryPoint = "syscall", SetLastError = true)]
        private static extern unsafe long SysCallBPF(long number, BPFCmd cmd, void* attr, long size);

        /// <remarks>
        ///
        /// /proc/sys/kernel/sched_schedstats
        /// /sys/kernel/debug/tracing/kprobe_events
        /// /proc/sys/kernel/perf_event_paranoid 2
        /// /proc/sys/kernel/perf_event_max_sample_rate 100000
        /// /proc/sys/kernel/perf_event_max_stack 127
        /// /proc/sys/kernel/perf_event_mlock_kb 516
        /// </remarks>
        [DllImport("libc", EntryPoint = "syscall", SetLastError = true)]
        private static extern long SysCallPerfEventOpen(long number, in PerfEventAttr attr, long pid, long cpu, long groupfd, long flags);

        [DllImport("libc", EntryPoint = "syscall", SetLastError = true)]
        private static extern long SysCallStatX(long number, long dfd, in byte filename, ulong flags, uint mask, in StatX statx);

        [DllImport("libc", EntryPoint = "ioctl", SetLastError = true)]
        private static extern int IOControl(int fd, long value, long flags);

        [DllImport("libc", EntryPoint = "setrlimit", SetLastError = true)]
        private static extern int SetResourceLimit(int resource, in RLimit r);

        [DllImport("libc", EntryPoint = "printf", SetLastError = true)]
        private static extern int PrintF(in byte content);

        [DllImport("libc", EntryPoint = "close", SetLastError = true)]
        private static extern int Close(int fd);
    }
}
