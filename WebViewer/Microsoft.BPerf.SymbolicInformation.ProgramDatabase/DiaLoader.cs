// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.SymbolicInformation.ProgramDatabase
{
    using System;
    using System.Runtime.InteropServices;
    using Dia2Lib;

    internal static class DiaLoader
    {
        private static readonly Guid ClassFactoryGuid = new Guid("00000001-0000-0000-C000-000000000046");

        private static readonly Guid IDiaDataSourceGuid = new Guid("65A23C15-BAB3-45DA-8639-F06DE86B9EA8");

        private static bool loadedNativeDll;

        [ComImport]
        [ComVisible(false)]
        [Guid("00000001-0000-0000-C000-000000000046")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        private interface IClassFactory
        {
            void CreateInstance([MarshalAs(UnmanagedType.Interface)] object aggregator, ref Guid refiid, [MarshalAs(UnmanagedType.Interface)] out object createdObject);

            void LockServer(bool incrementRefCount);
        }

        public static IDiaDataSource3 GetDiaSourceObject()
        {
            if (!loadedNativeDll)
            {
                // TODO: Remove System.IO.*
                LoadLibrary(System.IO.Path.Combine(System.IO.Path.GetDirectoryName(typeof(DiaLoader).Assembly.Location), IntPtr.Size == 8 ? @"amd64\msdia140.dll" : @"x86\msdia140.dll"));
                loadedNativeDll = true;
            }

            var diaSourceClassGuid = new Guid("{e6756135-1e65-4d17-8576-610761398c3c}");
            var comClassFactory = (IClassFactory)DllGetClassObject(diaSourceClassGuid, ClassFactoryGuid);

            Guid dataDataSourceGuid = IDiaDataSourceGuid;
            comClassFactory.CreateInstance(null, ref dataDataSourceGuid, out object comObject);
            return comObject as IDiaDataSource3;
        }

        [return: MarshalAs(UnmanagedType.Interface)]
        [DllImport("msdia140.dll", CharSet = CharSet.Unicode, ExactSpelling = true, PreserveSig = false)]
        private static extern object DllGetClassObject([In, MarshalAs(UnmanagedType.LPStruct)] Guid rclsid, [In, MarshalAs(UnmanagedType.LPStruct)] Guid riid);

        [DllImport("kernel32.dll")]
        private static extern IntPtr LoadLibrary(string filePath);
    }
}
