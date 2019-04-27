#define BPERFDLLIMPORT

#include "../lib/Program.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(ul_reason_for_call);
    UNREFERENCED_PARAMETER(lpReserved);
    return TRUE;
}

extern "C" __declspec(dllexport) int StartBPerf(int argc, PWSTR* argv, bool setCtrlCHandler, void(*fileMovedCallback)(const int length, const wchar_t *))
{
    return InvokeBPerfStart(argc, argv, setCtrlCHandler, fileMovedCallback);
}

extern "C" __declspec(dllexport) void StopBPerf()
{
    InvokeBPerfStop();
}