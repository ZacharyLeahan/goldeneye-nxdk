/* Native bindings for the fork's traversal tests and movement curves. All
 * inputs and persistent state belong to the player being simulated. */
typedef coord3d JumpVec;
#include "finals_traversal_types.inc"
typedef struct {
    coord3d point;
    StandTile *tile;
    float height;
    int valid;
} TraversalFloor;

typedef struct {
    XboxMantle mantle;
    XboxSlide slide;
    int allowed, blocked, airborne, padFlight, takeoff;
    float origin, jumpCooldown;
    XboxLife *life;
    TraversalFloor floors[128];
} XboxTraversal;
static XboxTraversal traversalPlayers[MAXCONTROLLERS];
static XboxTraversal *traversal;

static StandTile *floorBelow(float x, float y, float z, float *height)
{
    coord3d point = {x, y, z};
    // Nav tiles are static for the level. The hand and footprint probes often
    // ask for exactly the same landing tile throughout a grab.
    u32 bits[3];
    memcpy(bits, point.f, sizeof(bits));
    u32 hash = bits[0] * 31u + bits[1] * 13u + bits[2];
    TraversalFloor *entry = &traversal->floors[(hash ^ (hash >> 11) ^ (hash >> 20)) & 127];
    if (!entry->valid || entry->point.x != x || entry->point.y != y || entry->point.z != z) {
        entry->point = point;
        entry->tile = stanFindTileBelowPos(&point, NULL, &entry->height);
        entry->valid = 1;
    }
    *height = entry->height;
    return entry->tile;
}

static void mantleStop(void) { traversal->mantle.active = traversal->mantle.armed = 0; }

static void mantleAbort(void)
{
    if (traversal->mantle.active) {
        struct player *p = g_CurrentPlayer;
        coord3d pos = p->field_488.collision_position;
        float h;
        StandTile *under = floorBelow(pos.x, p->field_70 + 1, pos.z, &h);
        if (under) {
            p->field_488.current_tile_ptr = under;
            p->stanHeight = h;
        }
        traversal->airborne = 1;
    }
    mantleStop();
}

/* Only rooms in reach of this press can contribute a triangle. Keep the
 * candidates for this simulation tick, and load their mesh on first use. */
static int nearbyRooms[MAXROOMCOUNT];
static int nearbyCount, occupiedRooms[64];
static float rayScale;
#include "traversal_mesh.inc"
typedef struct {
    coord3d lo, hi;
    int actor;
} TraversalObstacle;
static TraversalObstacle obstacles[MAX_PROPS];
static int obstacleCount, unboundedProp;

static void prepareObstacles(void)
{
    obstacleCount = unboundedProp = 0;
    chraiGetPropRoomIds(g_CurrentPlayer->prop, occupiedRooms);
    roomGetProps(occupiedRooms);
    for (s16 *index = ptr_list_object_lookup_indices; *index >= 0; index++) {
        PropRecord *prop = &g_Props[*index];
        if (prop == g_CurrentPlayer->prop)
            continue;
        TraversalObstacle ob;
        ob.actor = prop->type == PROP_TYPE_CHR || prop->type == PROP_TYPE_VIEWER;
        if (ob.actor) {
            if (!prop->chr) {
                unboundedProp = 1;
                continue;
            }
            rect4f *polygon;
            int edges;
            float top, bottom;
            chraiGetCollisionBounds(prop, &polygon, &edges, &top, &bottom);
            if (!polygon || edges <= 0)
                continue;
            ob.lo = (coord3d){FLT_MAX, bottom, FLT_MAX};
            ob.hi = (coord3d){-FLT_MAX, top, -FLT_MAX};
            coord2d *points = (coord2d *)polygon;
            for (int j = 0; j < edges; j++) {
                ob.lo.x = fminf(ob.lo.x, points[j].x);
                ob.hi.x = fmaxf(ob.hi.x, points[j].x);
                ob.lo.z = fminf(ob.lo.z, points[j].y);
                ob.hi.z = fmaxf(ob.hi.z, points[j].y);
            }
        } else if (prop->type == PROP_TYPE_OBJ || prop->type == PROP_TYPE_DOOR ||
                   prop->type == PROP_TYPE_WEAPON) {
            ObjectRecord *obj = prop->obj;
            if (!obj || !obj->model || !obj->model->obj || prop->child || prop->parent) {
                unboundedProp = 1;
                continue;
            }
            float radius = getinstsize(obj->model);
            if (!(radius > 0)) {
                unboundedProp = 1;
                continue;
            }
            for (int k = 0; k < 3; k++) {
                ob.lo.f[k] = obj->runtime_pos.f[k] - radius;
                ob.hi.f[k] = obj->runtime_pos.f[k] + radius;
            }
        } else
            continue;
        obstacles[obstacleCount++] = ob;
    }
}

static int intersectsObstacle(coord3d from, coord3d delta, const TraversalObstacle *ob)
{
    float enter = 0, leave = 1;
    for (int k = 0; k < 3; k++) {
        float end = from.f[k] + delta.f[k];
        if ((from.f[k] < ob->lo.f[k] && end < ob->lo.f[k]) ||
            (from.f[k] > ob->hi.f[k] && end > ob->hi.f[k]))
            return 0;
        if (fabsf(delta.f[k]) < .00001f)
            continue;
        float a = (ob->lo.f[k] - from.f[k]) / delta.f[k], b = (ob->hi.f[k] - from.f[k]) / delta.f[k];
        enter = fmaxf(enter, fminf(a, b));
        leave = fminf(leave, fmaxf(a, b));
        if (enter > leave)
            return 0;
    }
    return 1;
}

static void traversalPrepare(void)
{
    coord3d at = g_CurrentPlayer->field_488.collision_position;
    const float reach = MANTLE_REACH + MANTLE_BARRIER_SPAN + 3 * MANTLE_RADIUS;
    rayScale = get_room_data_float1();
    meshPrepare(get_cur_playernum(), at);
    nearbyCount = 0;
    for (int i = 1; i < g_MaxNumRooms; i++) {
        s_room_info *room = &g_BgRoomInfo[i];
        if (room->csize_primary_DL_binary > 0 && (at.x - reach) * rayScale <= room->maxbounds.x + 1 &&
            (at.x + reach) * rayScale >= room->minbounds.x - 1 &&
            (at.z - reach) * rayScale <= room->maxbounds.z + 1 &&
            (at.z + reach) * rayScale >= room->minbounds.z - 1)
            nearbyRooms[nearbyCount++] = i;
    }
    if (traversal->mantle.armed || traversal->mantle.active)
        prepareObstacles();
}

extern s32 intersectRayTriangle(Vertex *, Vertex *, Vertex *, coord3d *, coord3d *, coord3d *,
                                coord3d *, HitThing *);

static int roomRay(int roomId, coord3d from, coord3d to, HitThing *hit)
{
    s_room_info *room = &g_BgRoomInfo[roomId];
    for (int k = 0; k < 3; k++) {
        float lo = room->minbounds.f[k] - 1, hi = room->maxbounds.f[k] + 1;
        float a = from.f[k] * rayScale, b = to.f[k] * rayScale;
        if ((a < lo && b < lo) || (a > hi && b > hi))
            return 0;
    }
    if (mesh->start[roomId] == -1) {
        if (!room->vtx_batch_bounds) {
            bgCheckIfRoomModelNeedsLoad(roomId);
            bgBuildRoomVtxBounds(roomId);
            if (!room->vtx_batch_bounds)
                return -1;
        }
        int first = mesh->count;
        if (meshReadRoom(roomId)) {
            mesh->start[roomId] = first;
            mesh->end[roomId] = mesh->count;
        } else {
            mesh->count = first;
            mesh->start[roomId] = -2;
        }
    }
    if (mesh->start[roomId] == -2) {
        if (!room->vtx_batch_bounds) {
            bgCheckIfRoomModelNeedsLoad(roomId);
            bgBuildRoomVtxBounds(roomId);
            if (!room->vtx_batch_bounds)
                return -1;
        }
        return bgTestBulletHitBackground(&from, &to, roomId, hit);
    }
    // Bullet rays may continue beyond their endpoint and look up a decal.
    // Traversal needs only the nearest triangle in the finite sweep.
    coord3d a, b, dir, low, high;
    for (int k = 0; k < 3; k++) {
        a.f[k] = from.f[k] * rayScale;
        b.f[k] = to.f[k] * rayScale;
        dir.f[k] = b.f[k] - a.f[k];
        low.f[k] = fminf(a.f[k], b.f[k]) - 1;
        high.f[k] = fmaxf(a.f[k], b.f[k]) + 1;
    }
    float nearest = FLT_MAX;
    coord3d *origin = &ptr_bgdata_room_fileposition_list[roomId].pos;
    for (int i = mesh->start[roomId]; i < mesh->end[roomId]; i++) {
        TraversalTriangle *t = &mesh->triangles[i];
        if (t->low.x > high.x || t->high.x < low.x || t->low.z > high.z || t->high.z < low.z ||
            t->low.y > high.y || t->high.y < low.y)
            continue;
        HitThing candidate;
        if (intersectRayTriangle(t->v, t->v + 1, t->v + 2, origin, &a, &b, &dir, &candidate)) {
            float dx = candidate.hitpos.x - a.x, dy = candidate.hitpos.y - a.y,
                  dz = candidate.hitpos.z - a.z;
            float dist = dx * dx + dy * dy + dz * dz;
            if (dist < nearest) {
                nearest = dist;
                hit->hitpos = candidate.hitpos;
                hit->normal = candidate.normal;
            }
        }
    }
    return nearest < FLT_MAX;
}

static int backgroundFloorBetween(float x, float z, float low, float high, float *height)
{
    coord3d from = {x, high, z}, to = {x, low, z};
    float best = -FLT_MAX;
    for (int i = 0; i < nearbyCount; i++) {
        HitThing hit;
        int result = roomRay(nearbyRooms[i], from, to, &hit);
        if (result < 0)
            return 0;
        if (result) {
            float y = hit.hitpos.y / rayScale;
            if (y >= low && y <= high + .1f && y > best)
                best = y;
        }
    }
    if (best == -FLT_MAX)
        return 0;
    *height = best;
    return 1;
}

static int mantleFacesPlayer(coord3d a, coord3d b)
{
    float dx = b.x - a.x, dz = b.z - a.z, len2 = dx * dx + dz * dz;
    float nearest = 1.01f, alignment = 0;
    for (int i = 0; i < nearbyCount; i++) {
        HitThing hit;
        int result = roomRay(nearbyRooms[i], a, b, &hit);
        if (result < 0)
            return 0;
        if (!result)
            continue;
        float t = ((hit.hitpos.x / rayScale - a.x) * dx + (hit.hitpos.z / rayScale - a.z) * dz) / len2;
        float normal2 = hit.normal.x * hit.normal.x + hit.normal.z * hit.normal.z;
        if (t > 0 && t < nearest && normal2 > 1e-8f) {
            nearest = t;
            alignment = fabsf(hit.normal.x * dx + hit.normal.z * dz) / sqrtf(normal2 * len2);
        }
    }
    return alignment > .45f;
}

extern s32 projectileFindCollidingProp(PropRecord *, coord3d *, coord3d *, u32, coord3d *, coord3d *,
                                       s32 *);
static int dropSegmentClear(coord3d from, coord3d to)
{
    coord3d delta = {to.x - from.x, to.y - from.y, to.z - from.z};
    float len2 = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
    if (len2 < .0001f)
        return 1;
    for (int i = 0; i < nearbyCount; i++) {
        HitThing hit;
        int result = roomRay(nearbyRooms[i], from, to, &hit);
        if (result < 0)
            return 0;
        if (result) {
            float t = 0;
            for (int k = 0; k < 3; k++)
                t += (hit.hitpos.f[k] / rayScale - from.f[k]) * delta.f[k];
            if (t > .001f * len2 && t < 1.001f * len2)
                return 0;
        }
    }
    int dynamic = unboundedProp;
    for (int i = 0; i < obstacleCount; i++) {
        const TraversalObstacle *ob = &obstacles[i];
        if (!intersectsObstacle(from, delta, ob))
            continue;
        if (ob->actor)
            return 0;
        dynamic = 1;
    }
    if (!dynamic)
        return 1;
    coord3d pos, normal;
    return !projectileFindCollidingProp(g_CurrentPlayer->prop, &from, &to, 0x13, &pos, &normal,
                                        occupiedRooms);
}

static float traversalLength(float a, float b) { return sqrtf(a * a + b * b); }
#define hypotf traversalLength
#define isfinite __builtin_isfinite
#define jumpActive traversal->airborne
#define jumpOriginFloor traversal->origin
#define jumpTakeoffTick traversal->takeoff
#define pendingJump traversal->life->jump
#define sprintBlend traversal->life->sprintBlend
#define sprintOn traversal->life->sprint
#define mantleTrace(event) ((void)0)
#include "finals_traversal.inc"
#undef mantleTrace
#undef sprintOn
#undef sprintBlend
#undef pendingJump
#undef jumpTakeoffTick
#undef jumpOriginFloor
#undef jumpActive
#undef isfinite
#undef hypotf
