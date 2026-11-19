/* Included by the downstream build adapter inside the native dossier menu. */
#include "port/lan/lan.h"
#ifdef GE_LAN_HOST
#include <string.h>
extern int lanDesktopOpen(const char *guestAddress);
extern void lanDesktopAddresses(char *text, size_t size);
#endif

void init_mp_options_for_scenario(s32 numplayers);

enum MultiplayerPage { DOSSIER, MULTIPLAYER, LAN_ROLES, JOIN_ADDRESS, HOST_WAIT };
static enum MultiplayerPage multiplayerPage;
static int addressOctet = 3;
static int hostAddress[4] = {192, 168, 1, 1};
static const char *modeMessage;
#ifdef GE_LAN_HOST
static char hostAddresses[512];
#endif

static void openMultiplayerPage(enum MultiplayerPage page)
{
    multiplayerPage = page;
    modeMessage = NULL;
    mission_difficulty_highlighted = 0;
    cursor_v_pos = 226.0f;
    cursor_h_pos = 170.0f;
}

static void initMultiplayerPage(void)
{
    lanClose();
#ifdef __XBOX__
    openMultiplayerPage(MULTIPLAYER);
#else
    multiplayerPage = DOSSIER;
    modeMessage = NULL;
#endif
}

static void multiplayerBack(void)
{
    lanClose();
    switch (multiplayerPage) {
    case MULTIPLAYER:
#ifdef __XBOX__
        frontChangeMenu(MENU_FILE_SELECT, FALSE);
#else
        openMultiplayerPage(DOSSIER);
        setCursorPOSforMode(GAMEMODE_MULTI);
#endif
        break;
    case JOIN_ADDRESS:
#ifdef __XBOX__
        openMultiplayerPage(MULTIPLAYER);
#else
        openMultiplayerPage(LAN_ROLES);
#endif
        break;
    case HOST_WAIT:
        openMultiplayerPage(LAN_ROLES);
        break;
    default:
        openMultiplayerPage(MULTIPLAYER);
        break;
    }
}

static void enterNetworkMatch(void)
{
    gamemode = GAMEMODE_MULTI;
    init_mp_options_for_scenario(2);
    reset_mp_options_for_scenario(SCENARIO_NORMAL);
    player_char[0] = lanIsHost() ? 0 : 1;
    player_char[1] = lanIsHost() ? 1 : 0;
    MP_stage_selected = lanArena();
    selected_stage = multi_stage_setups[MP_stage_selected].stage_id;
    briefingpage = -1;
    frontChangeMenu(MENU_RUN_STAGE, TRUE);
}

static void multiplayerMenuTick(void)
{
    unsigned keys = joyGetButtonsPressedThisFrame(PLAYER_1, ANY_BUTTON);
    int confirm = keys & (START_BUTTON | A_BUTTON | Z_TRIG);
    frontUpdateControlStickPosition();
    tab_prev_highlight = frontCheckCursorOnPreviousTab();
    if ((keys & B_BUTTON) || (confirm && tab_prev_highlight)) {
        multiplayerBack();
        return;
    }
    if (lanActive()) {
        lanPoll();
        if (lanConnected()) enterNetworkMatch();
        return;
    }
    if (multiplayerPage == JOIN_ADDRESS) {
        int step = (joyGetButtons(PLAYER_1, R_TRIG) ? 10 : 1);
        if (keys & L_JPAD) addressOctet = (addressOctet + 3) % 4;
        if (keys & R_JPAD) addressOctet = (addressOctet + 1) % 4;
        if (keys & U_JPAD) hostAddress[addressOctet] = (hostAddress[addressOctet] + step) % 256;
        if (keys & D_JPAD) hostAddress[addressOctet] = (hostAddress[addressOctet] + 256 - step) % 256;
        if (confirm) {
            char ip[16];
            sprintf(ip, "%d.%d.%d.%d", hostAddress[0], hostAddress[1], hostAddress[2], hostAddress[3]);
#ifdef GE_LAN_HOST
            if (!lanDesktopOpen(ip)) modeMessage = lanStatus();
#else
            if (!lanJoin(ip)) modeMessage = lanStatus();
#endif
        }
        return;
    }
    mission_difficulty_highlighted = cursor_v_pos >= 243.0f;
    if (!confirm) return;
    if (multiplayerPage == MULTIPLAYER) {
        if (mission_difficulty_highlighted) {
#ifdef __XBOX__
            openMultiplayerPage(JOIN_ADDRESS);
#else
            openMultiplayerPage(LAN_ROLES);
#endif
        } else if (joyGetControllerCount() >= 2) {
            gamemode = GAMEMODE_MULTI;
            sndPlaySfx(g_musicSfxBufferPtr, DOOR_METAL_CLOSE_SFX, 0);
            frontChangeMenu(MENU_MP_OPTIONS, FALSE);
        } else {
            modeMessage = "CONNECT A SECOND CONTROLLER\n";
        }
    }
#ifdef GE_LAN_HOST
    else if (multiplayerPage == LAN_ROLES) {
        if (mission_difficulty_highlighted) {
            openMultiplayerPage(JOIN_ADDRESS);
        } else if (lanDesktopOpen(NULL)) {
            openMultiplayerPage(HOST_WAIT);
            lanDesktopAddresses(hostAddresses, sizeof(hostAddresses));
        } else {
            modeMessage = lanStatus();
        }
    }
#endif
}

static Gfx *multiplayerLine(Gfx *dl, int x, int y, const char *text)
{
    return frontPrintText(dl, &x, &y, (s8 *)text, ptrFontZurichBoldChars,
                          ptrFontZurichBold, 255, viGetX(), viGetY(), 0, 0);
}

static Gfx *drawMultiplayerPage(Gfx *dl)
{
    if (multiplayerPage == JOIN_ADDRESS) {
        char field[4][8], address[40];
        for (int i = 0; i < 4; ++i)
            sprintf(field[i], i == addressOctet ? "[%d]" : "%d", hostAddress[i]);
        sprintf(address, "%s.%s.%s.%s\n", field[0], field[1], field[2], field[3]);
        dl = multiplayerLine(dl, 135, 210, "WINDOWS HOST IP\n");
        dl = multiplayerLine(dl, 135, 232, address);
        dl = multiplayerLine(dl, 135, 254, "D-PAD EDIT   LT X10\n");
        dl = multiplayerLine(dl, 135, 276, "A JOIN   B BACK\n");
    }
#ifdef GE_LAN_HOST
    else if (multiplayerPage == HOST_WAIT) {
        dl = multiplayerLine(dl, 135, 210, "WAITING FOR PLAYER\n");
        // Each interface is a separate line in the existing desktop host UI.
        const char *p = hostAddresses;
        for (int y = 232; *p && y <= 298; y += 22) {
            char line[64];
            size_t n = strcspn(p, "\r\n");
            snprintf(line, sizeof(line), "%.*s\n", (int)n, p);
            dl = multiplayerLine(dl, 135, y, line);
            p += n + strspn(p + n, "\r\n");
        }
        if (!*hostAddresses) dl = multiplayerLine(dl, 135, 232, "CHECK NETWORK CONNECTION\n");
        return dl;
    }
#endif
    else {
        static const char *const modes[] = {"LOCAL MULTIPLAYER\n", "LAN MULTIPLAYER\n"};
        static const char *const roles[] = {"HOST MATCH\n", "JOIN MATCH\n"};
        const char *const *rows = multiplayerPage == MULTIPLAYER ? modes : roles;
        for (int i = 0; i < 2; ++i) {
            int y = 220 + i * 32, height, width;
            textMeasure(&height, &width, (s8 *)rows[i], ptrFontZurichBoldChars, ptrFontZurichBold, 0);
            if (i == mission_difficulty_highlighted && !tab_prev_highlight)
                dl = microcode_constructor_related_to_menus(dl, 148, y-2, 175+width, y+14, 50);
            dl = multiplayerLine(dl, 150, y, i ? "2.\n" : "1.\n");
            dl = multiplayerLine(dl, 170, y, rows[i]);
        }
    }
    if (modeMessage || lanActive())
        dl = multiplayerLine(dl, 100, 302, modeMessage ? modeMessage : lanStatus());
    return dl;
}
