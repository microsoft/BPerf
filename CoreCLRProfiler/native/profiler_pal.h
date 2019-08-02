#pragma once

#ifndef _WINDOWS

#define _ASSERTE(x)
#define PAL_STDCPP_COMPAT
#define UNICODE
#define FEATURE_PAL
#define PLATFORM_UNIX

#if INTPTR_MAX == INT64_MAX
#define BIT64
#endif

#include <cstdlib>
#include "pal_mstypes.h"
#include "pal.h"
#include "ntimage.h"
#include "corhdr.h"
#define CoTaskMemAlloc(cb) malloc(cb)
#define CoTaskMemFree(cb) free(cb)

#else

#include <Windows.h>
#define PAL_wcslen wcslen

#endif

#include "PortableString.h"
