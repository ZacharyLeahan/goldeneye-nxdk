#pragma once

#include <PR/ultratypes.h>

#ifdef __cplusplus
extern "C" {
#endif

void inputStartEgyptTest(void);
int inputInit(void);
void inputUpdate(void);
int inputConnectedMask(void);
unsigned inputComputePad(int slot, signed char *x, signed char *y);

#ifdef __cplusplus
}
#endif
