/* One direct IPv4 peer, no directory service. Wire fields are 32-bit words in
 * network order. A boot nonce on each end excludes stale match datagrams. */
#ifdef __XBOX__
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <lwip/netif.h>
#include <nxdk/net.h>
typedef int LanSocket;
#define INVALID_SOCKET (-1)
#define CLOSE_SOCKET lwip_close
#elif defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include "system.h"
typedef SOCKET LanSocket;
#define CLOSE_SOCKET closesocket
#define LAN_LOG(...) sysLogPrintf(LOG_INFO, "LAN: " __VA_ARGS__)
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
typedef int LanSocket;
#define INVALID_SOCKET (-1)
#define CLOSE_SOCKET close
#endif
#ifndef LAN_LOG
#define LAN_LOG(...) ((void)0)
#endif
#include <SDL_timer.h>
#include <string.h>
#include "lan.h"

enum { HELLO = 1, WELCOME, POSITION, DISCOVER, ADVERTISE, HEADER_WORDS = 9,
       ADVERTISE_NAME_WORDS = 8, ADVERTISE_WORDS = HEADER_WORDS + ADVERTISE_NAME_WORDS,
       INPUT_WORDS = HEADER_WORDS + __builtin_offsetof(LanPose, seats) / sizeof(uint32_t),
       WORDS = HEADER_WORDS + sizeof(LanPose) / sizeof(uint32_t) };
static const uint32_t signature = 0x47454c31; /* GEL1, NTSC-U */
#ifndef __XBOX__
static const char discoveryGroup[] = "239.71.69.49";
#endif
static int arena = LAN_COMPLEX;
static int scoreLimit = 5;
static uint32_t roundId;
int lanSetScoreLimit(int limit)
{
    if (limit < 0 || limit > 100 || lanConnected()) return 0;
    scoreLimit = limit;
    return 1;
}
int lanScoreLimit(void) { return scoreLimit; }
uint32_t lanRound(void) { return roundId; }
static const char *const arenas[] = {"", "Temple", "Complex", "Caves", "Library",
    "Basement", "Stack", "Facility", "Bunker", "Archives", "Caverns", "Egypt"};
int lanArena(void) { return arena; }
const char *lanArenaName(int id) { return id >= 1 && id <= LAN_ARENAS ? arenas[id] : ""; }
int lanSetArena(int id)
{
    if (id < 1 || id > LAN_ARENAS || lanConnected()) return 0;
    arena = id;
    return 1;
}
static LanSocket sock = INVALID_SOCKET;
static struct sockaddr_in peer;
static uint32_t nonce, other, seq, lastSeq, lastRx, lastTx, lastHello;
static int hosting, discovering, havePose, netReady;
static uint32_t discoveryStarted;
static LanMatch matches[LAN_DISCOVERY_MAX];
static uint32_t matchSeen[LAN_DISCOVERY_MAX];
static int matchCount;
static LanPose remote;
uint32_t lanPeerId(void) { return other; }
void lanBeginRematch(void)
{
    if (!hosting) return;
    if (++roundId == 0) roundId = 1;
    havePose = 0;
}
static const char *status = "Offline";

void lanClose(void)
{
    if (sock != INVALID_SOCKET) CLOSE_SOCKET(sock);
    sock = INVALID_SOCKET;
    other = seq = lastSeq = 0;
    havePose = hosting = discovering = matchCount = 0;
    status = "Offline";
}
int lanActive(void) { return sock != INVALID_SOCKET; }
int lanIsHost(void) { return hosting; }
int lanConnected(void) { return lanActive() && other != 0; }
int lanDiscovering(void) { return lanActive() && discovering; }
const char *lanStatus(void) { return status; }

static int openSocket(void)
{
    lanClose();
    if (!netReady) {
#ifdef __XBOX__
        if (nxNetInit(NULL) < 0) { status = "Check Xbox network settings"; return 0; }
#elif defined(_WIN32)
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa)) { status = "Winsock initialization failed"; return 0; }
#endif
        netReady = 1;
    }
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) { status = "Cannot open LAN socket"; return 0; }
#ifdef __XBOX__
    unsigned long on = 1;
    int failed = lwip_ioctl(sock, FIONBIO, &on);
#elif defined(_WIN32)
    u_long on = 1;
    int failed = ioctlsocket(sock, FIONBIO, &on);
#else
    int failed = fcntl(sock, F_SETFL, O_NONBLOCK);
#endif
    if (failed) { lanClose(); status = "Cannot configure LAN socket"; return 0; }
    nonce += SDL_GetPerformanceCounter() + 1u;
    if (!nonce) nonce = 1;
    lastHello = SDL_GetTicks() - 500u;
    memset(&peer, 0, sizeof(peer));
    peer.sin_family = AF_INET;
    return 1;
}

int lanHost(void)
{
    return lanHostOnPort(LAN_PORT);
}
int lanHostOnPort(uint16_t port)
{
    if (!openSocket()) return 0;
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))) {
        lanClose(); status = "Cannot bind the LAN host UDP port"; return 0;
    }
#ifndef __XBOX__
    struct ip_mreq membership = {0};
    membership.imr_multiaddr.s_addr = inet_addr(discoveryGroup);
    membership.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   (const char *)&membership, sizeof(membership)))
        LAN_LOG("multicast discovery unavailable; broadcasts remain enabled");
#endif
    hosting = 1;
    roundId = nonce;
    status = "Waiting for player";
    LAN_LOG("host ready on UDP %d build=%08x", port, (unsigned)LAN_BUILD_ID);
    return 1;
}
int lanLocalAddress(char *text, size_t size)
{
    if (!text || !size) return 0;
    text[0] = '\0';
#ifdef __XBOX__
    if (netif_default && netif_is_up(netif_default) &&
        !ip4_addr_isany_val(*netif_ip4_addr(netif_default)) &&
        ip4addr_ntoa_r(netif_ip4_addr(netif_default), text, (int)size))
        return 1;
#endif
    return 0;
}
int lanJoin(const char *ipv4)
{
    return lanJoinOnPort(ipv4, LAN_PORT);
}
int lanJoinOnPort(const char *ipv4, uint16_t port)
{
    if (!openSocket()) return 0;
    if (inet_pton(AF_INET, ipv4, &peer.sin_addr) != 1 ||
        peer.sin_addr.s_addr == 0 || peer.sin_addr.s_addr == 0xffffffffu) {
        lanClose(); status = "Invalid host IPv4"; return 0;
    }
    peer.sin_port = htons(port);
    status = "Looking for host";
    LAN_LOG("client looking for host build=%08x", (unsigned)LAN_BUILD_ID);
    return 1;
}

static void transmitDiscovery(uint32_t type, const struct sockaddr_in *destination,
                              uint32_t request)
{
    uint32_t p[ADVERTISE_WORDS] = {signature, type, nonce, request, ++seq,
        LAN_BUILD_ID, (uint32_t)arena, roundId, (1u << 16) | 2u};
    for (int i = 0; i < HEADER_WORDS; ++i) p[i] = htonl(p[i]);
    int words = HEADER_WORDS;
    if (type == ADVERTISE) {
        static const char name[] = "GoldenEye Host";
        memcpy((char *)&p[HEADER_WORDS], name, sizeof(name));
        words = ADVERTISE_WORDS;
    }
    sendto(sock, (const char *)p, words * sizeof(*p), 0,
           (const struct sockaddr *)destination, sizeof(*destination));
}

static void requestDiscovery(void)
{
    struct sockaddr_in destination = {0};
    destination.sin_family = AF_INET;
    destination.sin_port = htons(LAN_PORT);
#ifndef __XBOX__
    destination.sin_addr.s_addr = inet_addr(discoveryGroup);
    transmitDiscovery(DISCOVER, &destination, nonce);
#endif
    destination.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    transmitDiscovery(DISCOVER, &destination, nonce);
#ifdef __XBOX__
    /* Some access points do not bridge the limited broadcast between wired
     * Xbox and Wi-Fi. Send the interface's directed broadcast as well. */
    if (netif_default && netif_is_up(netif_default) &&
        !ip4_addr_isany_val(*netif_ip4_addr(netif_default))) {
        uint32_t directed = netif_ip4_addr(netif_default)->addr |
                            ~netif_ip4_netmask(netif_default)->addr;
        if (directed != destination.sin_addr.s_addr) {
            destination.sin_addr.s_addr = directed;
            transmitDiscovery(DISCOVER, &destination, nonce);
        }
        /* Home routers sometimes filter both limited and directed broadcasts
         * between wired and Wi-Fi. Probe the Xbox's local /24 with the same
         * 36-byte request; replies still require the matching build ID. */
        uint32_t local = ntohl(netif_ip4_addr(netif_default)->addr);
        uint32_t first = local & 0xffffff00u;
        for (uint32_t host = first + 1; host < first + 255; ++host) {
            if (host == local) continue;
            destination.sin_addr.s_addr = htonl(host);
            transmitDiscovery(DISCOVER, &destination, nonce);
        }
    }
#endif
    lastHello = SDL_GetTicks();
}

int lanDiscover(void)
{
    if (!openSocket()) return 0;
    int on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&on, sizeof(on))) {
        lanClose(); status = "Cannot enable LAN discovery"; return 0;
    }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))) {
        lanClose(); status = "Cannot bind LAN discovery socket"; return 0;
    }
    discovering = 1;
    discoveryStarted = SDL_GetTicks();
    status = "Looking for matches";
    requestDiscovery();
    return 1;
}

void lanDiscoverRefresh(void)
{
    if (!lanDiscovering()) return;
    matchCount = 0;
    discoveryStarted = SDL_GetTicks();
    status = "Looking for matches";
    requestDiscovery();
}

int lanDiscoveryCount(void) { return matchCount; }
const LanMatch *lanDiscoveryMatch(int index)
{
    return index >= 0 && index < matchCount ? &matches[index] : NULL;
}
int lanJoinDiscovered(int index)
{
    if (index < 0 || index >= matchCount) return 0;
    char address[sizeof(matches[index].address)];
    memcpy(address, matches[index].address, sizeof(address));
    uint16_t port = matches[index].port;
    return lanJoinOnPort(address, port);
}

static void rememberMatch(const struct sockaddr_in *from, const uint32_t *p,
                          const char *name, uint32_t now)
{
    char address[16];
    if (!inet_ntop(AF_INET, &from->sin_addr, address, sizeof(address))) return;
    uint16_t port = ntohs(from->sin_port);
    int index = 0;
    while (index < matchCount &&
           (matches[index].id != p[2] || matches[index].port != port ||
            strcmp(matches[index].address, address))) ++index;
    if (index == matchCount) {
        if (matchCount >= LAN_DISCOVERY_MAX) return;
        ++matchCount;
    }
    LanMatch *match = &matches[index];
    memset(match, 0, sizeof(*match));
    const char *label = name[0] ? name : "GoldenEye Host";
    size_t labelLength = 0;
    while (label[labelLength] && labelLength < sizeof(match->name)-1) ++labelLength;
    memcpy(match->name, label, labelLength);
    match->name[labelLength] = '\0';
    for (size_t i = 0; i < labelLength; ++i)
        if ((unsigned char)match->name[i] < 32 || (unsigned char)match->name[i] > 126)
            match->name[i] = '?';
    memcpy(match->address, address, strlen(address)+1);
    match->id = p[2];
    match->port = port;
    match->arena = (uint8_t)p[6];
    match->players = (uint8_t)(p[8] >> 16);
    match->capacity = (uint8_t)p[8];
    matchSeen[index] = now;
    status = "Select a match";
}

static void transmit(uint32_t type, const LanPose *pose)
{
    uint32_t p[WORDS] = {signature, type, nonce, other, ++seq, LAN_BUILD_ID, arena, roundId, scoreLimit};
    int count = pose ? (hosting ? WORDS : INPUT_WORDS) : HEADER_WORDS;
    if (pose) memcpy(&p[HEADER_WORDS], pose, (count-HEADER_WORDS)*sizeof(*p));
    for (int i=0; i<count; ++i) p[i] = htonl(p[i]);
    sendto(sock, (const char *)p, count*sizeof(*p), 0, (struct sockaddr *)&peer, sizeof(peer));
}

static int validDrop(const LanPose *state)
{
    const LanDrop *drop = &state->drop;
    if (state->activeDrops >> LAN_DROP_SLOTS) return 0;
    if (!drop->generation) return !state->activeDrops;
    if (drop->slot >= LAN_DROP_SLOTS || !(state->activeDrops & (1u << drop->slot)) ||
        drop->item > 255 || drop->room > 255)
        return 0;
    for (int row = 0; row < 3; ++row) {
        if (!(drop->position[row] >= -32768 && drop->position[row] <= 32768)) return 0;
        for (int col = 0; col < 3; ++col)
            if (!(drop->rotation[row][col] >= -100 && drop->rotation[row][col] <= 100)) return 0;
    }
    return 1;
}

static int validSeat(const LanSeat *s)
{
    const LanInventory *inv = &s->inventory;
    if (inv->ready > 1) return 0;
    for (int i = 0; i < LAN_AMMO_TYPES; ++i)
        if (inv->reserve[i] > 65535) return 0;
    for (int hand = 0; hand < 2; ++hand)
        if (inv->weapon[hand] > 255 || inv->magazine[hand] > 65535) return 0;
    return s->health >= -1000 && s->health <= 10 && s->armour >= 0 && s->armour <= 10 &&
           s->dead <= 1 && s->kills[0] <= 100000 && s->kills[1] <= 100000 && s->deaths <= 100000 &&
           s->spawnX >= -32768 && s->spawnX <= 32768 && s->spawnZ >= -32768 && s->spawnZ <= 32768 &&
           s->spawnFeet >= -32768 && s->spawnFeet <= 32768 && s->spawnYaw >= -360 && s->spawnYaw <= 360;
}

void lanPoll(void)
{
    if (!lanActive()) return;
    uint32_t now = SDL_GetTicks();
    if (discovering) {
        for (int i = 0; i < matchCount;) {
            if (now - matchSeen[i] <= 10000u) { ++i; continue; }
            --matchCount;
            matches[i] = matches[matchCount];
            matchSeen[i] = matchSeen[matchCount];
        }
        if (now - lastHello >= 2000u) requestDiscovery();
        if (!matchCount && now - discoveryStarted >= 1500u) status = "No matches found";
    }
    if (other && now-lastRx > 8000u) {
        LAN_LOG("peer timeout after %u ms", (unsigned)(now-lastRx));
        other = lastSeq = havePose = 0;
        status = hosting ? "Player disconnected" : "Reconnecting to host";
    }
    if (!hosting && !other && now-lastHello >= 500u) {
        transmit(HELLO, NULL);
        lastHello = now;
    }
    for (int attempt=0; attempt<16; ++attempt) {
        uint32_t p[WORDS+1];
        struct sockaddr_in from;
#if defined(__XBOX__)
        socklen_t len = sizeof(from);
#elif defined(_WIN32)
        int len = sizeof(from);
#else
        socklen_t len = sizeof(from);
#endif
        int n = recvfrom(sock, (char *)p, sizeof(p), 0, (struct sockaddr *)&from, &len);
        if (n < 0) break;
        if (n < HEADER_WORDS*4 || n > WORDS*4 || n % 4 || from.sin_family != AF_INET) continue;
        char advertisedName[33] = {0};
        if (n == ADVERTISE_WORDS*4)
            memcpy(advertisedName, (char *)&p[HEADER_WORDS], ADVERTISE_NAME_WORDS*4);
        for (int i=0; i<HEADER_WORDS; ++i) p[i] = ntohl(p[i]);
        if (p[0] != signature || !p[2]) continue;
        if (p[1] == DISCOVER) {
            if (hosting && !other && n == HEADER_WORDS*4 && p[5] == LAN_BUILD_ID)
                transmitDiscovery(ADVERTISE, &from, p[2]);
            continue;
        }
        if (p[1] == ADVERTISE) {
            if (discovering && n == ADVERTISE_WORDS*4 && p[3] == nonce &&
                p[5] == LAN_BUILD_ID && p[6] >= 1 && p[6] <= LAN_ARENAS &&
                (p[8] >> 16) < (p[8] & 0xffffu) && (p[8] & 0xffffu) <= 255)
                rememberMatch(&from, p, advertisedName, now);
            continue;
        }
        for (int i=HEADER_WORDS; i<n/4; ++i) p[i] = ntohl(p[i]);
        if ((!hosting || other) && (peer.sin_addr.s_addr != from.sin_addr.s_addr ||
            peer.sin_port != from.sin_port)) continue;
        if (p[5] != LAN_BUILD_ID) {
            if (!other) {
                status = "LAN build mismatch";
                LAN_LOG("rejected peer build=%08x expected=%08x",
                        (unsigned)p[5], (unsigned)LAN_BUILD_ID);
            }
            continue;
        }
        int expected = p[1] == POSITION ? (hosting ? INPUT_WORDS : WORDS) : HEADER_WORDS;
        if (n != expected*4 || p[6] < 1 || p[6] > LAN_ARENAS || p[8] > 100) continue;
        if (hosting && p[1] == HELLO && (!other || other == p[2])) {
            peer = from; other = p[2]; lastRx = now;
            status = "Player joined"; transmit(WELCOME, NULL);
            LAN_LOG("accepted player HELLO nonce=%u", (unsigned)other);
        } else if (!hosting && p[1] == WELCOME && !other && p[3] == nonce && p[7]) {
            arena = p[6]; roundId = p[7]; scoreLimit = p[8];
            other = p[2]; lastRx = now; status = "Joined LAN host";
            LAN_LOG("accepted host WELCOME nonce=%u", (unsigned)other);
        } else if (p[1] == POSITION && p[6] == (unsigned)arena && other && p[2] == other && p[3] == nonce &&
                   (int32_t)(p[4]-lastSeq) > 0) {
            LanPose next = {0};
            memcpy(&next, &p[HEADER_WORDS], n-HEADER_WORDS*4);
            if (!(next.x >= -32768 && next.x <= 32768 && next.feet >= -32768 &&
                  next.feet <= 32768 && next.z >= -32768 && next.z <= 32768 &&
                  next.yaw >= -360 && next.yaw <= 360 && next.pitch >= -90 && next.pitch <= 90 &&
                  next.forward >= -8 && next.forward <= 8 && next.strafe >= -8 && next.strafe <= 8 &&
                  (next.run == 0 || next.run == 1))) continue;
            if (!p[7] || (hosting && p[7] != roundId) || next.results > 1 || next.ready > 1 ||
                next.weapon > 255 || next.buttons > 511 || next.commands > LAN_WORLD_WORDS * 32 ||
                next.firstCommand > 65535) continue;
            if (!hosting && (!validSeat(&next.seats[0]) || !validSeat(&next.seats[1]) || !validDrop(&next))) continue;
            if (!hosting) roundId = p[7];
            lastRx = now; lastSeq = p[4]; remote = next; havePose = 1;
        }
    }
}
void lanPublish(const LanPose *pose)
{
    uint32_t now = SDL_GetTicks();
    if (lanConnected() && now-lastTx >= 30u) {
        transmit(POSITION, pose);
        lastTx = now;
    }
}
int lanPublishDue(void) { return lanConnected() && SDL_GetTicks()-lastTx >= 30u; }
uint32_t lanRemoteSequence(void) { return lastSeq; }
int lanRemote(LanPose *pose)
{
    if (!havePose) return 0;
    *pose = remote;
    return 1;
}
_Static_assert(sizeof(LanInventory) == 39 * sizeof(uint32_t), "LAN inventory wire layout");
_Static_assert(sizeof(LanSeat) == 50 * sizeof(uint32_t), "LAN seat wire layout");
_Static_assert(sizeof(LanPose) == 138 * sizeof(uint32_t), "LAN pose wire layout");
