/* Included in the generated bondview2 translation unit on both LAN peers.
 * The host confirms damage. Each local viewport tracks its own opponents. */
enum { HIT_HOLD_TICKS = 195, HIT_FADE_TICKS = 12, HIT_TOTAL_TICKS = 207 };
typedef struct {
    int expires;
    float previousHealth;
} TargetHit;
static TargetHit targetHits[MAXCONTROLLERS][MAXCONTROLLERS];

void mpClearTargetHits(void)
{
    memset(targetHits, 0, sizeof(targetHits));
}

static void mpClearSeatHits(int slot)
{
    for (int i = 0; i < MAXCONTROLLERS; ++i) {
        targetHits[i][slot].expires = 0;
        targetHits[slot][i].expires = 0;
    }
}

static void mpConfirmTargetHit(int attacker, int target)
{
    struct player *p = g_CurrentPlayer;
    if (getPlayerCount() < 2 || (unsigned)attacker >= getPlayerCount() || attacker == target)
        return;
    if (p->bondhealth >= p->oldhealth && p->bondarmour >= p->oldarmour) return;
    TargetHit *hit = &targetHits[attacker][target];
    hit->expires = g_GlobalTimer + HIT_TOTAL_TICKS;
    hit->previousHealth = p->oldhealth;
}

static Gfx *hitCells(Gfx *dl, int x, int y, int step, int height, float fraction)
{
    for (int i = 0; i < 8; ++i) {
        float fill = fminf(1, fraction * 8 - i);
        if (fill <= 0) break;
        int width = (int)((step - 1) * fill + .5f);
        if (width > 0) { gDPFillRectangle(dl++, x + i*step, y, x + i*step + width, y + height); }
    }
    return dl;
}

extern s32 chrlvCurrentPlayerCall7F0B0E24(ChrRecord *chr);
Gfx *mpDrawTargetHits(Gfx *dl)
{
    int count = getPlayerCount(), viewer = get_cur_playernum();
    if (count < 2 || (lanActive() && viewer != 0) || g_CurrentPlayer->bonddead ||
        g_CurrentPlayer->mpmenuon || g_gameOverFlag || g_stopPlayFlag) return dl;
    for (int i = 0; i < count; ++i) {
        const TargetHit *hit = &targetHits[viewer][i];
        int left = hit->expires - g_GlobalTimer;
        if (left <= 0 || left > HIT_TOTAL_TICKS) continue;
        struct player *p = g_playerPointers[i];
        if (!p || !p->prop || !p->prop->chr || p->bonddead ||
            !(p->prop->flags & PROPFLAG_ONSCREEN) || !chrlvCurrentPlayerCall7F0B0E24(p->prop->chr)) continue;
        coord3d head = p->field_488.collision_position;
        head.y += 30;
        mtx4TransformVecInPlace(camGetWorldToScreenMtxf(), &head);
        if (head.z >= -1) continue;
        coord2d xy;
        transform3Dto2DCoords(&head, &xy);
        int vx = getPlayer_c_screenleft(), vy = getPlayer_c_screentop();
        int vw = getPlayer_c_screenwidth(), vh = getPlayer_c_screenheight();
        if (!(xy.x >= vx && xy.x < vx + vw && xy.y < vy + vh)) continue;
        int step = vw >= 300 ? 5 : 3;
        int width = 8 * step, height = vh >= 180 ? 4 : 3;
        int x = fmaxf(vx+2, fminf(xy.x-width/2, vx+vw-width-2));
        int y = fmaxf(vy+6, fminf(xy.y, vy+vh-height-2));
        float fade = fminf(1, left / (float)HIT_FADE_TICKS);
        int alpha = (int)(255 * fade * fade);
        float hp = fmaxf(0, fminf(1, p->bondhealth));
        float age = (HIT_TOTAL_TICKS-left) / 60.f;
        float trail = fmaxf(hp, hit->previousHealth - 1.8f*fmaxf(0,age-.16f));

        gDPPipeSync(dl++);
        gDPSetCycleType(dl++, G_CYC_1CYCLE);
        gDPSetRenderMode(dl++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetCombineMode(dl++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        gDPSetPrimColor(dl++, 0, 0, 25, 35, 22, alpha);
        gDPFillRectangle(dl++, x-1, y-1, x+width, y+height+1);
        gDPSetPrimColor(dl++, 0, 0, 199, 171, 99, alpha);
        dl = hitCells(dl,x,y,step,height,trail);
        gDPSetPrimColor(dl++, 0, 0, hp>.5f?71:210, hp>.5f?168:(hp>.25f?163:46), hp>.5f?56:31, alpha);
        dl = hitCells(dl,x,y,step,height,hp);
        if (p->bondarmour > 0) {
            gDPSetPrimColor(dl++, 0, 0, 77, 145, 201, alpha);
            dl = hitCells(dl,x,y-4,step,2,p->bondarmour);
        }
        gDPPipeSync(dl++);
    }
    return dl;
}
