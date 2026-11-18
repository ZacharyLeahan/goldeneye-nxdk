#pragma once
#include <stdint.h>

static inline bool xboxGameRam(uintptr_t address)
{
    return address >= 0x81800000u && address < 0x82000000u;
}

/* Most game GBI commands contain native pointers. The watch also emits
 * physical addresses into the same eight MiB allocation. Their high byte
 * looks like segment 1, but a model never spans eight MiB of that segment. */
static inline uintptr_t xboxGfxAddress(uintptr_t address)
{
    if (address >= 0x01800000u && address < 0x02000000u)
        return address | 0x80000000u;
    return address;
}
