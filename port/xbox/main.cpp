#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>
#include <hal/debug.h>
#include <nxdk/mount.h>
#include <hal/video.h>
#include <pbkit/pbkit.h>
#include "gfx_pc.h"
#include "nv2a/gfx_nv2a.h"
#include "controllers.h"

static_assert(sizeof(Gfx) == 8 && sizeof(Vtx) == 16 && sizeof(Mtx) == 64);

extern "C" {
extern unsigned char mainThread[];
void osCreateThread(void *, int, void (*)(void *), void *, void *, int);
void osStartThread(void *);
void mainproc(void *);
void mixerInit(void);
int audioInit(void);
int getPlayerCount(void);
extern unsigned unknown2;
static unsigned frame;
}
static constexpr uintptr_t cart = 0x10000000;
static unsigned mappedBytes;
extern "C" void xboxMountResources(u8 *, u32, const u8 *, u32);
static constexpr unsigned romSize = 12 * 1024 * 1024;

static void serialByte(unsigned char c)
{
    for (unsigned i = 0; i < 50000; ++i) {
        unsigned char ready;
        __asm__ volatile("inb %1,%0" : "=a"(ready) : "Nd"((unsigned short)0x3fd));
        if (ready & 0x20) {
            __asm__ volatile("outb %0,%1" :: "a"(c), "Nd"((unsigned short)0x3f8));
            break;
        }
    }
}
extern "C" void serialPuts(const char *s) { while (*s) serialByte(*s++); }
void sysLogPrintf(enum LogLevel, const char *fmt, ...)
{
    char buf[384];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    serialPuts(buf);
    serialPuts("\r\n");
}
void sysFatalError(const char *fmt, ...)
{
    char buf[384];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    serialPuts(buf);
    debugPrint("%s\n", buf);
    for (;;) Sleep(1000);
}

const char *sysGetTokenString() { return ""; }
const char *sysResolvePath(const char *path)
{
    if (!strcmp(path, "$S/ge007.eep")) return "E:\\UDATA\\GE007\\ge007.eep";
    return "";
}
uintptr_t sysImageBase() { return 0x10000; }
void sysSleep(uint32_t us) { Sleep((us + 999) / 1000); }
uint64_t sysGetMicroseconds() { return KeQueryInterruptTime() / 10; }
extern "C" int romdataCartAddrValid(unsigned a, unsigned n)
{
    return a >= cart && a - cart <= mappedBytes && n <= mappedBytes - (a - cart);
}
extern "C" void videoEndFrame() { gfx_end_frame(); ++frame; }
extern "C" void videoStartFrame() { gfx_start_frame(); }
extern "C" void videoSyncSplitScreen() { gfx_set_split_screen(getPlayerCount() >= 2); }
extern "C" int videoQuitRequested() { return 0; }
extern "C" void videoResetTextureCache() { gfx_texture_cache_clear(); }
extern "C" void videoUpdateNativeResolution(int w, int h)
{
    gfx_current_native_viewport = {0, 0, (uint32_t)w, (uint32_t)h};
}
extern "C" u32 videoGetFrameCount() { return frame; }

int main()
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    serialPuts("GE007 game entry\r\n");
    void *dram = MmAllocateContiguousMemoryEx(0x800000, 0x1800000, 0x1ffffff, 4096, PAGE_READWRITE);
    if ((uintptr_t)dram != 0x81800000) sysFatalError("Cannot reserve game DRAM (%p)", dram);
    void *rom = (void *)cart;
    FILE *resources = fopen("D:\\xbox.bin", "rb");
    if (!resources || fseek(resources, 0, SEEK_END)) sysFatalError("Cannot open xbox.bin");
    long resourceBytes = ftell(resources);
    if (resourceBytes < 8 || resourceBytes > 8 * 1024 * 1024) sysFatalError("Invalid resource size");
    rewind(resources);
    mappedBytes = romSize + resourceBytes;
    SIZE_T size = mappedBytes;
    auto status = NtAllocateVirtualMemory(&rom, 0, &size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (status < 0 || (uintptr_t)rom != cart) sysFatalError("Cannot map cartridge (%x)", status);
    FILE *f = fopen("D:\\ge007.ntsc-final.z64", "rb");
    if (!f || fread(rom, 1, romSize, f) != romSize || memcmp((char *)rom + 0x20, "GOLDENEYE", 9))
        sysFatalError("Cannot read NTSC-U cartridge");
    fclose(f);
    u8 *pack = (u8 *)rom + romSize;
    if (fread(pack, 1, resourceBytes, resources) != (size_t)resourceBytes) sysFatalError("Cannot read xbox.bin");
    fclose(resources);
    xboxMountResources((u8 *)rom, romSize, pack, resourceBytes);
    unknown2 = __builtin_bswap32(unknown2);
    GfxWindowManagerAPI window{};
    window.init = [](const GfxWindowInitSettings *) {
        pb_extra_buffers(1);
        if (pb_init()) sysFatalError("pbkit init failed");
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
    if (!nxIsDriveMounted('E')) nxMountDrive('E', "\\Device\\Harddisk0\\Partition1");
    CreateDirectoryA("E:\\UDATA", nullptr);
    CreateDirectoryA("E:\\UDATA\\GE007", nullptr);
    audioInit();
    mixerInit();
    if (inputInit()) sysFatalError("Controller initialization failed");
    serialPuts("starting mainproc\r\n");
    portKernelInit();
    osCreateThread(mainThread, 3, mainproc, nullptr, nullptr, 10);
    osStartThread(mainThread);
    for (;;) Sleep(1000);
}
