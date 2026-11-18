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
typedef SOCKET LanSocket;
#define CLOSE_SOCKET closesocket
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
typedef int LanSocket;
#define INVALID_SOCKET (-1)
#define CLOSE_SOCKET close
#endif
#include <SDL_timer.h>
#include <string.h>
#include "lan.h"

enum { HELLO = 1, WELCOME, POSITION, WORDS = 11 };
static const uint32_t signature = 0x47454c31; /* GEL1, NTSC-U Complex */
static LanSocket sock = INVALID_SOCKET;
static struct sockaddr_in peer;
static uint32_t nonce, other, seq, lastSeq, lastRx, lastTx, lastHello;
static int hosting, havePose, netReady;
static LanPose remote;
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
    if (!openSocket()) return 0;
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LAN_PORT);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))) {
        lanClose(); status = "UDP port 27007 is in use"; return 0;
    }
    hosting = 1;
    status = "Waiting for Xbox";
    return 1;
}
int lanJoin(const char *ipv4)
{
    if (!openSocket()) return 0;
    if (inet_pton(AF_INET, ipv4, &peer.sin_addr) != 1 ||
        peer.sin_addr.s_addr == 0 || peer.sin_addr.s_addr == 0xffffffffu) {
        lanClose(); status = "Invalid host IPv4"; return 0;
    }
    peer.sin_port = htons(LAN_PORT);
    status = "Looking for host";
    return 1;
}
static void transmit(uint32_t type, const LanPose *pose)
{
    uint32_t p[WORDS] = {signature, type, nonce, other, ++seq, LAN_BUILD_ID};
    if (pose) memcpy(&p[6], pose, sizeof(*pose));
    for (int i=0; i<WORDS; ++i) p[i] = htonl(p[i]);
    sendto(sock, (const char *)p, sizeof(p), 0, (struct sockaddr *)&peer, sizeof(peer));
}

void lanPoll(void)
{
    if (!lanActive()) return;
    uint32_t now = SDL_GetTicks();
    if (other && now-lastRx > 8000u) {
        other = lastSeq = havePose = 0;
        status = hosting ? "Xbox disconnected" : "Reconnecting to host";
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
        if (n != WORDS*4 || from.sin_family != AF_INET) continue;
        for (int i=0; i<WORDS; ++i) p[i] = ntohl(p[i]);
        if (p[0] != signature || !p[2]) continue;
        if ((!hosting || other) && (peer.sin_addr.s_addr != from.sin_addr.s_addr ||
            peer.sin_port != from.sin_port)) continue;
        if (p[5] != LAN_BUILD_ID) { if (!other) status = "LAN build mismatch"; continue; }
        if (hosting && p[1] == HELLO && (!other || other == p[2])) {
            peer = from; other = p[2]; lastRx = now;
            status = "Xbox joined"; transmit(WELCOME, NULL);
        } else if (!hosting && p[1] == WELCOME && !other && p[3] == nonce) {
            other = p[2]; lastRx = now; status = "Joined Windows host";
        } else if (p[1] == POSITION && other && p[2] == other && p[3] == nonce &&
                   (int32_t)(p[4]-lastSeq) > 0) {
            LanPose next;
            memcpy(&next, &p[6], sizeof(next));
            if (!(next.x >= -32768 && next.x <= 32768 && next.feet >= -32768 &&
                  next.feet <= 32768 && next.z >= -32768 && next.z <= 32768 &&
                  next.yaw >= -360 && next.yaw <= 360 && next.pitch >= -90 && next.pitch <= 90)) continue;
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
int lanRemote(LanPose *pose)
{
    if (!havePose) return 0;
    *pose = remote;
    return 1;
}
_Static_assert(sizeof(LanPose) == 20, "LAN pose wire layout");
