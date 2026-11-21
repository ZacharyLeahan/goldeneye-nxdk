/* Included in the native player's translation unit. Setup command numbers
 * identify authored items across the two native pointer representations. */
void objFree(ObjectRecord *obj, s32 freeprop, s32 canregen);
InvItem *bondinvGetDualWeapon(ITEM_IDS right, ITEM_IDS left);
void bondinvRemoveItem(InvItem *item);

static int lanPickupType(int type)
{
    return type == PROPDEF_COLLECTABLE || type == PROPDEF_AMMO ||
           type == PROPDEF_MAGAZINE || type == PROPDEF_ARMOUR;
}

int lanHoldPickupTimer(PropRecord *prop)
{
    return lanActive() && !lanIsHost() && prop->timetoregen > CHROBJ_TIMETOREGEN &&
           (prop->obj->state & PROPSTATE_RESPAWN) && lanPickupType(prop->obj->type);
}

static void lanPickupPage(LanPose *state)
{
    static unsigned nextCommand;
    PropDefHeaderRecord *cmd = (void *)setupGetPtrToCommandByIndex(nextCommand);
    if (!cmd) {
        nextCommand = 0;
        cmd = g_CurrentSetup.propDefs;
    }
    state->firstCommand = nextCommand;
    while (cmd && cmd->type != PROPDEF_END && state->commands < 32 * LAN_WORLD_WORDS) {
        unsigned bit = state->commands++;
        if (lanPickupType(cmd->type)) {
            ObjectRecord *obj = (ObjectRecord *)cmd;
            if (!obj->prop || (obj->runtime_bitflags & RUNTIMEBITFLAG_00000800))
                state->hidden[bit / 32] |= 1u << (bit % 32);
        }
        cmd += sizepropdef(cmd);
    }
    nextCommand = cmd && cmd->type != PROPDEF_END ? nextCommand + state->commands : 0;
}

static void lanApplyPickups(const LanPose *state)
{
    PropDefHeaderRecord *cmd = (void *)setupGetPtrToCommandByIndex(state->firstCommand);
    for (unsigned i = 0; cmd && cmd->type != PROPDEF_END && i < state->commands; ++i) {
        ObjectRecord *obj = (ObjectRecord *)cmd;
        if (lanPickupType(cmd->type) && obj->prop) {
            int hidden = (state->hidden[i / 32] >> (i % 32)) & 1u;
            if (hidden && !(obj->runtime_bitflags & RUNTIMEBITFLAG_00000800)) {
                if (obj->state & PROPSTATE_RESPAWN)
                    propExecuteTickOperation(obj->prop, TICKOP_FREE);
                else
                    objFree(obj, 1, 0);
            } else if (!hidden && lanHoldPickupTimer(obj->prop)) {
                /* Cross the native re-enable threshold on its next tick. */
                obj->prop->timetoregen = CHROBJ_TIMETOREGEN;
            }
        }
        cmd += sizepropdef(cmd);
    }
}

static QuickSlap *lanActiveSlap(struct player *p)
{
    for (int slot = 0; slot < 2; ++slot)
        if (xboxLives[slot].owner == p && xboxLives[slot].slap.phase)
            return &xboxLives[slot].slap;
    return NULL;
}

static void lanReadInventory(struct player *p, LanInventory *inv)
{
    _Static_assert(sizeof(p->ammoheldarr) == sizeof(inv->reserve), "reserve ammo layout");
    _Static_assert(ITEM_TANKSHELLS < LAN_WEAPON_WORDS * 32, "multiplayer weapon IDs");
    QuickSlap *slap = lanActiveSlap(p);
    inv->ready = 1;
    InvItem *first = p->ptr_inventory_first_in_cycle;
    if (first) {
        InvItem *item = first;
        do {
            unsigned id = ITEM_NOTHING;
            uint32_t *mask = inv->owned;
            if (item->type == INV_ITEM_WEAPON) {
                id = item->type_inv_item.type_weap.weapon;
            } else if (item->type == INV_ITEM_DUAL &&
                       item->type_inv_item.type_dual.weapon_left == item->type_inv_item.type_dual.weapon_right) {
                id = item->type_inv_item.type_dual.weapon_right;
                mask = inv->paired;
            }
            if (id <= ITEM_TANKSHELLS) mask[id / 32] |= 1u << (id % 32);
            item = item->next;
        } while (item && item != first);
    }
    for (int i = 0; i < LAN_AMMO_TYPES; ++i)
        inv->reserve[i] = p->ammoheldarr[i] > 0 ? p->ammoheldarr[i] : 0;
    for (int hand = 0; hand < 2; ++hand) {
        int id = slap ? slap->weapon[hand] : p->hands[hand].weaponnum;
        inv->weapon[hand] = id >= 0 ? id : 255;
        int mag = slap ? slap->magazine[hand] : p->hands[hand].weapon_ammo_in_magazine;
        inv->magazine[hand] = mag > 0 ? mag : 0;
    }
}

static void lanApplyInventory(struct player *p, const LanInventory *inv)
{
    QuickSlap *slap = lanActiveSlap(p);
    LanInventory local = {0};
    lanReadInventory(p, &local);
    for (int id = ITEM_KNIFE; id <= ITEM_TANKSHELLS; ++id) {
        unsigned word = id / 32;
        uint32_t mask = 1u << (id % 32);
        if (local.owned[word] & ~inv->owned[word] & mask) bondinvRemoveItemByID(id);
        if (local.paired[word] & ~inv->paired[word] & mask) bondinvRemoveItem(bondinvGetDualWeapon(id, id));
        if (inv->owned[word] & ~local.owned[word] & mask) bondinvAddInvItem(id);
        if (inv->paired[word] & ~local.paired[word] & mask) bondinvAddDoublesInvItem(id, id);
    }
    for (int i = 0; i < LAN_AMMO_TYPES; ++i) p->ammoheldarr[i] = inv->reserve[i];
    for (int hand = 0; hand < 2; ++hand) {
        if (slap && (unsigned)slap->weapon[hand] == inv->weapon[hand])
            slap->magazine[hand] = inv->magazine[hand];
        else if (!slap && (unsigned)p->hands[hand].weaponnum == inv->weapon[hand])
            p->hands[hand].weapon_ammo_in_magazine = inv->magazine[hand];
    }
}
