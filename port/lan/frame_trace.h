#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

enum { FT_LIST_TIMED = 1, FT_LIST_REUSE_MATERIAL = 2, FT_LIST_TRACE_TRI4 = 4, FT_LIST_PROJECT_TRI4 = 8 };

enum FrameEvent {
    FT_VI = 1, FT_GATE, FT_GAME, FT_BUILD, FT_SEAT, FT_VIS, FT_PROPS,
    FT_BG, FT_AFTER_BG, FT_SEAT_END, FT_SUBMIT, FT_HANDOFF, FT_RENDER,
    FT_RENDER_END, FT_QUEUE, FT_READY, FT_SCAN, FT_ACQUIRED, FT_DRAIN,
    FT_GPU_IDLE, FT_TRIANGLES, FT_SHOTS, FT_VIEW_DRAW, FT_LIST_BEGIN, FT_LIST_END, FT_WAKE_MODE, FT_WAKE_SERVICE, FT_TRI4_COST,
    FT_CMD_QUEUE, FT_WAKE_POST, FT_IRQ_RECEIVE, FT_CMD_APPEND, FT_WAKE_DEPTH, FT_AUDIO_START
};

#if defined(GE_LAN_HOST) || defined(__XBOX__) || defined(GE_PBKIT_TRACE)
extern unsigned ftGameId, ftRenderId;
int ftRecording(void);
int ftListBegin(void);
void ftListEnd(void);
uint64_t ftTri4Begin(void);
void ftTri4End(uint64_t start, unsigned commands);
uint64_t ftDrawBegin(void);
void ftDrawEnd(uint64_t started);
enum { PORT_GFX_WAKE_MSG = 670, PORT_GFX_PAIR_MSG = 671 };
int ftPairTrace(void);
int ftEarlyWake(void);
unsigned ftRasterPhase(void);
int ftSkipRemoteDraw(void);
void ftGameStart(int eligible);
void ftRenderStart(uintptr_t dl);
void ftRecord(unsigned event, unsigned id, uint64_t detail);
#else
#define ftListBegin() 0
#define ftListEnd() ((void)0)
#define ftTri4Begin() 0
#define ftTri4End(start, commands) ((void)0)
#define ftDrawBegin() 0
#define ftDrawEnd(started) ((void)(started))
#define ftRecord(...) ((void)0)
#define ftRenderStart(...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif
