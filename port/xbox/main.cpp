#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>
#include <hal/debug.h>
#include <nxdk/mount.h>
#include <hal/video.h>
#include <hal/xbox.h>
#include <pbkit/pbkit.h>
#include "gfx_pc.h"
#include "nv2a/gfx_nv2a.h"
#include "nv2a/render_profile.h"
#include "controllers.h"
#include "port/lan/lan.h"
#include "port/lan/frame_trace.h"
#include <cstdlib>

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
extern "C" unsigned xboxProfileGame(int *stage, int *shots, unsigned *throws);
bool xboxProfileEnabled;
static unsigned xboxRenderSample;
static bool recoverStalledTest;
static volatile LONG lastProgress;

extern "C" void xboxLoadCheckpoint(int stage, const char *step)
{
    if (!recoverStalledTest) return;
    InterlockedExchange(&lastProgress, (LONG)GetTickCount());
    FILE *out = fopen("D:\\ge-load.log", "a");
    if (out) {
        fprintf(out, "%lu stage %d entering %s\n", (unsigned long)GetTickCount(), stage, step);
        fclose(out);
    }
}

static int compareTimes(const void *a, const void *b)
{
    auto x = *static_cast<const unsigned *>(a);
    auto y = *static_cast<const unsigned *>(b);
    return (x > y) - (x < y);
}

static void profileMultiplayerFrame()
{
    static int stage = -1, startShots;
    static unsigned generation, startThrows, startVBlank;
    static XboxRenderProfile initial;
    static uint64_t startClock;
    static unsigned intervals[1024], count, minFree, nextMemoryCheck;
    static uint64_t warmUntil, start, previous;
    int current, shots;
    unsigned throws;
    int seats = getPlayerCount();
    if (seats < 2) { stage = -1; return; }
    unsigned currentGeneration = xboxProfileGame(&current, &shots, &throws);
    if (ftRecording()) {
        ftRecord(FT_SHOTS, ftRenderId, (unsigned)shots);
        previous = 0;
        return;
    }
    uint64_t now = sysGetMicroseconds();
    if (stage != current || generation != currentGeneration) {
        stage = current;
        generation = currentGeneration;
        warmUntil = now + 1000000;
        previous = 0;
    }
    if (now < warmUntil) return;
    if (!previous) {
        start = previous = now;
        initial = xboxRenderProfile;
        startClock = xboxRenderClock();
        startShots = shots;
        startThrows = throws;
        startVBlank = pb_get_vbl_counter();
        count = nextMemoryCheck = 0;
        minFree = ~0u;
        return;
    }
    intervals[count++] = (unsigned)(now - previous);
    previous = now;
    if (now - start >= nextMemoryCheck) {
        MM_STATISTICS mem = {};
        mem.Length = sizeof(mem);
        if (MmQueryStatistics(&mem) >= 0 && mem.AvailablePages < minFree)
            minFree = mem.AvailablePages;
        nextMemoryCheck += 1000000;
    }
    if (now - start < 7000000 && count < sizeof(intervals)/sizeof(intervals[0])) return;
    uint64_t endClock = xboxRenderClock();
    unsigned elapsedVBlanks = (unsigned)pb_get_vbl_counter() - startVBlank;
    qsort(intervals, count, sizeof(intervals[0]), compareTimes);
    FILE *out = fopen("D:\\ge-profile.csv", "a");
    if (out) {
        fprintf(out, "%d,%s,%u,%u,%u,%u,%d,%d,%d,%u,%u,%u\n", stage, lanActive() ? "lan" : "split",
                count, (unsigned)(now-start), intervals[(95*count+99)/100-1], intervals[count-1],
                minFree == ~0u ? -1 : (int)(minFree*4), shots-startShots, seats, throws-startThrows,
                intervals[(99*count+99)/100-1], elapsedVBlanks);
        fclose(out);
    }
    if (FILE *render = fopen("D:\\ge-render.csv", "a")) {
        const auto &p = xboxRenderProfile;
        double us = (double)(now - start) / ((endClock - startClock) * (double)count);
        fprintf(render, "%d,%s,%u,%u,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                stage, lanActive() ? "lan" : "split", count, (unsigned)(now-start),
                shots-startShots, (unsigned)((p.render-initial.render)*us),
                (unsigned)((p.submit-initial.submit)*us), (unsigned)((p.acquire-initial.acquire)*us),
                (unsigned)((p.drain-initial.drain)*us), (unsigned)((p.present-initial.present)*us),
                (unsigned)((p.uploads-initial.uploads)*us), (unsigned)(p.draws-initial.draws),
                (unsigned)(p.triangles-initial.triangles), (unsigned)(p.bytes-initial.bytes), elapsedVBlanks, xboxRenderSample);
        fclose(render);
    }
    ++xboxRenderSample;
    previous = 0;  // Exclude the disk write from the next sample.
}

extern "C" void videoEndFrame()
{
    gfx_end_frame();
    ++frame;
    if (recoverStalledTest) InterlockedExchange(&lastProgress, (LONG)GetTickCount());
    if (xboxProfileEnabled) profileMultiplayerFrame();
}
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
    if (FILE *marker = fopen("D:\\ge-recover.on", "rb")) {
        recoverStalledTest = true;
        fclose(marker);
    }
    if (FILE *marker = fopen("D:\\ge-profile.on", "rb")) {
        xboxProfileEnabled = true;
        fclose(marker);
    }
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
    if (FILE *marker = fopen("D:\\ge-local-four.on", "rb")) {
        fclose(marker);
        inputStartLocalFourTest();
        serialPuts("local four-player emulator fixture enabled\r\n");
    }
    // A private benchmark can join directly, including on a controllerless rig.
    DWORD testStarted = 0;
    char hostAddress[32] = {};
    unsigned hostPort = 0;
    bool firingTest = false;
    bool oneShotJoin = false;
    if (FILE *fixture = fopen("D:\\ge-test-peer.txt", "rb")) {
        if (fscanf(fixture, "%31s %u", hostAddress, &hostPort) == 2) {
            FILE *marker = fopen("D:\\ge-trace.on", "rb");
            if (marker) { firingTest = true; fclose(marker); }
            else oneShotJoin = true;
        }
        fclose(fixture);
    } else if (FILE *join = fopen("D:\\ge-emu-join.on", "rb")) {
        fclose(join);
        strcpy(hostAddress, "10.0.2.2");
        hostPort = 27008;
    }
    if (hostPort > 0 && hostPort <= 65535) {
        if (lanJoinOnPort(hostAddress, (uint16_t)hostPort)) {
            DWORD started = GetTickCount();
            while (!lanConnected() && (DWORD)(GetTickCount() - started) < 30000) {
                lanPoll();
                Sleep(10);
            }
            if (lanConnected() && oneShotJoin) remove("D:\\ge-test-peer.txt");
            if (!lanConnected()) lanClose();
            else if (firingTest && !strcmp(lanArenaName(lanArena()), "Egypt")) {
                inputStartEgyptTest();
                testStarted = GetTickCount();
            }
        }
    }
    serialPuts("starting mainproc\r\n");
    portKernelInit();
    osCreateThread(mainThread, 3, mainproc, nullptr, nullptr, 10);
    osStartThread(mainThread);
    if (recoverStalledTest) InterlockedExchange(&lastProgress, (LONG)GetTickCount());
    for (;;) {
        Sleep(1000);
        if (testStarted && (DWORD)(GetTickCount() - testStarted) >= 75000u) XLaunchXBE(NULL);
        if (recoverStalledTest && (DWORD)(GetTickCount() - (DWORD)lastProgress) > 30000u) {
            xboxLoadCheckpoint(-1, "return to dashboard after 30 seconds without a frame or load progress");
            XLaunchXBE(NULL);
        }
    }
}
