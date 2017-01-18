#pragma once

#ifndef _WINDOWS
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
