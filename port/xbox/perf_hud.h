#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* An epoch is nonzero while the render worker requests scanout samples. */
extern unsigned xboxHudScanEpoch;
void xboxHudScanout(unsigned epoch, unsigned surface);

#ifdef __cplusplus
}
extern bool xboxHudConfigured;
void xboxHudInit();
bool xboxHudBeginFrame();
void xboxHudDraw(int screenWidth, int screenHeight);
void xboxHudFrameCost(uint64_t prepTicks, uint64_t waitTicks);
#endif
