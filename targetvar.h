// targetver.h
#pragma once

// Windows 10 ve sonrası için hedef
#include <winsdkver.h>

#ifndef WINVER
#define WINVER 0x0A00          // Windows 10
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00    // Windows 10
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0A00  // Windows 10
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0A00       // IE 10.0
#endif