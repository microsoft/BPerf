namespace Microsoft.BPerf
{
    using System;
    using System.Runtime.InteropServices;

    public delegate void FileMoveNotifyCallback(string fileName);

    internal delegate void FileMoveCallbackInternal(int length, IntPtr fileName);

    public static class DataCollector
    {
        private static FileMoveNotifyCallback fileMoveNotifyCallback;

        public static void Start(string fullDirectoryPath, int rollOverTimeSeconds, FileMoveNotifyCallback callback)
        {
            if (string.IsNullOrEmpty(fullDirectoryPath))
            {
                throw new ArgumentNullException(nameof(fullDirectoryPath));
            }

            if (callback == null)
            {
                throw new ArgumentNullException(nameof(callback));
            }

            string[] argvs = { "unused", "1", rollOverTimeSeconds.ToString(), fullDirectoryPath };
            IntPtr[] arr = new IntPtr[argvs.Length];

            for (var i = 0; i < argvs.Length; ++i)
            {
                arr[i] = Marshal.StringToHGlobalUni(argvs[i]);
            }

            FileMoveCallbackInternal c = FileMoveInternal;

            fileMoveNotifyCallback = callback;
            StartBPerf(argvs.Length, arr, false, Marshal.GetFunctionPointerForDelegate(c));

            GC.KeepAlive(c);
        }

        public static void Stop()
        {
            StopBPerf();
        }

        private static void FileMoveInternal(int length, IntPtr fileNamePtr)
        {
            string fileName = Marshal.PtrToStringUni(fileNamePtr, length);
            fileMoveNotifyCallback(fileName);
        }

        [DllImport(@"BPerfCPUSamplesCollectorLibrary.dll")]
        private static extern int StartBPerf(int argc, IntPtr[] argv, bool setCtrlCHandler, IntPtr fileMoveCallback);

        [DllImport(@"BPerfCPUSamplesCollectorLibrary.dll")]
        private static extern void StopBPerf();
    }
}