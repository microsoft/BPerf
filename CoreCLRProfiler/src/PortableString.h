#pragma once
#include <string>

#ifndef _WINDOWS
using portable_wide_string = std::u16string;
using portable_wide_char = char16_t;
#define W(str) u##str
#else
using portable_wide_string = std::wstring;
using portable_wide_char = wchar_t;
#define W(str) L##str
#endif

#define ZEROSTRING W('0')
#define EMPTYSTRING W("")
#define UNKNOWNSTRING W("Unknown")