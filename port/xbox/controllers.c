#include <SDL.h>
#include <PR/os.h>
#include "controllers.h"
#include "finals_controls.h"

static FinalsPad finalsPads[MAXCONTROLLERS];
static SDL_SpinLock finalsLock;

void xboxReadFinalsPad(int slot, FinalsPad *pad)
{
    SDL_AtomicLock(&finalsLock);
    *pad = finalsPads[slot];
    SDL_AtomicUnlock(&finalsLock);
}

static struct {
    SDL_Joystick *device;
    u16 held;
    s8 x, y;
} pads[MAXCONTROLLERS];

int inputInit(void)
{
    return SDL_InitSubSystem(SDL_INIT_JOYSTICK);
}

int inputConnectedMask(void)
{
    int mask = 0;
    for (int i = 0; i < MAXCONTROLLERS; ++i)
        if (pads[i].device) mask |= 1 << i;
    return mask;
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
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_JOYDEVICEADDED) {
            int index = event.jdevice.which;
            /* nxdk SDL assigns the physical Xbox port number, starting at 1. */
            int slot = SDL_JoystickGetDevicePlayerIndex(index) - 1;
            if (slot >= 0 && slot < MAXCONTROLLERS && !pads[slot].device)
                pads[slot].device = SDL_JoystickOpen(index);
        } else if (event.type == SDL_JOYDEVICEREMOVED) {
            for (int i = 0; i < MAXCONTROLLERS; ++i) {
                if (pads[i].device && SDL_JoystickInstanceID(pads[i].device) == event.jdevice.which) {
                    SDL_JoystickClose(pads[i].device);
                    SDL_memset(&pads[i], 0, sizeof(pads[i]));
                }
            }
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
        SDL_AtomicLock(&finalsLock);
        finalsPads[i] = raw;
        SDL_AtomicUnlock(&finalsLock);
        if (!joy) continue;
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
    if ((unsigned)slot >= MAXCONTROLLERS) { *x = *y = 0; return 0; }
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
