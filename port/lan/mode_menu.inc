/* Included by the downstream build adapter inside the native dossier menu. */
#include "port/lan/lan.h"
#ifdef GE_LAN_HOST
#include <string.h>
extern int lanDesktopOpen(const char *guestAddress);
extern void lanDesktopAddresses(char *text, size_t size);
#endif

void init_mp_options_for_scenario(s32 numplayers);

enum MultiplayerPage { DOSSIER, MULTIPLAYER, LAN_ROLES, JOIN_LIST, JOIN_ADDRESS, HOST_WAIT };
static enum MultiplayerPage multiplayerPage;
static int addressOctet = 3;
static int hostAddress[4] = {192, 168, 1, 1};
static const char *modeMessage;
static char hostAddresses[512];
static int discoverySelection;

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
    openMultiplayerPage(MULTIPLAYER);
}

static void multiplayerBack(void)
{
    lanClose();
    switch (multiplayerPage) {
    case MULTIPLAYER:
        frontChangeMenu(MENU_FILE_SELECT, FALSE);
        break;
    case JOIN_ADDRESS:
    case JOIN_LIST:
        openMultiplayerPage(LAN_ROLES);
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

static void enterLocalMatchSetup(void)
{
    s32 players = joyGetControllerCount();

    /* The native options page only initializes its player count when the
     * previous selection is zero.  LAN always selects two players, so coming
     * back from LAN (or another two-player setup) could otherwise leave local
     * play at two even when all four Xbox ports are populated.  Re-seed the
     * local setup from the connected pads every time this row is entered.
     * Keep the original game's supported 2-4 player range explicit here. */
    if (players < 2) players = 2;
    if (players > MAXCONTROLLERS) players = MAXCONTROLLERS;

    lanClose();
    gamemode = GAMEMODE_MULTI;
    init_mp_options_for_scenario(players);
    sndPlaySfx(g_musicSfxBufferPtr, DOOR_METAL_CLOSE_SFX, 0);
    frontChangeMenu(MENU_MP_OPTIONS, FALSE);
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
    if (lanActive() && !lanDiscovering()) {
        lanPoll();
        if (lanConnected()) enterNetworkMatch();
        return;
    }
    if (multiplayerPage == JOIN_LIST) {
        lanPoll();
        int count = lanDiscoveryCount();
        int rows = count + 2;
        if (discoverySelection >= rows) discoverySelection = rows - 1;
        if (keys & U_JPAD) discoverySelection = (discoverySelection + rows - 1) % rows;
        if (keys & D_JPAD) discoverySelection = (discoverySelection + 1) % rows;
        if (confirm) {
            if (discoverySelection < count) {
                if (!lanJoinDiscovered(discoverySelection)) modeMessage = lanStatus();
            } else if (discoverySelection == count) {
                lanDiscoverRefresh();
            } else {
                lanClose();
                openMultiplayerPage(JOIN_ADDRESS);
            }
        }
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
    if (multiplayerPage == MULTIPLAYER && cursor_v_pos >= 275.0f)
        mission_difficulty_highlighted = 2;
    if (!confirm) return;
    if (multiplayerPage == MULTIPLAYER) {
        if (mission_difficulty_highlighted == 0) {
            gamemode = GAMEMODE_SOLO;
            sndPlaySfx(g_musicSfxBufferPtr, DOOR_METAL_CLOSE_SFX, 0);
            frontChangeMenu(MENU_MISSION_SELECT, FALSE);
            set_cursor_to_stage_solo(0);
            return;
        }
        if (mission_difficulty_highlighted == 1) {
            openMultiplayerPage(LAN_ROLES);
        } else if (joyGetControllerCount() >= 2) {
            enterLocalMatchSetup();
        } else {
            modeMessage = "CONNECT A SECOND CONTROLLER\n";
        }
    }
    else if (multiplayerPage == LAN_ROLES) {
        if (mission_difficulty_highlighted) {
            openMultiplayerPage(JOIN_LIST);
            discoverySelection = 0;
            if (!lanDiscover()) modeMessage = lanStatus();
        }
#ifdef GE_LAN_HOST
        else if (lanDesktopOpen(NULL)) {
            openMultiplayerPage(HOST_WAIT);
            lanDesktopAddresses(hostAddresses, sizeof(hostAddresses));
        }
#else
        else if (lanHost()) {
            openMultiplayerPage(HOST_WAIT);
            lanLocalAddress(hostAddresses, sizeof(hostAddresses));
        }
#endif
        else modeMessage = lanStatus();
    }
}

static Gfx *multiplayerLine(Gfx *dl, int x, int y, const char *text)
{
    return frontPrintText(dl, &x, &y, (s8 *)text, ptrFontZurichBoldChars,
                          ptrFontZurichBold, 255, viGetX(), viGetY(), 0, 0);
}

static Gfx *drawMultiplayerPage(Gfx *dl)
{
    if (multiplayerPage == JOIN_LIST) {
        int count = lanDiscoveryCount();
        int first = 0;
        if (count > 3) {
            if (discoverySelection < count) first = discoverySelection > 1 ? discoverySelection - 1 : 0;
            else first = count - 3;
            if (first > count - 3) first = count - 3;
        }
        dl = multiplayerLine(dl, 110, 188, "AVAILABLE LAN MATCHES\n");
        if (!count) dl = multiplayerLine(dl, 135, 220, "NO MATCHES FOUND\n");
        for (int row = 0; row < 3 && first + row < count; ++row) {
            int index = first + row, y = 212 + row * 22;
            const LanMatch *match = lanDiscoveryMatch(index);
            char line[96];
            snprintf(line, sizeof(line), "%s  %s  %d/%d\n", match->name,
                     lanArenaName(match->arena), match->players, match->capacity);
            if (index == discoverySelection && !tab_prev_highlight)
                dl = microcode_constructor_related_to_menus(dl, 108, y-2, 510, y+14, 50);
            dl = multiplayerLine(dl, 112, y, line);
        }
        static const char *const actions[] = {"REFRESH\n", "ENTER IP MANUALLY\n"};
        for (int action = 0; action < 2; ++action) {
            int index = count + action, y = 278 + action * 22;
            if (index == discoverySelection && !tab_prev_highlight)
                dl = microcode_constructor_related_to_menus(dl, 108, y-2, 310, y+14, 50);
            dl = multiplayerLine(dl, 112, y, actions[action]);
        }
        if (discoverySelection < count) {
            const LanMatch *match = lanDiscoveryMatch(discoverySelection);
            char endpoint[48];
            snprintf(endpoint, sizeof(endpoint), "%s:%u\n", match->address, (unsigned)match->port);
            dl = multiplayerLine(dl, 112, 322, endpoint);
        } else {
            dl = multiplayerLine(dl, 112, 322, lanStatus());
        }
        return dl;
    }
    else if (multiplayerPage == JOIN_ADDRESS) {
        char field[4][8], address[40];
        for (int i = 0; i < 4; ++i)
            sprintf(field[i], i == addressOctet ? "[%d]" : "%d", hostAddress[i]);
        sprintf(address, "%s.%s.%s.%s\n", field[0], field[1], field[2], field[3]);
        dl = multiplayerLine(dl, 135, 210, "HOST IP\n");
        dl = multiplayerLine(dl, 135, 232, address);
        dl = multiplayerLine(dl, 135, 254, "D-PAD EDIT   LT X10\n");
        dl = multiplayerLine(dl, 135, 276, "A JOIN   B BACK\n");
    }
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
    else {
        static const char *const modes[] = {"SOLO CAMPAIGN\n", "LAN MULTIPLAYER\n", "LOCAL MULTIPLAYER\n"};
        static const char *const roles[] = {"HOST MATCH\n", "JOIN MATCH\n"};
        const char *const *rows = multiplayerPage == MULTIPLAYER ? modes : roles;
        static const char *const numbers[] = {"1.\n", "2.\n", "3.\n"};
        int count = multiplayerPage == MULTIPLAYER ? sizeof(modes) / sizeof(modes[0]) : 2;
        for (int i = 0; i < count; ++i) {
            int y = 220 + i * 32, height, width;
            textMeasure(&height, &width, (s8 *)rows[i], ptrFontZurichBoldChars, ptrFontZurichBold, 0);
            if (i == mission_difficulty_highlighted && !tab_prev_highlight)
                dl = microcode_constructor_related_to_menus(dl, 148, y-2, 175+width, y+14, 50);
            dl = multiplayerLine(dl, 150, y, numbers[i]);
            dl = multiplayerLine(dl, 170, y, rows[i]);
        }
    }
    if (modeMessage || lanActive())
        dl = multiplayerLine(dl, 100, 302, modeMessage ? modeMessage : lanStatus());
    return dl;
}
