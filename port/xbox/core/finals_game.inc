/* Simulation-thread bindings for the downstream controller extension. */
#include "port/xbox/finals_controls.h"
#include "finals_constants.h"
#include "stan.h"

extern float fminf(float, float);
extern float fmaxf(float, float);
extern float tanf(float);
extern void gunRequestHandWeaponChange(enum GUNHAND, s32, s32);
extern s32 get_ammo_type_for_weapon(ITEM_IDS);
extern void bgBuildRoomVtxBounds(s32);
extern s32 bgCheckIfRoomModelNeedsLoad(s32);

typedef struct {
    struct player *owner;
    int timer, loaded, jump, carrying, placed, sprint, wheelOpen, validPreview, blockFire;
    unsigned previous;
    float sprintBlend, padCooldown;
    coord3d pad, preview;
} XboxLife;
static XboxLife xboxLives[MAXCONTROLLERS];

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

static float xboxCeiling(float rise)
{
    struct player *p = g_CurrentPlayer;
    coord3d from = p->field_488.collision_position;
    float head = fmaxf(120, p->standheight) + 14;
    float scale = get_room_data_float1();
    float nearest = 1e9f;
    float top = p->field_70 + head + rise + 5;
    // A split-screen camera may be facing away from the launch. Prepare the
    // collision data for every room intersecting this body sweep.
    for (int r = 1; r < g_MaxNumRooms; ++r) {
        s_room_info *room=&g_BgRoomInfo[r];
        if (room->csize_primary_DL_binary <= 0 ||
            (from.x-31)*scale > room->maxbounds.x+1 || (from.x+31)*scale < room->minbounds.x-1 ||
            (from.z-31)*scale > room->maxbounds.z+1 || (from.z+31)*scale < room->minbounds.z-1 ||
            p->field_70*scale > room->maxbounds.y+1 || top*scale < room->minbounds.y-1) continue;
        if (!room->vtx_batch_bounds) {
            bgCheckIfRoomModelNeedsLoad(r);
            bgBuildRoomVtxBounds(r);
            if (!room->vtx_batch_bounds) return p->field_70;
        }
    }
    for (int ray = 0; ray < 17; ++ray) {
        float angle = (ray - 1) * .3926990817f;
        from.x = p->field_488.collision_position.x + (ray ? 31*cosf(angle) : 0);
        from.z = p->field_488.collision_position.z + (ray ? 31*sinf(angle) : 0);
        from.y = p->field_70 + 12.5f;
        coord3d to = from;
        to.y = top;
        for (int r = 1; r < g_MaxNumRooms; r++) {
            s_room_info *room = &g_BgRoomInfo[r];
            if (!room->vtx_batch_bounds || from.x*scale < room->minbounds.x-1 ||
                from.x*scale > room->maxbounds.x+1 || from.z*scale < room->minbounds.z-1 ||
                from.z*scale > room->maxbounds.z+1) continue;
            HitThing hit;
            if (bgTestBulletHitBackground(&from,&to,r,&hit)) {
                float y=hit.hitpos.y/scale;
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
    FinalsPad pad;
    xboxReadFinalsPad(slot, &pad);
    if (life->owner != p || g_GlobalTimer < life->timer || p->bonddead) {
        memset(life,0,sizeof(*life));
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
    if(!active){
        gunSetGunAmmoVisible(0x40000000,1);
        life->wheelOpen=0;
        xboxFinalsReset(slot);
        bondviewProcessInput(stick_x,stick_y,buttons,oldbuttons);
        return;
    }
    if(!life->loaded){
        bondinvAddInvItem(ITEM_GRENADE);
        p->ammoheldarr[get_ammo_type_for_weapon(ITEM_GRENADE)] = 3;
        life->loaded=1;
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
    int equipment;
    int selecting=xboxFinalsWheel(slot,1,&pad,&inventory,&equipment);
    life->wheelOpen=selecting;
    if(equipment>=0){
        life->carrying=equipment==PAD_ITEM;
        if(!life->carrying){
            gunRequestHandWeaponChange(GUNRIGHT,equipment,1);
            gunRequestHandWeaponChange(GUNLEFT,ITEM_UNARMED,1);
        }
    }
    int wheelInput=selecting || ((previous|pad.buttons)&FIN_WHEEL);
    if (life->carrying) life->validPreview=xboxPadPreview(&life->preview);
    if ((life->carrying || wheelInput) && (pad.buttons & FIN_FIRE)) life->blockFire=1;
    int native=buttons & START_BUTTON, old=oldbuttons & START_BUTTON;
    if(!wheelInput){
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
    // Accept a new trigger squeeze on the same report as a pad selection.
    if ((!wheelInput || equipment==PAD_ITEM) && !selecting && life->carrying &&
        (pressed&FIN_FIRE) && life->validPreview) {
        life->pad=life->preview;
        life->placed=1;
        life->padCooldown=0;
        life->carrying=0;
    }
    gunSetGunAmmoVisible(0x40000000,!life->carrying);
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
    p->speedtheta=p->speedverta=0;
    if(!wheelInput) xboxFinalsLook(slot,&pad,p->fovy,g_GlobalTimerDelta,&p->vv_theta,&p->vv_verta);
}

static void xboxFinalsVertical(void)
{
    XboxLife *life=&xboxLives[get_cur_playernum()];
    struct player *p=g_CurrentPlayer;
    int jump=life->jump;
    life->jump=0;
    if(p->bonddead || lvlGetControlsLockedFlag() || !life->loaded) return;
    float impulse=jump && xboxGrounded()?5.8f:0;
    for(int i=0;i<getPlayerCount();i++){
        XboxLife *other=&xboxLives[i];
        if(i==get_cur_playernum()) other->padCooldown=fmaxf(0,other->padCooldown-g_GlobalTimerDelta);
        if(!other->placed || other->padCooldown>0 || !xboxGrounded()) continue;
        float dx=p->field_488.collision_position.x-other->pad.x,dz=p->field_488.collision_position.z-other->pad.z;
        if(dx*dx+dz*dz <= PAD_RADIUS*PAD_RADIUS && fabsf(p->field_70-other->pad.y)<8) {
            impulse=PAD_IMPULSE; other->padCooldown=45;
        }
    }
    if(impulse && xboxCeiling(impulse+4)>p->field_70+5){
        float h=stanGetPositionYValue(p->field_488.current_tile_ptr,p->field_488.collision_position.x,p->field_488.collision_position.z);
        float floor=p->stanHeight;
        if(h>floor && h-floor<=8) floor=h;
        p->field_70=fmaxf(p->field_70,floor)+.05f; p->field_7C=impulse;
    }
    if(p->field_7C>0 && p->field_70>p->stanHeight){
        float rise=p->field_7C*g_GlobalTimerDelta;
        if(xboxCeiling(rise+2)<=p->field_70+rise+2) p->field_7C=0;
    }
}

#include "port/xbox/core/pad_render.inc"
