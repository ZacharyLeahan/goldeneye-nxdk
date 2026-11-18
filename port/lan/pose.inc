#include "port/lan/lan.h"

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

static void lanMovePlayer(void)
{
    if (!lanActive() || getPlayerCount() != 2) return;
    struct player *p = g_CurrentPlayer;
    int slot = get_cur_playernum();
    static struct player *placed[2];
    static int lastTick[2];
    if (!p->prop || !p->field_488.current_tile_ptr) return;
    lanPoll();
    if (placed[slot] != p || g_GlobalTimer < lastTick[slot]) {
        int hostSeat = lanIsHost() == (slot == 0);
        LanPose spawn = {1009.9f, 0, hostSeat ? 330 : 495, hostSeat ? 0 : 180, 0};
        if (!lanPlace(p, &spawn)) return;
        placed[slot] = p;
    }
    lastTick[slot] = g_GlobalTimer;
    if (slot == 0) {
        LanPose pose = {p->field_488.collision_position.x, p->field_70,
                        p->field_488.collision_position.z, p->vv_theta, p->vv_verta};
        lanPublish(&pose);
    } else {
        LanPose pose;
        if (lanRemote(&pose)) lanPlace(p, &pose);
    }
}
