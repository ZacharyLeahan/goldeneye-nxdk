#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { FIN_A=1, FIN_B=2, FIN_X=4, FIN_Y=8, FIN_WHEEL=16, FIN_SPRINT=32, FIN_SLAP=64,
       FIN_FIRE=128, FIN_AIM=256 };

typedef struct {
    int16_t lx, ly, rx, ry;
    unsigned buttons;
} FinalsPad;

typedef struct {
    int items[32], count, current, grenades;
    float left, top, width, height;
} FinalsInventory;

void xboxReadFinalsPad(int slot, FinalsPad *pad);
int xboxFinalsWheel(int slot, int active, const FinalsPad *pad, const FinalsInventory *items,
                    int *equip);
void xboxFinalsLook(int slot, const FinalsPad *pad, float fov, float dt, float *yaw, float *pitch);
void xboxFinalsReset(int slot);
void xboxFinalsOverlay(uint32_t *pixels, int pitch, int width, int height);

#ifdef __cplusplus
}
#endif
