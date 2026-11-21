/* Simulation-thread bindings for the downstream controller extension. */
#include "finals_controls.h"
#include "port/lan/lan.h"
#include "finals_constants.h"
#include "stan.h"
#if defined(_WIN32) && !defined(__XBOX__)
#include "system.h"
#endif

extern float fminf(float, float);
extern float fmaxf(float, float);
extern float tanf(float);
extern void gunRequestHandWeaponChange(enum GUNHAND, s32, s32);
extern s32 get_ammo_type_for_weapon(ITEM_IDS);
extern void bgBuildRoomVtxBounds(s32);
extern s32 bgCheckIfRoomModelNeedsLoad(s32);

typedef struct {
    enum { SLAP_READY, SLAP_DRAW, SLAP_STRIKE } phase;
    ITEM_IDS weapon[2], previous[2], fistModel;
    int magazine[2], started;
} QuickSlap;

typedef struct {
    QuickSlap slap;
    struct player *owner;
    int timer, loaded, jump, carrying, placed, sprint, wheelOpen, validPreview, blockFire;
    unsigned previous;
    float sprintBlend, padCooldown;
    coord3d pad, preview;
} XboxLife;
static XboxLife xboxLives[MAXCONTROLLERS];
static uint32_t lanButtons, lanTriggerPress, lanConsumedTrigger;

static uint32_t lanSlapPress, lanConsumedSlap;

static void slapEquip(int index, ITEM_IDS weapon)
{
    struct hand *hand = &g_CurrentPlayer->hands[index];
    // Fist can represent either a palm or the sniper butt. Reload its model
    // even when both the old and the new logical weapon are ITEM_FIST.
    if (weapon == ITEM_FIST && g_CurrentPlayer->hand_item[index] == ITEM_FIST)
        g_CurrentPlayer->hand_item[index] = ITEM_UNARMED;
    currentPlayerUnEquipWeaponWrapper(index, weapon);
    hand->weapon_action_state = GUN_ANIM_STATE_IDLE;
    hand->weapon_current_animation = 0;
    hand->weapon_animation_trigger = 0;
    hand->field_88C = hand->field_890 = 0;
    hand->weapon_firing_status = 0;
}

static int quickSlapTick(QuickSlap *slap, int request)
{
    struct player *p = g_CurrentPlayer;
    struct hand *right = &p->hands[GUNRIGHT];
    if (slap->phase == SLAP_READY) {
        if (!request || right->weaponnum < ITEM_FIST || right->weaponnum > ITEM_TANKSHELLS)
            return 0;
        for (int h = 0; h < 2; ++h) {
            struct hand *hand = &p->hands[h];
            if (hand->weaponnum_watchmenu >= 0 || hand->weapon_current_animation ||
                hand->weapon_action_state >= GUN_ANIM_STATE_SWITCH_LOWER)
                return 0;
        }
        slap->started = g_GlobalTimer;
        slap->fistModel = p->cur_item_weapon_getname;
        p->cur_item_weapon_getname = ITEM_FIST;
        for (int h = 0; h < 2; ++h) {
            struct hand *hand = &p->hands[h];
            slap->weapon[h] = hand->weaponnum;
            slap->previous[h] = hand->previous_weapon;
            slap->magazine[h] = hand->weapon_ammo_in_magazine;
            // Keep the loaded rounds with the holstered weapon during the swap.
            hand->weapon_ammo_in_magazine = 0;
            slapEquip(h, h == GUNRIGHT ? ITEM_FIST : ITEM_UNARMED);
        }
        slap->phase = SLAP_DRAW;
    }
    if ((slap->phase == SLAP_STRIKE && right->weapon_action_state == GUN_ANIM_STATE_IDLE) ||
        g_GlobalTimer - slap->started > 120) {
        p->cur_item_weapon_getname = slap->fistModel;
        for (int h = 0; h < 2; ++h) {
            slapEquip(h, slap->weapon[h]);
            p->hands[h].previous_weapon = slap->previous[h];
            p->hands[h].weapon_ammo_in_magazine = slap->magazine[h];
        }
        slap->phase = SLAP_READY;
    } else if (slap->phase == SLAP_DRAW && p->hand_item[GUNRIGHT] == ITEM_FIST &&
               p->hand_invisible[GUNRIGHT] >= 0) {
        right->weapon_action_state = GUN_ANIM_STATE_PUNCH1_STRIKE;
        right->field_88C = right->field_890 = 0;
        slap->phase = SLAP_STRIKE;
    }
    return 1;
}

static int xboxGrounded(void)
{
    struct player *p = g_CurrentPlayer;
    float gap = p->stanHeight - p->field_70;
    if (!p->field_488.current_tile_ptr || fabsf(p->field_7C) >= .6f) return 0;
    if (fabsf(gap) < 1.5f) return 1;
    coord3d *at = &p->field_488.collision_position;
    return gap > 0 && gap <= 32 &&
        stanTestPointWithinTileBoundsMaybe(p->field_488.current_tile_ptr, at->x, at->z) &&
        fabsf(stanGetPositionYValue(p->field_488.current_tile_ptr, at->x, at->z) - p->stanHeight) < 3;
}

#include <float.h>
#include "traversal.inc"

static float xboxCeiling(float rise)
{
    struct player *p = g_CurrentPlayer;
    coord3d from = p->field_488.collision_position;
    float head = fmaxf(120, p->standheight) + 14;
    float scale = get_room_data_float1();
    float nearest = 1e9f;
    float top = p->field_70 + head + rise + 5;
    for (int ray = 0; ray < 17; ++ray) {
        float angle = (ray - 1) * .3926990817f;
        from.x = p->field_488.collision_position.x + (ray ? 31*cosf(angle) : 0);
        from.z = p->field_488.collision_position.z + (ray ? 31*sinf(angle) : 0);
        from.y = p->field_70 + .25f;
        // On a ramp the uphill side of the footprint starts inside the
        // walking surface. Only discard hits adjacent to its connected tile.
        float support = -FLT_MAX;
        StandTile *toe = p->field_488.current_tile_ptr;
        coord3d center = p->field_488.collision_position;
        if (toe && (!ray || walkTilesBetweenPoints_NoCallback(&toe,center.x,center.z,from.x,from.z)) && toe) {
            float h = stanGetPositionYValue(toe,from.x,from.z);
            if (h <= p->field_70 + 48) {
                support = h;
                from.y = fmaxf(from.y,h+.5f);
            }
        }
        coord3d to = from;
        to.y = top;
        for (int r=0;r<nearbyCount;r++) {
            HitThing hit;
            int result=roomRay(nearbyRooms[r],from,to,&hit);
            if(result<0)return p->field_70;
            if(result) {
                float y=hit.hitpos.y/scale;
                if (hit.normal.y>0 && fabsf(y-support)<=12 && y>=from.y-.5f && y<top) {
                    coord3d above=from;
                    above.y=y+.5f;
                    result=roomRay(nearbyRooms[r],above,to,&hit);
                    if(result<0)return p->field_70;
                    if(!result)continue;
                    y=hit.hitpos.y/scale;
                }
                if (y>=from.y-.5f && y<=to.y+1 && y<nearest) nearest=y;
            }
        }
    }
    return nearest-head;
}

static int xboxPadPreview(coord3d *preview)
{
    coord3d at = g_CurrentPlayer->field_488.collision_position;
    coord3d dir = g_CurrentPlayer->field_488.theta_transform;
    float distance = 220;
    if (g_CurrentPlayer->vv_verta < -10)
        distance = fmaxf(75,fminf(240,(at.y-g_CurrentPlayer->field_70)/tanf(-g_CurrentPlayer->vv_verta*.01745329252f)));
    coord3d spot = at;
    spot.x += dir.x*distance; spot.z += dir.z*distance;
    spot.y = g_CurrentPlayer->stanHeight;
    *preview = spot;
    if (!xboxGrounded()) return 0;
    StandTile *tile = g_CurrentPlayer->field_488.current_tile_ptr;
    if (!walkTilesBetweenPoints_NoCallback(&tile,at.x,at.z,spot.x,spot.z) || !tile) return 0;
    spot.y = stanGetPositionYValue(tile,spot.x,spot.z);
    *preview = spot;
    if (fabsf(spot.y-g_CurrentPlayer->stanHeight)>24) return 0;
    for(int i=0;i<16;i++){
        float x=spot.x+PAD_RADIUS*cosf(i*.3926990817f), z=spot.z+PAD_RADIUS*sinf(i*.3926990817f);
        StandTile *edge=tile;
        if(!walkTilesBetweenPoints_NoCallback(&edge,spot.x,spot.z,x,z) || !edge ||
           fabsf(stanGetPositionYValue(edge,x,z)-spot.y)>3) return 0;
    }
    return 1;
}

static void xboxFinalsProcessInput(s8 stick_x, s8 stick_y, u16 buttons, u16 oldbuttons)
{
    int slot = get_cur_playernum();
    struct player *p = g_CurrentPlayer;
    XboxLife *life = &xboxLives[slot];
    traversal=&traversalPlayers[slot];
    int remoteSeat = slot == 1 && lanActive();
    FinalsPad pad = {0};
    if (!remoteSeat) xboxReadFinalsPad(slot, &pad);
#if defined(_WIN32) && !defined(__XBOX__)
    /* The existing Windows harness supplies independent native pad states. */
    static const char *const scripts[] = {"GE_INPUTSCRIPT", "GE_INPUTSCRIPT_P1",
                                          "GE_INPUTSCRIPT_P2", "GE_INPUTSCRIPT_P3"};
    static int scripted[] = {-1, -1, -1, -1};
    if (scripted[slot] < 0) scripted[slot] = getenv(scripts[slot]) != NULL;
    if (!remoteSeat && scripted[slot]) {
        pad = (FinalsPad){0};
        if (buttons & Z_TRIG) pad.buttons |= FIN_FIRE;
        if (buttons & L_CBUTTONS) pad.buttons |= FIN_SLAP; /* scripted right-stick click */
        if (buttons & A_BUTTON) pad.buttons |= FIN_A;
        if (buttons & B_BUTTON) pad.buttons |= FIN_B;
        if (buttons & R_TRIG) pad.buttons |= FIN_AIM;
        if (buttons & L_TRIG) pad.buttons |= FIN_WHEEL;
        pad.lx = stick_x * 400;
        pad.ly = -stick_y * 400;
        if (buttons & L_TRIG) { pad.rx = pad.lx; pad.ry = pad.ly; pad.lx = pad.ly = 0; }
    }
#endif
    LanPose peerInput = {0};
    int remoteSlap = 0;
    if (remoteSeat) {
        lanRemote(&peerInput);
        if (peerInput.life && peerInput.slapPress != lanConsumedSlap) {
            lanConsumedSlap = peerInput.slapPress;
            remoteSlap = 1;
        }
        pad.buttons = peerInput.buttons & (FIN_FIRE | FIN_AIM | FIN_X);
        if (peerInput.life && peerInput.triggerPress != lanConsumedTrigger) {
            lanConsumedTrigger = peerInput.triggerPress;
            pad.buttons |= FIN_FIRE;
        }
    }
    if (life->owner != p || traversal->mantle.stage!=lvlGetCurrentStageToLoad() || g_GlobalTimer < life->timer || p->bonddead) {
        remoteSlap = 0;
        memset(life,0,sizeof(*life));
        memset(traversal,0,sizeof(*traversal));
        traversal->life=life;
        traversal->mantle.stage=lvlGetCurrentStageToLoad();
        playerMeshes[slot].valid=0;
        life->owner=p; life->previous=pad.buttons;
        xboxFinalsReset(slot);
    }
    life->timer=g_GlobalTimer;
    unsigned pressed=pad.buttons & ~life->previous;
    unsigned previous=life->previous;
    int previousFireBlocked=life->blockFire;
    life->previous=pad.buttons;
    if (!(pad.buttons & FIN_FIRE)) life->blockFire=0;
    int active = !p->bonddead && !g_PlayerIsInTank && !g_stopPlayFlag && !g_gameOverFlag &&
        p->watch_animation_state==WATCH_ANIMATION_0x0 && !lvlGetControlsLockedFlag() &&
        disablePlayerActionsWhenPausedOrInMpMenu();
    traversal->allowed=active;
    traversal->mantle.clock=g_GlobalTimer;
    if(!active){
        mantleAbort(); slideStop(); life->jump=life->sprint=0;
        gunSetGunAmmoVisible(0x40000000,1);
        life->wheelOpen=0;
        xboxFinalsReset(slot);
        bondviewProcessInput(stick_x,stick_y,buttons,oldbuttons);
        return;
    }
    if(!life->loaded){
        if (lanActive()) {
            bondinvAddInvItem(ITEM_WPPK);
            p->ammoheldarr[get_ammo_type_for_weapon(ITEM_WPPK)] = 49;
            currentPlayerEquipWeaponWrapper(GUNRIGHT, ITEM_WPPK);
        }
        bondinvAddInvItem(ITEM_GRENADE);
        p->ammoheldarr[get_ammo_type_for_weapon(ITEM_GRENADE)] = 3;
        life->loaded=1;
    }
    if (!life->slap.phase && remoteSeat && peerInput.life && peerInput.weapon >= ITEM_KNIFE &&
        peerInput.weapon <= ITEM_TANKSHELLS && bondinvItemAvailable(peerInput.weapon) &&
        get_item_in_hand_or_watch_menu(GUNRIGHT) != peerInput.weapon) {
        gunRequestHandWeaponChange(GUNRIGHT, peerInput.weapon, 1);
    }
    FinalsInventory inventory={0};
    for(int id=4;id<=25;id++)
        if(id!=23 && bondinvItemAvailable(id)) inventory.items[inventory.count++]=id;
    inventory.items[inventory.count++]=ITEM_GRENADE;
    inventory.items[inventory.count++]=PAD_ITEM;
    inventory.grenades=get_ammo_count_for_weapon(ITEM_GRENADE);
    inventory.current=life->carrying?PAD_ITEM:getCurrentPlayerWeaponId(GUNRIGHT);
    inventory.validPlacement=life->validPreview;
    inventory.left=getPlayer_c_screenleft()/viGetX(); inventory.top=getPlayer_c_screentop()/viGetY();
    inventory.width=getPlayer_c_screenwidth()/viGetX(); inventory.height=getPlayer_c_screenheight()/viGetY();
    if (life->slap.phase) pad.buttons &= ~FIN_WHEEL;
    int equipment;
    int selecting=xboxFinalsWheel(slot,1,&pad,&inventory,&equipment);
#if defined(_WIN32) && !defined(__XBOX__)
    static int traceSplit = -1;
    static struct { int initialized, wheel, weapon, grenades, shots; } previousSeat[MAXCONTROLLERS];
    if (traceSplit < 0) traceSplit = getenv("GE_SPLIT_TRACE") != NULL;
    if (traceSplit && !lanActive()) {
        int shots = g_playerPlayerData[slot].shot_count[0];
        int grenades = inventory.grenades;
        for (int hand = 0; hand < 2; ++hand)
            if (p->hands[hand].weaponnum == ITEM_GRENADE)
                grenades += p->hands[hand].weapon_ammo_in_magazine;
        if (!previousSeat[slot].initialized || previousSeat[slot].wheel != selecting ||
            previousSeat[slot].weapon != inventory.current || previousSeat[slot].grenades != grenades ||
            previousSeat[slot].shots != shots) {
            sysLogPrintf(LOG_INFO, "Split seat %d wheel %d weapon %d grenades %d shots %d",
                         slot, selecting, inventory.current, grenades, shots);
            previousSeat[slot].initialized = 1;
            previousSeat[slot].wheel = selecting;
            previousSeat[slot].weapon = inventory.current;
            previousSeat[slot].grenades = grenades;
            previousSeat[slot].shots = shots;
        }
    }
#endif
    life->wheelOpen=selecting;
    if(equipment>=0){
        life->carrying=equipment==PAD_ITEM;
        if(!life->carrying){
            gunRequestHandWeaponChange(GUNRIGHT,equipment,1);
            gunRequestHandWeaponChange(GUNLEFT,ITEM_UNARMED,1);
        }
    }
    int wheelInput=selecting || ((previous|pad.buttons)&FIN_WHEEL);
    int requestSlap = !wheelInput && !traversal->mantle.active &&
                      (remoteSeat ? remoteSlap : (pressed & FIN_SLAP) != 0);
    int startingSlap = !life->slap.phase;
    int slapping = quickSlapTick(&life->slap, requestSlap);
    if (slapping && startingSlap && slot == 0 && lanActive()) ++lanSlapPress;
    if (slapping && (pad.buttons & FIN_FIRE)) life->blockFire = 1;
    if (life->carrying) life->validPreview=xboxPadPreview(&life->preview);
    if ((life->carrying || wheelInput) && (pad.buttons & FIN_FIRE)) life->blockFire=1;
    int native=buttons & START_BUTTON, old=oldbuttons & START_BUTTON;
    if(!wheelInput && !slapping){
        if(pad.buttons&FIN_Y) native|=B_BUTTON;
        if(previous&FIN_Y) old|=B_BUTTON;
        if((pad.buttons&FIN_AIM)&&!life->carrying) native|=R_TRIG;
        if(previous&FIN_AIM) old|=R_TRIG;
        if((pad.buttons&FIN_FIRE)&&!life->blockFire) native|=Z_TRIG;
        if((previous&FIN_FIRE)&&!previousFireBlocked) old|=Z_TRIG;
        if(pressed&FIN_X){attempt_reload_item_in_hand(GUNRIGHT);attempt_reload_item_in_hand(GUNLEFT);}
        if(pressed&FIN_B){
            if(life->carrying) life->carrying=0;
            else p->crouchpos=p->crouchpos==CROUCH_STAND?CROUCH_SQUAT:CROUCH_STAND;
        }
        if(pressed&FIN_A){life->jump=1;p->crouchpos=CROUCH_STAND;}
    }
    traversal->blocked=wheelInput || life->carrying || traversal->padFlight;
    slideBefore(!wheelInput && (pressed & FIN_B));
    if (traversal->mantle.active) native &= ~Z_TRIG;
    if (lanActive() && slot == 0) {
        unsigned effective = pad.buttons & (FIN_AIM | FIN_X | FIN_A);
        if (native & Z_TRIG) effective |= FIN_FIRE;
        if ((effective & FIN_FIRE) && !(lanButtons & FIN_FIRE)) ++lanTriggerPress;
        lanButtons = effective;
    }
    // Accept a new trigger squeeze on the same report as a pad selection.
    if ((!wheelInput || equipment==PAD_ITEM) && !slapping && !selecting && life->carrying &&
        (pressed&FIN_FIRE) && life->validPreview) {
        life->pad=life->preview;
        life->placed=1;
        life->padCooldown=0;
        life->carrying=0;
    }
    gunSetGunAmmoVisible(0x40000000,slapping || !life->carrying);
    bondviewProcessInput(0,0,native,old);
    float lx=pad.lx,ly=pad.ly;
    float length=sqrtf(lx*lx+ly*ly);
    float strength=fmaxf(0,fminf(1,(length-4000)/(32767-4000)));
    float forward=length?-ly/length*strength:0;
    float side=length?lx/length*strength:0;
    if((pressed&FIN_SPRINT)&&forward>.45f) life->sprint=!life->sprint;
    if(forward<.3f || wheelInput || life->carrying || (native&(R_TRIG|Z_TRIG)) || p->crouchpos!=CROUCH_STAND) life->sprint=0;
    life->sprintBlend=life->sprint?fminf(1,life->sprintBlend+g_GlobalTimerDelta/9):0;
    float scale=(native&R_TRIG)?.90f:MOVE_RUN_SCALE*(1+(MOVE_SPRINT_RATIO-1)*life->sprintBlend);
    p->speedforwards=forward*1.08f*scale; p->speedsideways=side*scale;
    traversal->mantle.forward=forward>.25f;
    slideAfter();
    if(traversal->mantle.active) p->speedforwards=p->speedsideways=0;
    p->speedtheta=p->speedverta=0;
    if(!wheelInput) xboxFinalsLook(slot,&pad,p->fovy,g_GlobalTimerDelta,&p->vv_theta,&p->vv_verta);
}

static void xboxFinalsVertical(int *useHeight, float *offset)
{
    XboxLife *life=&xboxLives[get_cur_playernum()];
    struct player *p=g_CurrentPlayer;
    traversal=&traversalPlayers[get_cur_playernum()];
    int jump=life->jump;
    life->jump=0;
    if(!traversal->allowed || !life->loaded) return;
    traversal->jumpCooldown=fmaxf(0,traversal->jumpCooldown-g_GlobalTimerDelta);
    if (traversal->airborne && xboxGrounded()) traversal->airborne=traversal->padFlight=0;
    if (jump && !traversal->mantle.active) mantlePress(g_GlobalTimer);
    float impulse=jump && !traversal->mantle.active && traversal->jumpCooldown<=0 && xboxGrounded()?5.8f:0;
    for(int i=0;i<getPlayerCount();i++){
        XboxLife *other=&xboxLives[i];
        if(i==get_cur_playernum()) other->padCooldown=fmaxf(0,other->padCooldown-g_GlobalTimerDelta);
        if(!other->placed || other->padCooldown>0 || !xboxGrounded()) continue;
        float dx=p->field_488.collision_position.x-other->pad.x,dz=p->field_488.collision_position.z-other->pad.z;
        if(dx*dx+dz*dz <= PAD_RADIUS*PAD_RADIUS && fabsf(p->field_70-other->pad.y)<8) {
            impulse=PAD_IMPULSE; other->padCooldown=45;
        }
    }
    if(impulse || p->field_7C>0 || traversal->mantle.active || traversal->mantle.armed)
        traversalPrepare();
    if(impulse && xboxCeiling(70)>p->field_70+5){
        float h=stanGetPositionYValue(p->field_488.current_tile_ptr,p->field_488.collision_position.x,p->field_488.collision_position.z);
        float floor=p->stanHeight;
        if(h>floor && h-floor<=8) floor=h;
        traversal->jumpCooldown=8;
        if(impulse==PAD_IMPULSE){ mantleStop(); traversal->padFlight=1; }
        traversal->airborne=1; traversal->origin=floor; traversal->takeoff=g_GlobalTimer;
        p->field_70=fmaxf(p->field_70,floor)+.05f; p->field_7C=impulse;
    }
    if(p->field_7C>0 && p->field_70>p->stanHeight){
        float rise=p->field_7C*g_GlobalTimerDelta;
        if(xboxCeiling(rise+2)<=p->field_70+rise+2) p->field_7C=0;
    }
    if(traversal->mantle.active || traversal->mantle.armed){
        mantleVertical(useHeight,offset);
    }
}

#include "pad_render.inc"
