#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <utility>
#include <windows.h>
#include <hal/debug.h>
#include <hal/video.h>
#include <pbkit/pbkit.h>
#include <zlib/zlib/zlib.h>
#include "gfx_pc.h"
#include "nv2a/gfx_nv2a.h"

static_assert(sizeof(Gfx) == 8 && sizeof(Vtx) == 16 && sizeof(Mtx) == 64);
extern "C" {
void setupRarewareLogoData(s32 address, s32 size);
Gfx *retrieve_display_rareware_logo(Gfx *dl);
s32 isGunBarrelInMode2(void);
extern Mtx *matrixBufferRareLogo0, *matrixBufferRareLogo1, *matrixBufferRareLogo2;
u8 *g_VtxBuffers[3];
}

static FILE *rom;
static constexpr unsigned cartBase = 0x10000000;
static constexpr unsigned romBytes = 12 * 1024 * 1024;
static constexpr size_t logoBytes = 0x67f0;
alignas(64) static uint8_t logo[(logoBytes + 63) & ~63];
static unsigned frame;

void sysLogPrintf(enum LogLevel, const char *format, ...)
{
    char message[384];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    DbgPrint("GE007: %s\n", message);
}

// This disc-only target has no writable desktop shader cache.
const char *sysResolvePath(const char *) { return ""; }

extern "C" void serialPuts(const char *message) { DbgPrint("GE007: %s", message); }

void sysFatalError(const char *format, ...)
{
    char message[384];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    DbgPrint("GE007: %s\n", message);
    debugPrint("GoldenEye 007\n%s\n", message);
    for (;;) Sleep(1000);
}

extern "C" void romCopy(void *target, void *source, u32 length)
{
    unsigned address = (uintptr_t)source;
    if (address < cartBase || address - cartBase > romBytes ||
        length > romBytes - (address - cartBase) ||
        fseek(rom, address - cartBase, SEEK_SET) || fread(target, 1, length, rom) != length)
        sysFatalError("ROM read failed at %08x (%u bytes)", address, length);
}

// Use host-order GBI words, as for assets/rarewarelogo.c on the desktop.
// Keep vertex color/normal bytes in their original order.
static void loadIntro()
{
    setupRarewareLogoData((uintptr_t)logo, sizeof(logo));
    if (crc32(0, logo, logoBytes) != 0x05f60803)
        sysFatalError("Unexpected intro segment. Use a retail US .z64 ROM.");
    // assets/rarewarelogo.c starts with six four-byte color arrays.
    for (unsigned i = 0x18; i < logoBytes;) {
        if (i >= 0x2b18 && i < 0x43e8) {
            for (unsigned field = 0; field < 12; field += 2)
                std::swap(logo[i + field], logo[i + field + 1]);
            i += sizeof(Vtx);
        } else {
            std::swap(logo[i], logo[i + 3]);
            std::swap(logo[i + 1], logo[i + 2]);
            i += 4;
        }
    }
}

extern "C" u32 osVirtualToPhysical(void *address) { return (uintptr_t)address; }
extern "C" s16 viGetX() { return 320; }
extern "C" s16 viGetY() { return 240; }
extern "C" u8 *viGetFrameBuf2() { return (u8 *)pb_back_buffer(); }
extern "C" u32 videoGetFrameCount() { return frame; }
extern "C" Gfx *optionsOverlayEmit() { return nullptr; }

int main()
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    rom = fopen("D:\\ge007.ntsc-final.z64", "rb");
    if (!rom) sysFatalError("Place ge007.ntsc-final.z64 beside default.xbe.");
    unsigned char header[64];
    if (fread(header, 1, sizeof(header), rom) != sizeof(header) ||
        memcmp(header, "\x80\x37\x12\x40", 4) || memcmp(header + 0x20, "GOLDENEYE", 9) ||
        memcmp(header + 0x3c, "GEE", 3) || fseek(rom, 0, SEEK_END) || ftell(rom) != romBytes)
        sysFatalError("Expected a 12 MiB NTSC-U GoldenEye ROM in .z64 byte order.");

    static Mtx matrices[3][2];
    matrixBufferRareLogo0 = matrices[0];
    matrixBufferRareLogo1 = matrices[1];
    matrixBufferRareLogo2 = matrices[2];
    loadIntro();

    GfxWindowManagerAPI window{};
    window.init = [](const GfxWindowInitSettings *) {
        pb_extra_buffers(1);
        int err = pb_init();
        if (err) sysFatalError("pbkit init failed (%d)", err);
        pb_show_front_screen();
    };
    window.get_dimensions = [](uint32_t *w, uint32_t *h, int32_t *x, int32_t *y) {
        *w = pb_back_buffer_width(); *h = pb_back_buffer_height(); *x = *y = 0;
    };
    window.handle_events = []() {};
    window.start_frame = []() { return true; };
    window.swap_buffers_begin = []() {};
    window.swap_buffers_end = []() {};
    GfxInitSettings settings{&window, &gfx_nv2a_api, {"GoldenEye 007", 640, 480}};
    gfx_init(&settings);
    gfx_current_native_viewport = {0, 0, 320, 240};
    gfx_current_native_aspect = 4.0f / 3.0f;

    static Vp viewport = {{{640, 480, 511, 0}, {640, 480, 511, 0}}};
    static Gfx commands[256];
    for (;;) {
        if (isGunBarrelInMode2()) loadIntro();
        Gfx *dl = commands;
        gSPViewport(dl++, &viewport);
        gDPSetScissor(dl++, G_SC_NON_INTERLACE, 0, 0, 320, 240);
        dl = retrieve_display_rareware_logo(dl);
        gSPEndDisplayList(dl++);
        gfx_start_frame();
        gfx_run(commands);
        gfx_end_frame();
        ++frame;
    }
}
