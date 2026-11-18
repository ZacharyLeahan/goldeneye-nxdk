#include <libaudio.h>

/* The game waits here for the audio thread to finish stopping a track during
 * a stage change. Whole-program optimization must reload this shared field. */
s32 alCSPGetState(ALCSPlayer *player)
{
    return __atomic_load_n(&player->state, __ATOMIC_ACQUIRE);
}
