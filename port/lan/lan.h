#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { LAN_PORT = 27007 };
enum { LAN_COMPLEX = 2, LAN_ARENAS = 11 };
/* Scoped to the game thread while it builds the discarded LAN seat. */
extern int lanSkipViewDraw;
int lanArena(void);
int lanSetArena(int arena);
const char *lanArenaName(int arena);
int lanSetScoreLimit(int limit);
int lanScoreLimit(void);
uint32_t lanRound(void);
uint32_t lanPeerId(void);
void lanBeginRematch(void);
enum { LAN_AMMO_TYPES = 30, LAN_WEAPON_WORDS = 2, LAN_WORLD_WORDS = 2 };
enum { LAN_DROP_SLOTS = 30 };
typedef struct {
    uint32_t slot, generation, item, room;
    float position[3], rotation[3][3];
} LanDrop;
typedef struct {
    uint32_t ready, owned[LAN_WEAPON_WORDS], paired[LAN_WEAPON_WORDS];
    uint32_t reserve[LAN_AMMO_TYPES], weapon[2], magazine[2];
} LanInventory;
/* Native seats are local-first on each machine. The host owns the session
 * and each player publishes their own movement for the Complex movement test. */
typedef struct {
    float health, armour;
    uint32_t dead, life, kills[2], deaths;
    float spawnX, spawnFeet, spawnZ, spawnYaw;
    LanInventory inventory;
} LanSeat;
typedef struct {
    float x, feet, z, yaw, pitch, forward, strafe, run;
    uint32_t buttons, triggerPress, weapon, life, results, ready;
    LanSeat seats[2]; /* Only the host authors these, in host seat order. */
    uint32_t firstCommand, commands, hidden[LAN_WORLD_WORDS];
    uint32_t activeDrops;
    LanDrop drop;
} LanPose;
void lanResetDrops(void);
void lanReadDrops(LanPose *state);
void lanApplyDrops(const LanPose *state);
#ifdef GE_LAN_HOST
int lanDesktopStart(void);
int lanDesktopWait(void);
#endif
int lanHost(void);
int lanJoin(const char *ipv4);
int lanHostOnPort(uint16_t port);
int lanJoinOnPort(const char *ipv4, uint16_t port);
void lanClose(void);
int lanActive(void);
int lanIsHost(void);
int lanConnected(void);
void lanPoll(void);
void lanPublish(const LanPose *pose);
int lanPublishDue(void);
uint32_t lanRemoteSequence(void);
int lanRemote(LanPose *pose);
const char *lanStatus(void);
#ifdef __cplusplus
}
#endif
