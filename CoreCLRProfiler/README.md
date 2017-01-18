# BPerf CoreCLR Profiler

This is the cross-platform managed-code user-mode profiler component of the BPerf Production Profiling system. It implements the [CLR Profiling APIs](https://msdn.microsoft.com/en-us/library/ms404386(v=vs.110).aspx) and collects data about the managed application and logs it to [SQLite](https://www.sqlite.org).

It is capable of profiling .NET 4.5 applications and .NET Core (a.k.a CoreCLR) applications on all supported platforms (Linux, macOS, Windows)

Consuming BPerf
---------------

* Acquire BPerfCoreCLRProfiler(.dll|so) via our [NuGet Package](https://nuget.org/packages/BPerf) built for Windows/Ubuntu 14.04/16.04/16.10 or build from source.
* Perform environment variable setup for your platform (documented below)
* Setup additional BPerf options:

  1. ``BPERF_LOG_PATH`` - Output location for the SQLite database (ensure your process has write permissions!)

  2. ``BPERF_OVERWRITE_LOGS`` -- Whether the existing logs should be overwritten

  3. ``BPERF_MONITOR_ALLOCATIONS`` - Monitors every object allocation and GC. _Caution_: Application most likely usable, but noticable impact on performance.

  4. ``BPERF_MONITOR_ENTERLEAVE`` - Monitors every function entry/exit. _Caution_: Application most likely usable, but noticable impact on performance.

5. View data from your SQLite database

Building on Windows
-------------------

### Environment

All instructions must be run on the ``VS 2015 x64 Native Tools Command Prompt``.

### Build
On the ``VS 2015 x64 Native Tools Command Prompt``.

```batch
msbuild BPerfCoreCLRProfiler.vcxproj
```

* or open using Visual Studio 2015+ (from the aforementioned command prompt)

### Setup

Profiling for .NET (and .NET Core) applications on Windows requires the following environment variables set to indicate the ``Profiler CLSID`` (a unique GUID), a path to the unmanaged dll and whether profiling is enabled or not.

The following example setup is configured with the correct ``Profiler CLSID``, and an imaginary file location for the BPerf Profiler DLL.

```batch
SET COR_PROFILER={D46E5565-D624-4C4D-89CC-7A82887D3626}
SET COR_ENABLE_PROFILING=1
SET COR_PROFILER_PATH=C:\filePath\to\BPerfCoreCLRProfiler.dll
SET BPERF_LOG_PATH=C:\BPerfLogs.db
YourProgram.exe OR (corerun YourProgram.exe for CoreCLR applications)
```

Building on Linux, macOS
------------------------

BPerf requires a CoreCLR repository built so that it can link against certain components in addition to consuming header files and other native artifacts.

### Prerequisites

* CoreCLR Repository (built from source)
* Clang 3.5+

### Environment

``build.sh`` expects the following environment variables to be setup; default values are shown below.

```bash
export CORECLR_PATH=~/coreclr # default
export BuildOS=Linux # Linux(default), MacOSX
export BuildArch=x64 # x64 (default)
export BuildType=Debug # Debug(default), Release
export Output=CorProfiler.so # default
```

``CORECLR_PATH`` is the path to your cloned and successfully built CoreCLR repository.

``BuildOS``, ``BuildArch`` and ``BuildType`` must match how you built the CoreCLR repository, so the header files and other artifacts needed for compilation are locatable by convention.

### Build

```bash
git clone http://github.com/Microsoft/BPerf
cd CoreCLRProfiler
./build.sh
```

This produces a ``BPerfCoreCLRProfiler.so`` file you can then use in the setup when executing your .NET Core application.

### Setup

Profiling for .NET Core applications on Linux and macOS requires the following environment variables set to indicate the ``Profiler CLSID`` (a unique GUID), a path to the unmanaged shared library and whether profiling is enabled or not.

The following example setup is configured with the correct ``Profiler CLSID``, and an imaginary file location for the BPerf Profiler Shared Library.

```bash
export CORECLR_PROFILER={D46E5565-D624-4C4D-89CC-7A82887D3626}
export CORECLR_ENABLE_PROFILING=1
export CORECLR_PROFILER_PATH=/filePath/to/CorProfiler.so
export BPERF_LOG_PATH=/filePath/to/BPerfLog.db
./corerun YourProgram.dll
```
