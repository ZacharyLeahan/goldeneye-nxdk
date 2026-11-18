#pragma once
#include "port/include/portaddr.h"
#include "port/xbox/shim/platform.h"
/* Finish the libultra types before the mutually recursive game headers. */
#include <ultra64.h>
#include "port/shim/bondtypes.h"
#include "port/shim/bondconstants.h"
#include "include/math.h"
#include <float.h>
/* Native Xbox pointers survive the N64 physical-address round trip. */
#undef OS_K0_TO_PHYSICAL
#define OS_K0_TO_PHYSICAL(x) ((u32)(uintptr_t)(x))
#undef PHYS_TO_K0
#define PHYS_TO_K0(x) ((u32)(uintptr_t)(x))
