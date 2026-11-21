#include <libaudio.h>
#include "port/lan/frame_trace.h"

/* Several local deaths can request the same sequence player in one game tick.
 * A play event may still be queued (or prefetched for the audio callback)
 * before its state changes. Report it so the caller queues a stop and waits
 * before it reuses the sequence buffer. */
s32 alCSPGetState(ALCSPlayer *player)
{
    OSIntMask mask = osSetIntMask(OS_IM_NONE);
    s32 state = __atomic_load_n(&player->state, __ATOMIC_ACQUIRE);
    if (state == AL_STOPPED) {
        if (player->nextEvent.type == AL_SEQP_PLAY_EVT) state = AL_PLAYING;
        for (ALLink *link = player->evtq.allocList.next; link && state == AL_STOPPED; link = link->next) {
            if (((ALEventListItem *)link)->evt.type == AL_SEQP_PLAY_EVT) state = AL_PLAYING;
        }
    }
    osSetIntMask(mask);
    return state;
}

void xboxWaitForSequenceStop(ALCSPlayer *player)
{
    ftRecord(FT_SEQ_WAIT_BEGIN, ftGameId, (uintptr_t)player);
    /* The Xbox audio worker consumes the pending stop. Give it CPU time
     * before polling again, while preserving ownership of the sequence
     * buffer until the previous player has actually stopped. */
    while (alCSPGetState(player)) {
        osYieldThread();
    }
    ftRecord(FT_SEQ_WAIT_END, ftGameId, (uintptr_t)player);
}
