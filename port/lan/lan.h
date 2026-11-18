#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { LAN_PORT = 27007 };
/* Native seats are local-first on each machine. The host owns the session
 * and each player publishes their own movement for the Complex movement test. */
typedef struct { float x, feet, z, yaw, pitch; } LanPose;
#ifdef GE_LAN_HOST
int lanDesktopHost(void);
int lanDesktopWait(void);
#endif
int lanHost(void);
int lanJoin(const char *ipv4);
void lanClose(void);
int lanActive(void);
int lanIsHost(void);
int lanConnected(void);
void lanPoll(void);
void lanPublish(const LanPose *pose);
int lanRemote(LanPose *pose);
const char *lanStatus(void);
#ifdef __cplusplus
}
#endif
