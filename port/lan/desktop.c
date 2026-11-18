#include <SDL.h>
#include "system.h"
#include "lan.h"

int lanDesktopHost(void)
{
    if (!lanHost()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "GoldenEye LAN", lanStatus(), NULL);
        return 0;
    }
    sysLogPrintf(LOG_INFO, "LAN hosting two-player Complex on UDP 27007");
    return 1;
}

int lanDesktopWait(void)
{
    SDL_Window *window = SDL_GetWindowFromID(1);
    if (window) SDL_SetWindowTitle(window, "GoldenEye LAN - Waiting for Xbox on UDP 27007");
    while (!lanConnected()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE)) {
                lanClose();
                return 0;
            }
        }
        lanPoll();
        SDL_Delay(10);
    }
    if (window) SDL_SetWindowTitle(window, "GoldenEye LAN - Complex");
    sysLogPrintf(LOG_INFO, "Xbox joined the LAN match");
    return 1;
}
