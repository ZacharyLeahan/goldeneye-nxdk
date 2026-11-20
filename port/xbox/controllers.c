#include <SDL.h>
#include <stdio.h>
#include <hal/xbox.h>
#include <PR/os.h>
#include "controllers.h"
#include "port/lan/lan.h"
#include "port/lan/frame_trace.h"
#include "finals_controls.h"

static FinalsPad finalsPads[MAXCONTROLLERS];
static SDL_SpinLock finalsLock;
static int traceVolley;
static SDL_atomic_t volleyStart;

/* Reproduce a simultaneous local death in xemu without host controllers.
 * The private fixture begins when its first fully warmed trace is recording. */
static FinalsPad volleyPad(int slot)
{
    FinalsPad pad = {0};
    if (!ftRecording()) return pad;
    Uint32 now = SDL_GetTicks();
    Uint32 start = SDL_AtomicGet(&volleyStart);
    if (!start) {
        SDL_AtomicCAS(&volleyStart, 0, (int)now);
        start = SDL_AtomicGet(&volleyStart);
    }
    Uint32 elapsed = start > now ? 0 : now - start;
    if (elapsed >= 500 && elapsed < 1300) {
        pad.buttons = FIN_WHEEL;
        pad.rx = 32767;
    } else if (elapsed >= 1300 && elapsed < 1500) {
        pad.rx = 32767;
    } else if (elapsed >= 2200 && elapsed < 3500) {
        pad.ry = 32767;
    } else if (slot == 0 && ((elapsed >= 3700 && elapsed < 4500) ||
                               (elapsed >= 5600 && elapsed < 6400))) {
        pad.buttons = FIN_FIRE;
    }
    return pad;
}

void xboxReadFinalsPad(int slot, FinalsPad *pad)
{
    if (traceVolley) { *pad = volleyPad(slot); return; }
    SDL_AtomicLock(&finalsLock);
    *pad = lanActive() && slot != 0 ? (FinalsPad){0} : finalsPads[slot];
    SDL_AtomicUnlock(&finalsLock);
}

static struct {
    SDL_Joystick *device;
    u16 held;
    s8 x, y;
    Uint32 resetStarted;
    int resetHeld;
} pads[MAXCONTROLLERS];

static int egyptTest;
static int localFourTest;
static int closeSpawnTest;
static Uint32 egyptStart;

int xboxEgyptTestActive(void) { return egyptTest; }
int xboxLocalFourTestActive(void) { return localFourTest; }

void inputStartEgyptTest(void)
{
    egyptStart = SDL_GetTicks();
    egyptTest = 1;
}

void inputStartLocalFourTest(int arena)
{
    localFourTest = arena;
    FILE *marker = fopen("D:\\ge-close.on", "rb");
    if (marker) {
        fclose(marker);
        closeSpawnTest = 1;
    }
    marker = fopen("D:\\ge-local-volley.on", "rb");
    if (marker) { fclose(marker); traceVolley = 1; }
}

int xboxCloseSpawnTest(void) { return closeSpawnTest; }

static int egyptTrigger(void)
{
    if (!egyptTest) return 0;
    Uint32 elapsed = SDL_GetTicks() - egyptStart;
    return elapsed >= 4000 && elapsed < 41000 && (elapsed - 4000) % 650 < 130;
}

int inputInit(void)
{
    return SDL_InitSubSystem(SDL_INIT_JOYSTICK);
}

int inputConnectedMask(void)
{
    static int lastLoggedMask = -1;
    int mask = 0;
    for (int i = 0; i < MAXCONTROLLERS; ++i)
        if (pads[i].device) mask |= 1 << i;
    if (localFourTest && mask != lastLoggedMask) {
        FILE *log = fopen("D:\\ge-pads.log", "a");
        if (log) {
            fprintf(log, "%u arena %d physical mask %x\n", SDL_GetTicks(), localFourTest, mask);
            fclose(log);
        }
        lastLoggedMask = mask;
    }
    if (lanActive()) return mask | 3;
    return localFourTest ? (mask | 15) : mask;
}

static int stick(int axis)
{
    const int dead = 5000;
    if (axis > dead) return (axis - dead) * 80 / (32767 - dead);
    if (axis < -dead) return (axis + dead) * 80 / (32768 - dead);
    return 0;
}

void inputUpdate(void)
{
    static Uint32 lastScan;
    int rescan = 0;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_JOYDEVICEADDED) {
            rescan = 1;
        } else if (event.type == SDL_JOYDEVICEREMOVED) {
            for (int i = 0; i < MAXCONTROLLERS; ++i) {
                if (pads[i].device && SDL_JoystickInstanceID(pads[i].device) == event.jdevice.which) {
                    SDL_JoystickClose(pads[i].device);
                    SDL_memset(&pads[i], 0, sizeof(pads[i]));
                }
            }
        }
    }
    /* The nxdk device list may reorder between queuing and reading add
     * events. Enumerate its current indexes and keep handles by physical port. */
    Uint32 now = SDL_GetTicks();
    if (rescan || now - lastScan >= 500) {
        lastScan = now;
        int count = SDL_NumJoysticks();
        for (int index = 0; index < count; ++index) {
            int slot = SDL_JoystickGetDevicePlayerIndex(index) - 1;
            if (slot >= 0 && slot < MAXCONTROLLERS && !pads[slot].device)
                pads[slot].device = SDL_JoystickOpen(index);
        }
    }
    for (int i = 0; i < MAXCONTROLLERS; ++i) {
        SDL_Joystick *joy = pads[i].device;
        FinalsPad raw = {0};
        if (joy) {
            raw.lx = SDL_JoystickGetAxis(joy, 0);
            raw.ly = SDL_JoystickGetAxis(joy, 1);
            raw.rx = SDL_JoystickGetAxis(joy, 3);
            raw.ry = SDL_JoystickGetAxis(joy, 4);
            for (int b = 0; b <= 4; ++b)
                if (SDL_JoystickGetButton(joy, b)) raw.buttons |= 1u << b;
            if (SDL_JoystickGetButton(joy, 8)) raw.buttons |= FIN_SPRINT;
            if (SDL_JoystickGetButton(joy, 9)) raw.buttons |= FIN_SLAP;
            if (SDL_JoystickGetAxis(joy, 2) > 4000) raw.buttons |= FIN_AIM;
            if (SDL_JoystickGetAxis(joy, 5) > 4000) raw.buttons |= FIN_FIRE;
        }
        /* A trace must replay the same camera even with a live or drifting pad.
         * The physical reset chord below remains available during capture. */
        if (i == 0 && egyptTest)
            raw = (FinalsPad){.buttons = egyptTrigger() ? FIN_FIRE : 0};
        SDL_AtomicLock(&finalsLock);
        finalsPads[i] = raw;
        SDL_AtomicUnlock(&finalsLock);
        if (!joy) continue;
        /* nxdk SDL maps Black to button 5 and Back to button 6. Track each
         * physical pad separately so two players cannot complete a chord. */
        int reset = SDL_JoystickGetButton(joy, 5) && SDL_JoystickGetButton(joy, 6)
                 && SDL_JoystickGetAxis(joy, 2) > 16000 && SDL_JoystickGetAxis(joy, 5) > 16000;
        if (reset) {
            Uint32 now = SDL_GetTicks();
            if (!pads[i].resetHeld) pads[i].resetStarted = now;
            if (now - pads[i].resetStarted >= 1500) XLaunchXBE(NULL);
        }
        pads[i].resetHeld = reset;
        u16 buttons = 0;
        int hat = SDL_JoystickGetHat(joy, 0);
        if (hat & SDL_HAT_UP) buttons |= U_JPAD;
        if (hat & SDL_HAT_DOWN) buttons |= D_JPAD;
        if (hat & SDL_HAT_LEFT) buttons |= L_JPAD;
        if (hat & SDL_HAT_RIGHT) buttons |= R_JPAD;
        if (SDL_JoystickGetButton(joy, 0)) buttons |= A_BUTTON;
        if (SDL_JoystickGetButton(joy, 1)) buttons |= B_BUTTON;
        if (SDL_JoystickGetButton(joy, 7)) buttons |= START_BUTTON;
        if (SDL_JoystickGetAxis(joy, 5) > 0) buttons |= Z_TRIG;
        if (SDL_JoystickGetAxis(joy, 2) > 0) buttons |= R_TRIG;
        int rx = SDL_JoystickGetAxis(joy, 3), ry = SDL_JoystickGetAxis(joy, 4);
        if (rx > 10000) buttons |= R_CBUTTONS;
        if (rx < -10000) buttons |= L_CBUTTONS;
        if (ry > 10000) buttons |= D_CBUTTONS;
        if (ry < -10000) buttons |= U_CBUTTONS;
        pads[i].held = buttons;
        pads[i].x = stick(SDL_JoystickGetAxis(joy, 0));
        pads[i].y = -stick(SDL_JoystickGetAxis(joy, 1));
    }
}

unsigned inputComputePad(int slot, signed char *x, signed char *y)
{
    if ((unsigned)slot >= MAXCONTROLLERS || (lanActive() && slot != 0)) { *x = *y = 0; return 0; }
    if (traceVolley) {
        *x = *y = 0;
        return volleyPad(slot).buttons & FIN_FIRE ? Z_TRIG : 0;
    }
    if (slot == 0 && egyptTest) {
        *x = *y = 0;
        return egyptTrigger() ? Z_TRIG : 0;
    }
    *x = pads[slot].x;
    *y = pads[slot].y;
    return pads[slot].held;
}

int inputRumbleSupported(int slot)
{
    return (unsigned)slot < MAXCONTROLLERS && pads[slot].device != NULL;
}

void inputRumble(int slot, float strength, float seconds)
{
    if (!inputRumbleSupported(slot)) return;
    if (strength < 0) strength = 0;
    if (strength > 1) strength = 1;
    Uint16 amplitude = (Uint16)(strength * 65535);
    SDL_JoystickRumble(pads[slot].device, amplitude, amplitude, (Uint32)(seconds * 1000));
}
