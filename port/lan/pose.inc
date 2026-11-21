#include "port/lan/lan.h"
#if defined(_WIN32) && !defined(__XBOX__)
#include "system.h"
#endif
void mp_respawn_handler(void);
void bondviewKillCurrentPlayer(void);
static uint32_t lanLife[2] = {1, 1};
static LanPose lanSpawn[2];
static int lanSpawnReady[2];
static uint32_t lanWantsRespawn;
static uint32_t loadedRound, previousPeer, appliedPacket[2];
static struct player *placed[2];
static int lastTick[2], roundReloading, resultsLogged;
#include "port/lan/inventory.inc"
#if defined(_WIN32) && !defined(__XBOX__)
#include "port/lan/tests/pickup_fixture.inc"
#endif

void lanEnterRound(void)
{
    loadedRound = lanRound();
    lanResetDrops();
    roundReloading = resultsLogged = lanWantsRespawn = 0;
    previousPeer = lanPeerId();
    lanSlapPress = lanConsumedSlap = 0;
    lanButtons = lanTriggerPress = lanConsumedTrigger = 0;
    for (int i = 0; i < 2; ++i) {
        lanLife[i] = 1;
        lanSpawnReady[i] = appliedPacket[i] = lastTick[i] = 0;
        placed[i] = NULL;
        xboxLives[i] = (XboxLife){0};
    }
#if defined(_WIN32) && !defined(__XBOX__)
    lanTestTarget = NULL;
    if (getenv("GE_LAN_TRACE"))
        sysLogPrintf(LOG_INFO, "LAN round %u started arena %d score limit %d", loadedRound, lanArena(), lanScoreLimit());
#endif
}

static void lanReloadRound(void)
{
    extern struct mp_stage_setup multi_stage_setups[];
    roundReloading = 1;
    MP_stage_selected = lanArena();
    selected_stage = multi_stage_setups[MP_stage_selected].stage_id;
    bossSetLoadedStage(selected_stage);
}

static void lanNewLife(void)
{
    if (!lanIsHost()) return;
    int slot = get_cur_playernum();
    struct player *p = g_CurrentPlayer;
    ++lanLife[slot];
    lanSpawnReady[slot] = 1;
    lanSpawn[slot] = (LanPose){p->field_488.collision_position.x, p->field_70,
                               p->field_488.collision_position.z, p->vv_theta};
}

static int lanRespawnButton(int buttons)
{
    if (!lanActive()) return buttons;
    if (!lanIsHost()) {
        if (get_cur_playernum() == 0 && buttons) lanWantsRespawn = FIN_A;
        return 0;
    }
    if (get_cur_playernum() == 0) return buttons;
    LanPose remote;
    return lanRemote(&remote) && remote.life == lanLife[1] && (remote.buttons & FIN_A);
}

/* Wall/floor and room pointers are local engine objects. Reconstruct them from
 * world coordinates on the receiving architecture. */
static int lanPlace(struct player *p, const LanPose *pose)
{
    coord3d point = {pose->x, pose->feet + 4, pose->z};
    float floor;
    StandTile *tile = stanFindTileBelowPos(&point, NULL, &floor);
    if (!tile) return 0;
    p->field_488.current_tile_ptr = tile;
    p->field_488.collision_position.x = point.x;
    p->field_488.collision_position.z = point.z;
    p->stanHeight = floor;
    p->field_70 = pose->feet;
    p->field_6C = pose->feet / .2004f;
    p->field_7C = 0;
    p->vv_theta = pose->yaw;
    p->vv_verta = pose->pitch;
    bondviewApplyVertaTheta();
    return 1;
}

/* Run even through death cameras, where MoveBond can be skipped. */
void lanTickSeat(void)
{
    if (!lanActive() || getPlayerCount() != 2) return;
    lanPoll();
    if (roundReloading) return;
    if (loadedRound != lanRound()) { lanReloadRound(); return; }
    int slot = get_cur_playernum();
    struct player *p = g_CurrentPlayer;
    if (!p->prop || !p->field_488.current_tile_ptr) return;
    LanPose remote = {0};
    int received = lanRemote(&remote);
    if (lanIsHost() && lanPeerId() && lanPeerId() != previousPeer) {
        struct player *guest = g_playerPointers[1];
        if (guest && guest->prop && lanSpawnReady[1]) {
            lanSpawn[1] = (LanPose){guest->field_488.collision_position.x, guest->field_70,
                                    guest->field_488.collision_position.z, guest->vv_theta};
#if defined(_WIN32) && !defined(__XBOX__)
            if (getenv("GE_LAN_TRACE"))
                sysLogPrintf(LOG_INFO, "LAN restoring guest life %u at %.2f %.2f", lanLife[1],
                             lanSpawn[1].x, lanSpawn[1].z);
#endif
        }
        mpClearTargetHits();
        previousPeer = lanPeerId();
        lanConsumedTrigger = 0;
    }
    if (!lanIsHost() && received) {
        uint32_t packet = lanRemoteSequence();
        const LanSeat *s = &remote.seats[1-slot];
        if (s->life) {
            if (!lanSpawnReady[slot] || s->life != lanLife[slot]) {
                if (s->life != lanLife[slot]) mp_respawn_handler();
                LanPose spawn = {s->spawnX,s->spawnFeet,s->spawnZ,s->spawnYaw};
                if (!lanPlace(p,&spawn)) return;
#if defined(_WIN32) && !defined(__XBOX__)
                if (!slot && getenv("GE_LAN_TRACE"))
                    sysLogPrintf(LOG_INFO, "LAN placed guest life %u at %.2f %.2f", s->life, spawn.x, spawn.z);
#endif
                lanSpawnReady[slot] = 1;
                lanLife[slot] = s->life;
                if (!slot) lanWantsRespawn = 0;
            }
            if (s->dead && !p->bonddead) bondviewKillCurrentPlayer();
            if (p->bondhealth > s->health || p->bondarmour > s->armour) {
                p->oldhealth = p->bondhealth;
                p->oldarmour = p->bondarmour;
                p->healthshowtime = p->damageshowtime = 0;
            }
            p->bondhealth = s->health;
            p->bondarmour = s->armour;
            p->deathcount = s->deaths;
            if (appliedPacket[slot] != packet) {
                if (slot == 1) {
                    TargetHit *hit = &targetHits[0][1];
                    hit->expires = remote.guestHitTicks && remote.guestHitTicks <= HIT_TOTAL_TICKS
                        ? g_GlobalTimer + remote.guestHitTicks : 0;
                    hit->previousHealth = fmaxf(0, fminf(1, remote.guestHitPreviousHealth));
                }
                if (!slot) {
                    lanApplyPickups(&remote);
                    lanApplyDrops(&remote);
                }
                if (!s->dead && s->inventory.ready && xboxLives[slot].loaded)
                    lanApplyInventory(p, &s->inventory);
                appliedPacket[slot] = packet;
            }
            g_playerPlayerData[slot].kill_count = s->kills[slot];
            for (int victim=0; victim<2; ++victim)
                g_playerPlayerData[slot].kill_counts[victim] = s->kills[1-victim];
        }
    }
    if (!slot && !lanIsHost() && received && remote.results && !g_gameOverFlag) {
        /* Both kill matrices must be current before the native awards pass. */
        for (int who = 0; who < 2; ++who)
            for (int victim = 0; victim < 2; ++victim)
                g_playerPlayerData[who].kill_counts[victim] = remote.seats[1-who].kills[1-victim];
        mpCalculateAwards(FALSE);
    }
    int ready = g_gameOverFlag && g_playerPointers[0]->mpmenumode == MENU_FINISHED;
    if (!slot && lanIsHost() && ready && received && remote.ready) {
        lanBeginRematch();
        lanReloadRound();
        return;
    }
#if defined(_WIN32) && !defined(__XBOX__)
    static int trace = -1;
    static struct { float health; int dead, shots, kills, life, weapon, ammo; } last[2];
    if (trace < 0) trace = getenv("GE_LAN_TRACE") != NULL;
    if (trace) {
        if (g_gameOverFlag && !resultsLogged) {
            int host = !lanIsHost();
            sysLogPrintf(LOG_INFO, "LAN round %u results host %d guest %d", loadedRound,
                         get_points_for_mp_player(host), get_points_for_mp_player(1-host));
            resultsLogged = 1;
        }
        lanTestPickupTrace();
        int shots = g_playerPlayerData[slot].shot_count[0], kills = g_playerPlayerData[slot].kill_count;
        int weapon = getCurrentPlayerWeaponId(GUNRIGHT);
        int ammo = p->ammoheldarr[get_ammo_type_for_weapon(ITEM_WPPK)];
        if (weapon == ITEM_WPPK) ammo += p->hands[GUNRIGHT].weapon_ammo_in_magazine;
        if (last[slot].life != lanLife[slot] || last[slot].health != p->bondhealth ||
            last[slot].dead != !!p->bonddead || last[slot].shots != shots || last[slot].kills != kills ||
            last[slot].weapon != weapon || last[slot].ammo != ammo) {
            sysLogPrintf(LOG_INFO, "LAN seat %d life %u hp %.3f dead %d kills %d shots %d weapon %d pistolAmmo %d",
                         slot, lanLife[slot], p->bondhealth, !!p->bonddead, kills, shots, weapon, ammo);
            last[slot].ammo=ammo;
            last[slot].life=lanLife[slot]; last[slot].health=p->bondhealth; last[slot].dead=!!p->bonddead;
            last[slot].kills=kills; last[slot].shots=shots; last[slot].weapon=weapon;
        }
    }
#endif
    if (slot != 0 || !lanPublishDue()) return;
    LanPose pose = {p->field_488.collision_position.x, p->field_70,
                    p->field_488.collision_position.z, p->vv_theta, p->vv_verta,
                    p->speedforwards, p->speedsideways, p->headanim != 0};
    pose.buttons = p->bonddead ? lanWantsRespawn : lanButtons;
    pose.slapPress = lanSlapPress;
    pose.triggerPress = lanTriggerPress;
    pose.results = g_gameOverFlag != 0;
    pose.ready = ready;
    pose.weapon = xboxLives[0].slap.phase ? xboxLives[0].slap.weapon[GUNRIGHT] : getCurrentPlayerWeaponId(GUNRIGHT);
    if (pose.weapon > 255) pose.weapon = 0;
    pose.life = lanSpawnReady[0] ? lanLife[0] : 0;
    if (lanIsHost()) {
        const TargetHit *guestHit = &targetHits[1][0];
        if (guestHit->expires > g_GlobalTimer) {
            pose.guestHitTicks = guestHit->expires - g_GlobalTimer;
            pose.guestHitPreviousHealth = guestHit->previousHealth;
        }
        lanPickupPage(&pose);
        lanReadDrops(&pose);
        for (int i=0; i<2; ++i) {
            struct player *seat = g_playerPointers[i];
            if (!seat || !seat->prop || !lanSpawnReady[i]) continue;
            pose.seats[i] = (LanSeat){seat->bondhealth, seat->bondarmour, seat->bonddead != 0, lanLife[i],
                {g_playerPlayerData[i].kill_counts[0], g_playerPlayerData[i].kill_counts[1]}, seat->deathcount,
                lanSpawn[i].x,lanSpawn[i].feet,lanSpawn[i].z,lanSpawn[i].yaw};
            if (!seat->bonddead && xboxLives[i].loaded)
                lanReadInventory(seat, &pose.seats[i].inventory);
        }
    }
    lanPublish(&pose);
}

static void lanMovePlayer(void)
{
    if (!lanActive() || getPlayerCount() != 2) return;
    struct player *p = g_CurrentPlayer;
    int slot = get_cur_playernum();
    if (!p->prop || !p->field_488.current_tile_ptr) return;
    lanPoll();
    if (placed[slot] != p || g_GlobalTimer < lastTick[slot]) {
        int hostSeat = lanIsHost() == (slot == 0);
        LanPose spawn = {1009.9f, 0, hostSeat ? 330 : 495, hostSeat ? 0 : 180, 0};
        if (lanArena() != LAN_COMPLEX)
            spawn = (LanPose){p->field_488.collision_position.x, p->field_70,
                              p->field_488.collision_position.z, p->vv_theta};
#if defined(_WIN32) && !defined(__XBOX__)
        lanTestFindPickup();
        if (lanTestTarget) {
            spawn = lanTestSpawn;
            spawn.z += hostSeat ? -12 : 12;
            spawn.yaw = hostSeat ? 0 : 180;
        }
#endif
        if (lanIsHost()) {
            if (!lanPlace(p, &spawn)) return;
            lanSpawn[slot] = spawn;
            lanSpawnReady[slot] = 1;
        }
        placed[slot] = p;
    }
    lastTick[slot] = g_GlobalTimer;
#if defined(_WIN32) && !defined(__XBOX__)
    static int testDrop = -1, dropSeen;
    if (testDrop < 0) testDrop = getenv("GE_LAN_TEST_DROP") != NULL;
    if (testDrop && !slot) {
        extern int lanTestDroppedPistol(coord3d *);
        coord3d position;
        if (lanTestDroppedPistol(&position)) {
            if (!dropSeen) dropSeen = g_GlobalTimer;
            if (g_GlobalTimer - dropSeen >= 480) {
                float floor;
                position.y += 4;
                if (stanFindTileBelowPos(&position, NULL, &floor)) {
                    LanPose approach = {position.x, floor, position.z, p->vv_theta};
                    lanPlace(p, &approach);
                }
            }
        } else dropSeen = 0;
    }
    /* Leave the pickup area so native regeneration can run after the test. */
    static int leftArea[2];
    if (lanTestTarget && (lanTestTarget->runtime_bitflags & RUNTIMEBITFLAG_00000800) && !leftArea[slot]) {
        LanPose exit = {1009.9f, 0, lanIsHost() == (slot == 0) ? 330 : 495};
        leftArea[slot] = lanPlace(p, &exit);
    }
#endif
    if (slot == 1) {
        LanPose pose;
        if (lanRemote(&pose) && pose.life == lanLife[slot]) lanPlace(p, &pose);
    }
}
