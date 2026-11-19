/* Windows-only integration fixture. Both native seats approach the same
 * authored item. Hold its award until both are inside its pickup radius. */
static ObjectRecord *lanTestTarget;
static int lanTestCommand, lanTestKind = -1;
static LanPose lanTestSpawn;

static void lanTestFindPickup(void)
{
    if (lanTestKind < 0) {
        const char *kind = getenv("GE_LAN_TEST_PICKUP");
        lanTestKind = !kind ? 0 : !strcmp(kind, "ammo") ? PROPDEF_AMMO : PROPDEF_COLLECTABLE;
    }
    if (!lanTestKind || lanTestTarget) return;
    PropDefHeaderRecord *cmd = g_CurrentSetup.propDefs;
    for (int index = 0; cmd && cmd->type != PROPDEF_END; ++index, cmd += sizepropdef(cmd)) {
        if (cmd->type != lanTestKind) continue;
        ObjectRecord *obj = (ObjectRecord *)cmd;
        if (!obj->prop || !(obj->state & PROPSTATE_RESPAWN)) continue;
        if (cmd->type == PROPDEF_COLLECTABLE && ((WeaponObjRecord *)obj)->weaponnum == ITEM_WPPK) continue;
        lanTestTarget = obj;
        lanTestCommand = index;
        lanTestSpawn = (LanPose){obj->runtime_pos.x, obj->runtime_pos.y, obj->runtime_pos.z};
        sysLogPrintf(LOG_INFO, "LAN pickup target %d type %d at %.1f %.1f %.1f",
                     index, cmd->type, lanTestSpawn.x, lanTestSpawn.feet, lanTestSpawn.z);
        break;
    }
}

int lanTestAllowPickup(PropRecord *prop)
{
    if (!lanTestKind || !lanTestTarget || prop != lanTestTarget->prop) return 1;
    for (int i = 0; i < 2; ++i) {
        struct player *p = g_playerPointers[i];
        if (!p || !p->prop || !xboxLives[i].loaded) return 0;
        float dx = p->prop->pos.x - lanTestSpawn.x;
        float dz = p->prop->pos.z - lanTestSpawn.z;
        if (dx*dx + dz*dz > 10000 || fabsf(p->prop->pos.y - lanTestSpawn.feet) > 200) return 0;
    }
    sysLogPrintf(LOG_INFO, "LAN contested pickup %d awarded to %d", lanTestCommand, get_cur_playernum());
    return 1;
}

static void lanTestPickupTrace(void)
{
    if (!lanTestTarget || !lanTestTarget->prop) return;
    static int previousHidden = -1;
    static unsigned previousInventory[2][3];
    static int recorded[2];
    int hidden = !!(lanTestTarget->runtime_bitflags & RUNTIMEBITFLAG_00000800);
    int slot = get_cur_playernum();
    if (hidden != previousHidden) {
        sysLogPrintf(LOG_INFO, "LAN local pickup %d hidden %d", lanTestCommand, hidden);
        previousHidden = hidden;
    }
    if (!xboxLives[slot].loaded) return;
    LanInventory inv = {0};
    lanReadInventory(g_CurrentPlayer, &inv);
    unsigned total = inv.magazine[0] + inv.magazine[1];
    for (int i = 0; i < LAN_AMMO_TYPES; ++i) total += inv.reserve[i];
    unsigned state[] = {inv.owned[0], inv.owned[1], total};
    if (!recorded[slot] || memcmp(previousInventory[slot], state, sizeof(state))) {
        sysLogPrintf(LOG_INFO, "LAN local inventory %d owned %08x %08x ammo %u",
                     slot, state[0], state[1], total);
        memcpy(previousInventory[slot], state, sizeof(state));
        recorded[slot] = 1;
    }
}
