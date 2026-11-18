#pragma once

/* The i386 cart command and both native word pointers fit in eight bytes. */
#include "include/PR/gbi.h"
#include <PR/mbi.h>

/* The game also inspects individual GBI fields in host byte order. Keep the
 * command stride at two i386 words, including for ROM-backed room lists. */
typedef union {
    Gwords words;
    /* Immediate opcodes including G_ENDDL are negative in the cartridge GBI. */
    struct { unsigned par : 24; signed int cmd : 8; uintptr_t addr; } dma;
    struct { unsigned w0; struct { unsigned char v[3], flag; } tri; } tri;
    struct { unsigned tl : 12, sl : 12, cmd : 8, th : 12, sh : 12, tile : 3, pad : 5; } loadtile;
    long long alignment;
} Gfx_le;
#define Gfx Gfx_le
