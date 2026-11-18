#pragma once

/* nxdk provides the Win32 C ABI and Xbox kernel services. */
#include "port/include/platform.h"
#undef PLATFORM_WINDOWS
#define PLATFORM_XBOX 1
