/* One direct IPv4 peer, no directory service. Wire fields are 32-bit words in
 * network order. A boot nonce on each end excludes stale match datagrams. */
#ifdef __XBOX__
#include <lwip/sockets.h>
#include <lwip/inet.h>
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

enum { HELLO = 1, WELCOME, POSITION, HEADER_WORDS = 9,
       INPUT_WORDS = HEADER_WORDS + __builtin_offsetof(LanPose, seats) / sizeof(uint32_t),
       WORDS = HEADER_WORDS + sizeof(LanPose) / sizeof(uint32_t) };
static const uint32_t signature = 0x47454c31; /* GEL1, NTSC-U */
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
static int hosting, havePose, netReady;
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
    havePose = hosting = 0;
    status = "Offline";
}
int lanActive(void) { return sock != INVALID_SOCKET; }
int lanIsHost(void) { return hosting; }
int lanConnected(void) { return lanActive() && other != 0; }
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
    hosting = 1;
    roundId = nonce;
    status = "Waiting for player";
    LAN_LOG("host ready on UDP %d build=%08x", port, (unsigned)LAN_BUILD_ID);
    return 1;
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
#ifdef _WIN32
        int len = sizeof(from);
#else
        socklen_t len = sizeof(from);
#endif
        int n = recvfrom(sock, (char *)p, sizeof(p), 0, (struct sockaddr *)&from, &len);
        if (n < 0) break;
        if (n < HEADER_WORDS*4 || n > WORDS*4 || n % 4 || from.sin_family != AF_INET) continue;
        for (int i=0; i<n/4; ++i) p[i] = ntohl(p[i]);
        if (p[0] != signature || !p[2]) continue;
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
            other = p[2]; lastRx = now; status = "Joined Windows host";
            LAN_LOG("accepted Windows WELCOME nonce=%u", (unsigned)other);
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
_Static_assert(sizeof(LanPose) == 135 * sizeof(uint32_t), "LAN pose wire layout");
