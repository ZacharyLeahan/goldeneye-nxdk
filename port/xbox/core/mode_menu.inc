/* Xbox mode-select rows use the downstream dossier layout and GE's own font,
 * cursor, selection sound and menu transitions. Included inside front.c. */
static const char *const xboxModeNames[] = {
    "COOP CAMPAIGN\n", "ONLINE MULTIPLAYER\n", "LOCAL MULTIPLAYER\n"
};
static const char *const xboxModeNumbers[] = {"1.\n", "2.\n", "3.\n"};
static const char *xboxModeMessage;
static int xboxModeMessageUntil;

static void xboxChooseMode(void)
{
    int row = cursor_v_pos >= 275 ? 2 : cursor_v_pos >= 243 ? 1 : 0;
    mission_difficulty_highlighted = row;
    if (!joyGetButtonsPressedThisFrame(PLAYER_1, START_BUTTON | Z_TRIG | A_BUTTON))
        return;

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
