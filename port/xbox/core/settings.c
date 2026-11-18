#include <stdint.h>
#include "romdata.h"

int portSkipIntro = 1;
int portAllUnlocked, portNoHitFlash, portCrosshairHide;
float portScreenShakeScale = 1.0f;
float portFovYScaleFactor(void) { return 1.0f; }
float portScaleFovY(float fovy, int title) { return fovy; }
float portRoomPoolScale(void) { return 1.0f; }
float portDrawDistanceMultiplier(void) { return 1.0f; }
float portFogDistanceMultiplier(void) { return 1.0f; }
float portLodDistanceMultiplier(void) { return 1.0f; }
float portGunSwayTheta(float dt) { return 0; }
float portNativeAspect(void) { return 4.0f / 3; }
int portHudScalePercent(void) { return 100; }
int portAIWideView(void) { return 0; }
int portMouseAimPdGetTurn(float *x, float *y) { *x = *y = 0; return 0; }
int portCrosshairStyle(void) { return 0; }
float portCrosshairScale(void) { return 1; }
int portCrosshairPersistent(void) { return 0; }
void portCrosshairApplyTintCombine(void *cmd) {}
void portCrosshairTint(int *r, int *g, int *b) { *r = 255; *g = 0; *b = 0; }
void *optionsFileSelectLabel(void *dl) { return dl; }
int optionsOverlayIsOpen(void) { return 0; }
void *optionsOverlayEmit(void) { return 0; }
void optionsResetProbePrepare(void) {}
void optionsResetProbeDispatchStage(void) {}
void optionsResetProbeVerifyStage(void) {}
void d318WatchdogTick(void) {}
void crashInit(void) {}
void crashDumpThreads(const unsigned long *ids, const char **names, int n) {}
int geEepImportEmulatorSave(const char *path) { return 0; }
void pccgPatchTable(void) {}

u32 romdataFontPcSize(const u8 *src, u32 size) { return size; }
void romdataFixupFont(u8 *blob, u32 size)
{
    /* On i386 each of the 94 glyphs is six 32-bit words including its offset. */
    u32 words = size / 4;
    if (words > 169 + 94 * 6) words = 169 + 94 * 6;
    for (u32 i = 0; i < words; ++i)
        ((u32 *)blob)[i] = __builtin_bswap32(((u32 *)blob)[i]);
}

/* PDCLib provides strtod but omits the atof convenience wrapper. */
#include <stdlib.h>
double atof(const char *s) { return strtod(s, NULL); }
