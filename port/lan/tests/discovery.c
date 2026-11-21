/* Exercise host discovery and joining through the same transport as the game. */
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
    int host = strcmp(argv[1], "host") == 0 || strcmp(argv[1], "advertise") == 0;
    int advertise = strcmp(argv[1], "advertise") == 0;
    int scan = strcmp(argv[1], "scan") == 0;
    if (host) {
        if (!lanSetArena(11) || !lanHost()) {
            fprintf(stderr, "%s\n", lanStatus());
            return 1;
        }
    } else if (!lanDiscover()) {
        fprintf(stderr, "%s\n", lanStatus());
        return 1;
    }

    uint32_t start = SDL_GetTicks();
    int found = 0, refreshed = 0;
    uint32_t timeout = scan ? 15000u : 6000u;
    while (SDL_GetTicks() - start < timeout) {
        lanPoll();
        if (advertise && SDL_GetTicks() - start >= 2200u) {
            lanClose();
            puts("Host stopped advertising");
            return 0;
        }
        if (scan && lanDiscoveryCount() > 1) {
            fputs("Duplicate discovery entry\n", stderr);
            return 1;
        }
        if (!host && !found && lanDiscoveryCount() > 0) {
            const LanMatch *match = lanDiscoveryMatch(0);
            if (!match || strcmp(match->name, "GoldenEye Host") || match->arena != 11 ||
                match->players != 1 || match->capacity != 2 || match->port != LAN_PORT) {
                fputs("Invalid discovery metadata\n", stderr);
                return 1;
            }
            found = 1;
            if (scan) {
                lanDiscoverRefresh();
                if (lanDiscoveryCount() != 0) {
                    fputs("Refresh did not clear stale entries\n", stderr);
                    return 1;
                }
                refreshed = 1;
                continue;
            }
            if (!lanJoinDiscovered(0)) {
                fprintf(stderr, "%s\n", lanStatus());
                return 1;
            }
        }
        if (scan && refreshed && lanDiscoveryCount() == 1) refreshed = 2;
        if (lanConnected()) break;
        SDL_Delay(5);
    }
    if (scan) {
        int passed = found && refreshed == 2 && lanDiscoveryCount() == 0;
        lanClose();
        if (!passed) {
            fputs("Refresh, deduplication or expiry failed\n", stderr);
            return 1;
        }
        puts("Refresh deduplicated the host and expiry removed it");
        return 0;
    }
    int passed = lanConnected() && (host || found);
    lanClose();
    if (!passed) {
        fprintf(stderr, "%s timed out\n", host ? "Host" : "Discovery client");
        return 1;
    }
    puts(host ? "Host advertised and accepted discovered client" :
                "Client discovered metadata and joined host");
    return 0;
}
