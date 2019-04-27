#pragma once
#include <Windows.h>

constexpr GUID ImageLoadGuid = { 0x2cb15d1d, 0x5fc1, 0x11d2, {0xab, 0xe1, 0x00, 0xa0, 0xc9, 0x11, 0xf5, 0x18} };
constexpr GUID ClrProviderGuid = { 0xE13C0D23, 0xCCBC, 0x4E12, {0x93, 0x1B, 0xD9, 0xCC, 0x2E, 0xEE, 0x27, 0xE4} };
constexpr GUID KernelProcessGuid = { 0x22fb2cd6, 0x0e7b, 0x422b, { 0xa0, 0xc7, 0x2f, 0xad, 0x1f, 0xd0, 0xe7, 0x16 } };

constexpr USHORT MethodLoadEvent = 143;
constexpr USHORT MethodUnloadEvent = 144;
constexpr USHORT ModuleLoadEvent = 152;
constexpr USHORT ModuleUnloadEvent = 153;
constexpr USHORT ILToNativeMapEvent = 190;

struct SafeFileHandle
{
    SafeFileHandle(HANDLE handle)
    {
        this->h = handle;
    }

    SafeFileHandle(SafeFileHandle &&movedFrom) noexcept
    {
        this->h = movedFrom.h;
        movedFrom.h = INVALID_HANDLE_VALUE;
    }

    SafeFileHandle(const SafeFileHandle& that)
    {
        this->h = that.h;
    }

    SafeFileHandle& operator=(const SafeFileHandle& that)
    {
        this->h = that.h;
        return *this;
    }

    ~SafeFileHandle()
    {
        if (this->h != INVALID_HANDLE_VALUE)
        {
            CloseHandle(this->h);
        }
    }

    const HANDLE Get() const
    {
        return this->h;
    }

private:
    HANDLE h;
};

int InvokeBPerfStart(const int argc, PWSTR *argv, bool setCtrlCHandler, void (*fileMovedCallback)(const int length, const wchar_t *));
void InvokeBPerfStop();