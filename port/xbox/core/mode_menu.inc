/* Xbox mode-select rows use the downstream dossier layout and GE's own font,
 * cursor, selection sound and menu transitions. Included inside front.c. */
static const char *const xboxModeNames[] = {
    "COOP CAMPAIGN\n", "LAN MULTIPLAYER\n", "LOCAL MULTIPLAYER\n"
};
static const char *const xboxModeNumbers[] = {"1.\n", "2.\n", "3.\n"};
static const char *xboxModeMessage;
static int xboxModeMessageUntil;

#include "port/lan/lan.h"
void init_mp_options_for_scenario(s32 numplayers);
static int lanEditor, lanOctet = 3;
static int lanAddress[4] = {192, 168, 1, 1};
static int lanAttempted;

static void xboxLanEditorTick(void)
{
    unsigned keys = joyGetButtonsPressedThisFrame(PLAYER_1, ANY_BUTTON);
    if (keys & B_BUTTON) {
        lanClose();
        lanEditor = lanAttempted = 0;
        return;
    }
    if (lanActive()) {
        lanPoll();
        if (lanConnected()) {
            lanEditor = 0;
            gamemode = GAMEMODE_MULTI;
            init_mp_options_for_scenario(2);
            reset_mp_options_for_scenario(SCENARIO_NORMAL);
            player_char[0] = 1;
            player_char[1] = 0;
            MP_stage_selected = MP_STAGE_COMPLEX;
            selected_stage = multi_stage_setups[MP_stage_selected].stage_id;
            briefingpage = -1;
            frontChangeMenu(MENU_RUN_STAGE, TRUE);
        }
        return;
    }
    int step = (joyGetButtons(PLAYER_1, R_TRIG) ? 10 : 1);
    if (keys & L_JPAD) lanOctet = (lanOctet + 3) % 4;
    if (keys & R_JPAD) lanOctet = (lanOctet + 1) % 4;
    if (keys & U_JPAD) lanAddress[lanOctet] = (lanAddress[lanOctet]+step) % 256;
    if (keys & D_JPAD) lanAddress[lanOctet] = (lanAddress[lanOctet]+256-step) % 256;
    if (keys & (A_BUTTON | START_BUTTON)) {
        char ip[16];
        sprintf(ip, "%d.%d.%d.%d", lanAddress[0], lanAddress[1], lanAddress[2], lanAddress[3]);
        lanAttempted = 1;
        lanJoin(ip);
    }
}

static Gfx *xboxDrawLanEditor(Gfx *dl)
{
    char line[80];
    char field[4][8];
    for (int i=0; i<4; ++i)
        sprintf(field[i], i == lanOctet ? "[%d]" : "%d", lanAddress[i]);
    sprintf(line, "%s.%s.%s.%s\n", field[0],field[1],field[2],field[3]);
    const char *rows[] = {"WINDOWS HOST IP\n", line, "D-PAD EDIT   LT X10\n",
        "A JOIN   B BACK\n", lanAttempted ? lanStatus() : ""};
    for (int i=0; i<5; ++i) {
        int x=135, y=210+i*22;
        dl = frontPrintText(dl,&x,&y,(s8 *)rows[i],ptrFontZurichBoldChars,ptrFontZurichBold,
                            255,viGetX(),viGetY(),0,0);
    }
    return dl;
}

static void xboxChooseMode(void)
{
    int row = cursor_v_pos >= 275 ? 2 : cursor_v_pos >= 243 ? 1 : 0;
    mission_difficulty_highlighted = row;
    if (!joyGetButtonsPressedThisFrame(PLAYER_1, START_BUTTON | Z_TRIG | A_BUTTON))
        return;

    if (row == 1) {
        lanEditor = 1;
        lanAttempted = 0;
        return;
    }
    if (row == 2 && joyGetControllerCount() >= 2) {
        xboxModeMessage = NULL;
        gamemode = GAMEMODE_MULTI;
        sndPlaySfx(g_musicSfxBufferPtr, DOOR_METAL_CLOSE_SFX, 0);
    } else {
        xboxModeMessage = row == 2 ? "CONNECT A SECOND CONTROLLER\n" : "UNAVAILABLE ON XBOX\n";
        xboxModeMessageUntil = g_GlobalTimer + 120;
    }
}

static Gfx *xboxDrawModeRows(Gfx *dl)
{
    if (lanEditor) return xboxDrawLanEditor(dl);
    for (int row = 0; row < 3; ++row) {
        int x = 150, y = 220 + row * 32;
        int height, width;
        s8 *name = (s8 *)xboxModeNames[row];
        textMeasure(&height, &width, name, ptrFontZurichBoldChars, ptrFontZurichBold, 0);
        if (mission_difficulty_highlighted == row && !tab_prev_highlight)
            dl = microcode_constructor_related_to_menus(dl, 148, y - 2, 175 + width, y + 14, 50);
        dl = frontPrintText(dl, &x, &y, (s8 *)xboxModeNumbers[row], ptrFontZurichBoldChars,
                            ptrFontZurichBold, 255, viGetX(), viGetY(), 0, 0);
        x = 170;
        y = 220 + row * 32;
        dl = frontPrintText(dl, &x, &y, name, ptrFontZurichBoldChars, ptrFontZurichBold,
                            255, viGetX(), viGetY(), 0, 0);
    }
    if (xboxModeMessage && g_GlobalTimer < xboxModeMessageUntil) {
        int x = 70, y = 302;
        dl = frontPrintText(dl, &x, &y, (s8 *)xboxModeMessage, ptrFontZurichBoldChars,
                            ptrFontZurichBold, 255, viGetX(), viGetY(), 0, 0);
    }
    return dl;
}
