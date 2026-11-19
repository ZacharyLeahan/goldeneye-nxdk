/* Run two processes against the same transport the game links. */
#include "../lan.h"
#include <SDL_timer.h>
#include <stdio.h>
#include <string.h>

#ifdef main
#undef main
#endif

#ifdef _WIN32
#include "system.h"
void sysLogPrintf(enum LogLevel level, const char *fmt, ...)
{
    (void)level;
    (void)fmt;
}
#endif

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    int host = strcmp(argv[1], "host") == 0;
    if (!(host ? lanHost() : lanJoin("127.0.0.1"))) {
        fprintf(stderr, "%s\n", lanStatus());
        return 1;
    }
    LanPose origin = {1009, 0, host ? 330 : 495, host ? 0 : 180, 0};
    int sawOrigin = 0, sawMovement = 0;
    uint32_t start = SDL_GetTicks(), joined = 0;
    while (SDL_GetTicks() - start < 3000) {
        lanPoll();
        LanPose pose = origin;
        if (lanConnected() && !joined) joined = SDL_GetTicks();
        if (joined && SDL_GetTicks()-joined > 300) {
            pose.z += 32; pose.yaw += 22;
            pose.forward = host ? 1.25f : -0.75f;
            pose.strafe = host ? -0.5f : 0.25f;
            pose.run = host;
        }
        lanPublish(&pose);
        LanPose other;
        if (lanRemote(&other)) {
            const int z = host ? 495 : 330;
            const int yaw = host ? 180 : 0;
            if (other.z == z && other.yaw == yaw) sawOrigin = 1;
            if (other.z == z+32 && other.yaw == yaw+22 &&
                other.forward == (host ? -0.75f : 1.25f) &&
                other.strafe == (host ? 0.25f : -0.5f) && other.run == !host) sawMovement = 1;
        }
        SDL_Delay(5);
    }
    lanClose();
    if (!(sawOrigin && sawMovement)) return 1;
    puts(host ? "Host received client movement and turn" : "Client received host movement and turn");
    return 0;
}
