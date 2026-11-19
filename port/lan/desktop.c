#include <winsock2.h>
#include <ws2tcpip.h>
/* The decomp stdarg shim imports a function-like BOOL macro. */
#undef BOOL
#include <iphlpapi.h>
#include <netioapi.h>
#include <wchar.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include "system.h"
#include "lan.h"

/* Read the PC's assigned addresses without contacting an Internet service.
 * Show both physical links when they are up: only the person joining knows
 * which of two independent LANs their console is on. */
void lanDesktopAddresses(char *text, size_t size)
{
    MIB_UNICASTIPADDRESS_TABLE *table = NULL;
    text[0] = '\0';
    if (GetUnicastIpAddressTable(AF_INET, &table) != NO_ERROR) return;
    size_t used = 0;
    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const MIB_UNICASTIPADDRESS_ROW *address = &table->Table[i];
        if (address->DadState != IpDadStatePreferred) continue;
        MIB_IF_ROW2 link = {0};
        link.InterfaceLuid = address->InterfaceLuid;
        if (GetIfEntry2(&link) != NO_ERROR || link.OperStatus != IfOperStatusUp ||
            !link.InterfaceAndOperStatusFlags.HardwareInterface) continue;
        /* Npcap and KM-TEST identify themselves as physical Ethernet even
         * though their loopback addresses are reachable only on this PC. */
        if (link.AccessType == NET_IF_ACCESS_LOOPBACK || wcsstr(link.Description, L"Loopback")) continue;
        const char *label;
        if (link.Type == IF_TYPE_ETHERNET_CSMACD) label = "Ethernet";
        else if (link.Type == IF_TYPE_IEEE80211) label = "Wi-Fi";
        else continue;
        char ip[INET_ADDRSTRLEN];
        if (!inet_ntop(AF_INET, &address->Address.Ipv4.sin_addr, ip, sizeof(ip))) continue;
        char entry[48];
        SDL_snprintf(entry, sizeof(entry), "%s%s  (%s)", used ? "\r\n" : "", ip, label);
        size_t length = SDL_strlen(entry);
        if (used + length >= size) break;
        SDL_memcpy(text + used, entry, length + 1);
        used += length;
    }
    FreeMibTable(table);
}

int lanDesktopOpen(const char *guestAddress)
{
    long port = LAN_PORT;
    if (sysArgCheck("--lan-port")) {
        const char *arg = sysArgGetString("--lan-port");
        char *end = NULL;
        port = arg ? strtol(arg, &end, 10) : 0;
        if (!end || *end || port < 1 || port > 65535) {
            sysLogPrintf(LOG_ERROR, "--lan-port requires a UDP port from 1 to 65535");
            return 0;
        }
    }
    return guestAddress ? lanJoinOnPort(guestAddress, (uint16_t)port) : lanHostOnPort((uint16_t)port);
}

int lanDesktopStart(void)
{
    const int host = sysArgCheck("--lan-host");
    const char *address = sysArgGetString("--lan-join");
    if (sysArgCheck("--lan-score-limit")) {
        const char *value = sysArgGetString("--lan-score-limit");
        char *end = NULL;
        long limit = value ? strtol(value, &end, 10) : -1;
        if (!host || !value || end == value || *end || limit < 0 || limit > 100 ||
            !lanSetScoreLimit((int)limit)) {
            sysLogPrintf(LOG_ERROR, "--lan-score-limit requires 0 (unlimited) or 1 to 100 on the host");
            return 0;
        }
    }
    if (sysArgCheck("--lan-map")) {
        const char *name = sysArgGetString("--lan-map");
        int id = 1;
        while (id <= LAN_ARENAS && name && SDL_strcasecmp(lanArenaName(id), name)) ++id;
        if (!host || !name || !lanSetArena(id)) {
            sysLogPrintf(LOG_ERROR, "--lan-map requires a retail arena name on the Windows host");
            return 0;
        }
    }
    if ((host && sysArgCheck("--lan-join")) || (!host && !address)) {
        sysLogPrintf(LOG_ERROR, "Choose --lan-host or --lan-join IPv4");
        return 0;
    }
    if (!lanDesktopOpen(host ? NULL : address)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "GoldenEye LAN", lanStatus(), NULL);
        return 0;
    }
    sysLogPrintf(LOG_INFO, "LAN %s on UDP %s", host ? "host" : address,
                 sysArgCheck("--lan-port") ? sysArgGetString("--lan-port") : "27007");
    return 1;
}

int lanDesktopWait(void)
{
    SDL_Window *window = SDL_GetWindowFromID(1);
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    HWND label = NULL;
    HFONT font = NULL;
    if (window && SDL_GetWindowWMInfo(window, &info)) {
        label = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                                0, 0, 0, 0, info.info.win.window, NULL, NULL, NULL);
        font = CreateFontW(-26, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                           DEFAULT_PITCH, L"Segoe UI");
        if (label && font) SendMessageW(label, WM_SETFONT, (WPARAM)font, FALSE);
    }
    char addresses[512] = "", message[640], title[640] = "GoldenEye LAN";
    Uint32 refresh = SDL_GetTicks();
    int width = -1, height = -1, lines = 0;
    while (!lanConnected()) {
        Uint32 now = SDL_GetTicks();
        if (SDL_TICKS_PASSED(now, refresh)) {
            char current[sizeof(addresses)];
            if (lanIsHost()) lanDesktopAddresses(current, sizeof(current));
            else SDL_strlcpy(current, sysArgGetString("--lan-join"), sizeof(current));
            if (!lines || SDL_strcmp(current, addresses)) {
                SDL_strlcpy(addresses, current, sizeof(addresses));
                SDL_snprintf(message, sizeof(message), "%s\r\n%s",
                             !lanIsHost() ? "Joining Windows host" :
                             addresses[0] ? "Enter this address on the joining device" : "Waiting for a LAN address",
                             addresses[0] ? addresses : "Connect this PC to Ethernet or Wi-Fi.");
                if (label) SetWindowTextA(label, message);
                SDL_snprintf(title, sizeof(title), "GoldenEye LAN - %s",
                             addresses[0] ? addresses : "Waiting for a LAN address");
                for (char *c = title; *c; ++c) if (*c == '\r' || *c == '\n') *c = ' ';
                if (window) SDL_SetWindowTitle(window, title);
                sysLogPrintf(LOG_INFO, "%s", title);
                lines = 1;
                for (const char *c = message; *c; ++c) if (*c == '\n') ++lines;
                width = -1;
            }
            refresh = now + 2000;
        }
        if (label) {
            RECT area;
            GetClientRect(info.info.win.window, &area);
            if (width != area.right || height != area.bottom) {
                width = area.right;
                height = area.bottom;
                int boxHeight = 40 * lines;
                MoveWindow(label, 0, SDL_max(0, (height - boxHeight) / 2),
                           width, boxHeight, TRUE);
            }
        }
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE)) {
                lanClose();
                goto done;
            }
        }
        lanPoll();
        SDL_Delay(10);
    }
done:
    if (label) DestroyWindow(label);
    if (font) DeleteObject(font);
    if (!lanConnected()) return 0;
    if (window) {
        SDL_strlcat(title, " - ", sizeof(title));
        SDL_strlcat(title, lanArenaName(lanArena()), sizeof(title));
        SDL_SetWindowTitle(window, title);
    }
    sysLogPrintf(LOG_INFO, "LAN match connected as %s", lanIsHost() ? "host" : "client");
    return 1;
}
