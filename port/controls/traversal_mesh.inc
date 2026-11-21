#include "include/gbi_extension.h"

/* A mantle sweeps many short segments through the same few static triangles.
 * Copy local-room geometry once so room eviction and a second viewport cannot
 * invalidate it. Dense regions use the engine's ordinary room ray instead. */
enum { TRAVERSAL_TRIANGLES = 384 };
typedef struct {
    Vertex v[3];
    coord3d low, high;
} TraversalTriangle;
typedef struct {
    float x, z;
    int valid, count;
    short start[MAXROOMCOUNT], end[MAXROOMCOUNT];
    TraversalTriangle triangles[TRAVERSAL_TRIANGLES];
} TraversalMesh;
static TraversalMesh playerMeshes[MAXCONTROLLERS];
static TraversalMesh *mesh;
static void meshPrepare(int slot, coord3d at)
{
    mesh = &playerMeshes[slot];
    if (mesh->valid && fabsf(at.x - mesh->x) <= 100 && fabsf(at.z - mesh->z) <= 100)
        return;
    mesh->x = at.x;
    mesh->z = at.z;
    mesh->count = 0;
    mesh->valid = 1;
    memset(mesh->start, -1, sizeof(mesh->start));
}

static int meshAdd(Vertex *vertices, int a, int b, int c, coord3d origin)
{
    if (a == b || a == c || b == c)
        return 1;
    Vertex *v[3] = {vertices + a, vertices + b, vertices + c};
    coord3d low, high;
    for (int k = 0; k < 3; k++) {
        float v0 = v[0]->coord.AsArray[k], v1 = v[1]->coord.AsArray[k], v2 = v[2]->coord.AsArray[k];
        low.f[k] = origin.f[k] + fminf(v0, fminf(v1, v2));
        high.f[k] = origin.f[k] + fmaxf(v0, fmaxf(v1, v2));
    }
    const float span = 100 + MANTLE_REACH + MANTLE_BARRIER_SPAN + 3 * MANTLE_RADIUS + 4;
    if (low.x > (mesh->x + span) * rayScale || high.x < (mesh->x - span) * rayScale ||
        low.z > (mesh->z + span) * rayScale || high.z < (mesh->z - span) * rayScale)
        return 1;
    if (mesh->count == TRAVERSAL_TRIANGLES)
        return 0;
    TraversalTriangle *triangle = &mesh->triangles[mesh->count++];
    triangle->low = low;
    triangle->high = high;
    for (int i = 0; i < 3; i++)
        triangle->v[i] = *v[i];
    return 1;
}

static int meshReadRoom(int id)
{
    s_room_info *room = &g_BgRoomInfo[id];
    coord3d origin = ptr_bgdata_room_fileposition_list[id].pos;
    for (int i = 0; i < room->num_vtx_batch_bounds; i++) {
        RoomVtxBatchBounds *batch = &room->vtx_batch_bounds[i];
        Gfx *cmd = (Gfx *)room->ptr_expanded_mapping_info + batch->gdlindex;
        int first = (cmd->words.w0 >> 16) & 15, count = ((cmd->words.w0 >> 20) & 15) + 1;
        Vertex *vertices = (Vertex *)((u8 *)room->vertices + (cmd->words.w1 & 0xffffff));
        for (cmd++;; cmd++) {
            int op = (u8)cmd->dma.cmd;
            if (op == (u8)G_ENDDL || op == (u8)G_VTX)
                break;
            if (op != (u8)G_TRI1 && op != (u8)G_TRI4)
                continue;
            u32 w0 = cmd->words.w0, w1 = cmd->words.w1;
            for (int j = 0; j < (op == (u8)G_TRI4 ? 4 : 1); j++) {
                int a, b, c;
                if (op == (u8)G_TRI4) {
                    a = w1 & 15;
                    b = (w1 >> 4) & 15;
                    c = w0 & 15;
                    w1 >>= 8;
                    w0 >>= 4;
                } else {
                    a = ((w1 >> 16) & 255) / 10;
                    b = ((w1 >> 8) & 255) / 10;
                    c = (w1 & 255) / 10;
                }
                if (a == b || b == c || a == c)
                    continue;
                a -= first;
                b -= first;
                c -= first;
                if ((unsigned)a >= count || (unsigned)b >= count || (unsigned)c >= count ||
                    !meshAdd(vertices, a, b, c, origin))
                    return 0;
            }
        }
    }
    return 1;
}
