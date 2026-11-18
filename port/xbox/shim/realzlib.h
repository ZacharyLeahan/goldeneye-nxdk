#pragma once
#include <stdlib.h>
#include <zlib/zlib/zlib.h>

/* nxdk uses Z_SOLO. Its inflater requires explicit allocator callbacks. */
static inline voidpf xboxInflateAlloc(voidpf opaque, uInt n, uInt size)
{
    return calloc(n, size);
}

static inline void xboxInflateFree(voidpf opaque, voidpf p)
{
    free(p);
}

static inline int xboxInflateInit(z_streamp stream, int windowBits)
{
    stream->zalloc = xboxInflateAlloc;
    stream->zfree = xboxInflateFree;
    return inflateInit2_(stream, windowBits, ZLIB_VERSION, sizeof(*stream));
}

#undef inflateInit2
#define inflateInit2(stream, windowBits) xboxInflateInit(stream, windowBits)
