#include "../lib/Program.h"

int wmain(int argc, PWSTR *argv)
{
    return InvokeBPerfStart(argc, argv, true, nullptr);
}