#if defined(_WIN32) && !defined(__XBOX__)
#include "system.h"
#include <stdlib.h>
#define DROP_TRACE(...) do { if (getenv("GE_LAN_TRACE")) sysLogPrintf(LOG_INFO, __VA_ARGS__); } while (0)
#else
#define DROP_TRACE(...) ((void)0)
#endif
/* The host owns loose weapons created by the native death handler. A bitmask
 * retires collected items immediately. Repeating one live transform per
 * snapshot also populates the world for a guest who joins after the drop. */
static struct {
    ObjectRecord *object;
    LanDrop last;
} lanDrops[LAN_DROP_SLOTS];
static uint32_t lanActiveDrops, lanDropGeneration;
static unsigned lanNextDrop;
_Static_assert(MAX_WEAPON_SLOTS <= LAN_DROP_SLOTS, "native dropped weapon capacity");

void lanResetDrops(void)
{
    __builtin_memset(lanDrops, 0, sizeof(lanDrops));
    lanActiveDrops = lanDropGeneration = lanNextDrop = 0;
}

static void lanForgetDrop(ObjectRecord *obj)
{
    if (!lanActiveDrops || obj->type != PROPDEF_COLLECTABLE) return;
    for (unsigned i = 0; i < LAN_DROP_SLOTS; ++i) {
        if (lanDrops[i].object != obj) continue;
        DROP_TRACE("LAN removed drop %u item %u", lanDrops[i].last.generation, lanDrops[i].last.item);
        lanActiveDrops &= ~(1u << i);
        lanDrops[i].object = NULL;
        break;
    }
}

static void lanTrackDrop(PropRecord *prop)
{
    for (unsigned i = 0; i < LAN_DROP_SLOTS; ++i) {
        if (lanDrops[i].object) continue;
        if (!++lanDropGeneration) ++lanDropGeneration;
        lanDrops[i].object = prop->obj;
        lanDrops[i].last = (LanDrop){i, lanDropGeneration, prop->weapon->weaponnum};
        lanActiveDrops |= 1u << i;
        DROP_TRACE("LAN created drop %u item %u", lanDropGeneration, prop->weapon->weaponnum);
        return;
    }
}

void lanReadDrops(LanPose *state)
{
    state->activeDrops = lanActiveDrops;
    if (!lanActiveDrops) return;
    while (!lanDrops[lanNextDrop].object)
        lanNextDrop = (lanNextDrop + 1) % LAN_DROP_SLOTS;
    ObjectRecord *obj = lanDrops[lanNextDrop].object;
    state->drop = lanDrops[lanNextDrop].last;
    state->drop.room = obj->prop->stan ? obj->prop->stan->room : 255;
    for (int row = 0; row < 3; ++row) {
        state->drop.position[row] = obj->runtime_pos.f[row];
        __builtin_memcpy(state->drop.rotation[row], obj->mtx.m[row], sizeof(state->drop.rotation[row]));
    }
    lanNextDrop = (lanNextDrop + 1) % LAN_DROP_SLOTS;
}

void lanApplyDrops(const LanPose *state)
{
    const LanDrop *drop = &state->drop;
    uint32_t removed = lanActiveDrops & ~state->activeDrops;
    if (drop->generation && lanDrops[drop->slot].object &&
        lanDrops[drop->slot].last.generation != drop->generation) removed |= 1u << drop->slot;
    for (unsigned i = 0; removed; ++i) {
        if (!(removed & (1u << i))) continue;
        removed &= ~(1u << i);
        objFreePermanently(lanDrops[i].object, TRUE);
    }
    if (!drop->generation || drop->item >= ITEM_IDS_MAX) return;
    unsigned i = drop->slot;
    ObjectRecord *obj = lanDrops[i].object;
    if (obj && !__builtin_memcmp(&lanDrops[i].last, drop, sizeof(*drop))) return;
    coord3d position = {drop->position[0], drop->position[1], drop->position[2]};
    coord3d probe = position;
    probe.y += 4;
    float floor;
    u8 rooms[] = {(u8)drop->room, 255};
    StandTile *tile = stanFindTileBelowPos(&probe, drop->room == 255 ? NULL : rooms, &floor);
    if (!tile) return;
    if (!obj) {
        int model = getPropForHeldItem(drop->item);
        if (model < 0) return;
        obj = create_new_item_instance_of_model(model, drop->item);
        if (!obj) return;
        chrpropActivate(obj->prop);
        chrpropEnable(obj->prop);
        lanDrops[i].object = obj;
        lanActiveDrops |= 1u << i;
        DROP_TRACE("LAN mirrored drop %u item %u", drop->generation, drop->item);
    }
    Mtxf transform;
    matrix_4x4_set_identity(&transform);
    for (int row = 0; row < 3; ++row)
        __builtin_memcpy(transform.m[row], drop->rotation[row], sizeof(drop->rotation[row]));
    objChangeShading(obj, &position, &transform, tile);
    setupUpdateObjectRoomPosition(obj);
    lanDrops[i].last = *drop;
}

static void lanCollectDrop(ObjectRecord *obj)
{
    if (!lanActiveDrops || obj->type != PROPDEF_COLLECTABLE) return;
#if defined(_WIN32) && !defined(__XBOX__)
    for (unsigned i = 0; i < LAN_DROP_SLOTS; ++i)
        if (lanDrops[i].object == obj)
            DROP_TRACE("LAN awarded drop %u item %u seat %d pistolAmmo %d", lanDrops[i].last.generation,
                       lanDrops[i].last.item, get_cur_playernum(),
                       g_CurrentPlayer->ammoheldarr[get_ammo_type_for_weapon(ITEM_WPPK)] +
                       (g_CurrentPlayer->hands[GUNRIGHT].weaponnum == ITEM_WPPK
                           ? g_CurrentPlayer->hands[GUNRIGHT].weapon_ammo_in_magazine : 0));
#endif
    lanForgetDrop(obj);
}

#if defined(_WIN32) && !defined(__XBOX__)
/* Expose the native world prop to the local integration fixture. The shooter
 * can approach the PP7 even when native drop physics toss it behind the body. */
int lanTestDroppedPistol(coord3d *position)
{
    for (unsigned i = 0; i < LAN_DROP_SLOTS; ++i) {
        ObjectRecord *obj = lanDrops[i].object;
        if (obj && lanDrops[i].last.item == ITEM_WPPK) {
            *position = obj->runtime_pos;
            return 1;
        }
    }
    return 0;
}
#endif
