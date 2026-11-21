#include "perf_hud.h"
#include "port/lan/frame_trace.h"
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include "port/controls/finals_font.inc"

extern "C" bool xboxDrawPerfTile(int left, int top, int width, int height, const uint32_t *pixels);
bool xboxHudConfigured;
unsigned xboxHudScanEpoch;
namespace {
constexpr int width = 260, height = 48, history = 512;
constexpr int graphX = 105, graphWidth = 116, graphBottom = 32;
constexpr unsigned background = 0xe0161b20, white = 0xfff1f4f1, muted = 0xffb4c1c5;
unsigned stamps[history], published, publishedEpoch;
bool testView, benchmark;
unsigned epoch, lastRefresh, costFrames;
uint64_t prepTotal, waitTotal;
double clockHz;
uint32_t *tile;

void label(int x, int y, const char *str, unsigned color, int scale = 1)
{
    for (; *str && x+5*scale<=width; ++str, x+=6*scale) {
        char c = *str;
        int idx = c>='A' && c<='Z' ? c-'A' : c>='0' && c<='9' ? 26+c-'0' :
                  c=='-' ? 36 : c=='.' ? 37 : -1;
        if (idx<0) continue;
        for (int row=0; row<7; ++row) for (int col=0; col<5; ++col) {
            if (!(retroGlyphs[idx][row] & (16>>col))) continue;
            for (int dy=0; dy<scale; ++dy)
                std::fill_n(tile+(y+row*scale+dy)*width+x+col*scale, scale, color);
        }
    }
}

int graphY(double ms)
{
    if (ms<=33.333) return graphBottom-int(ms*0.45+0.5);
    // Keep the refresh guides legible during a hitch. Above two refreshes,
    // each doubling of the interval uses three more pixels.
    double lower=33.333;
    int y=graphBottom-15;
    while (ms>2*lower && y>5) { lower*=2; y-=3; }
    return std::max(2,y-int(std::min((ms-lower)/lower,1.0)*3+0.5));
}

void paint(unsigned now)
{
    std::fill_n(tile,width*height,background);
    char buf[80];
    label(graphX+graphWidth+5,2,"MS",muted);
    for (double guide : {16.667,33.333}) {
        const int y=graphY(guide);
        std::fill_n(tile+y*width+graphX,graphWidth,0xff667781);
        label(graphX+graphWidth+5,y-3,guide<20 ? "16.7" : "33.3",muted);
    }
    const double msPerStamp=1024000.0/clockHz;
    unsigned changes=0, span=0, newest=0;
    double worst=0;
    if (__atomic_load_n(&publishedEpoch,__ATOMIC_ACQUIRE)==epoch) {
        const unsigned end=__atomic_load_n(&published,__ATOMIC_ACQUIRE);
        // A single atomic stamp per changed scanout. At most 301 stamps are
        // needed for five seconds at 60 Hz, well inside the 512-slot ring.
        for (unsigned i=1; i<std::min(end,unsigned(history)); ++i) {
            unsigned right=__atomic_load_n(&stamps[(end-i)%history],__ATOMIC_RELAXED);
            unsigned left=__atomic_load_n(&stamps[(end-i-1)%history],__ATOMIC_RELAXED);
            if (int32_t(now-right)<0) continue;
            double age=(now-right)*msPerStamp;
            if (age>=5000) break;
            double interval=(right-left)*msPerStamp;
            worst=std::max(worst,interval);
            if (age<1000) {
                if (!changes) newest=right;
                ++changes;
                span=newest-left;
            }
            int x=graphX+graphWidth-1-int(age*graphWidth/5000);
            const unsigned color=interval>34 ? 0xffffa866 : 0xff81dcbd;
            // Multiple frames may share one horizontal pixel. Drawing each
            // column from its sample to the baseline retains its tallest spike.
            for (int y=graphY(interval); y<=graphBottom; ++y) tile[y*width+x]=color;
            if (interval>100) tile[graphY(interval)*width+x]=0xffff635d;
        }
    }
    if (span) {
        unsigned fps=unsigned(10000.0*changes/(span*msPerStamp)+0.5);
        std::snprintf(buf,sizeof(buf),"%u.%u",fps/10,fps%10);
    }
    else std::snprintf(buf,sizeof(buf),"--");
    label(5,5,buf,white,2);
    label(64,10,"FPS",white);
    if (testView && worst>0) {
        std::snprintf(buf,sizeof(buf),"5S MAX %uMS",unsigned(worst+0.5));
        label(5,25,buf,muted);
    } else label(5,25,"LAST 5S",muted);
    MM_STATISTICS stats={};
    stats.Length=sizeof(stats);
    char ram[16]="--";
    if (MmQueryStatistics(&stats)>=0) {
        unsigned tenths=(stats.AvailablePages*10+128)/256;
        std::snprintf(ram,sizeof(ram),"%u.%u",tenths/10,tenths%10);
    }
    const double ms=costFrames ? 1000.0/(costFrames*clockHz) : 0;
    unsigned prep=unsigned(prepTotal*ms*10+0.5), wait=unsigned(waitTotal*ms*10+0.5);
    if (costFrames)
        std::snprintf(buf,sizeof(buf),"PREP %u.%uMS  GPU WAIT %u.%uMS  FREE %sMIB",prep/10,prep%10,wait/10,wait%10,ram);
    else std::snprintf(buf,sizeof(buf),"PREP --MS  GPU WAIT --MS  FREE %sMIB",ram);
    label(5,37,buf,white);
    costFrames=0;
    prepTotal=waitTotal=0;
}
}

extern "C" void xboxHudScanout(unsigned requestedEpoch,unsigned surface)
{
    // One writer, in the vblank interrupt. No allocation, I/O or locks.
    static unsigned lastEpoch, lastSurface, written;
    if (lastEpoch!=requestedEpoch) {
        lastEpoch=requestedEpoch;
        written=0;
        lastSurface=~surface;
    }
    if (lastSurface==surface) return;
    lastSurface=surface;
    unsigned tick=unsigned(__builtin_ia32_rdtsc()>>10);
    __atomic_store_n(&stamps[written%history],tick,__ATOMIC_RELAXED);
    __atomic_store_n(&published,++written,__ATOMIC_RELEASE);
    __atomic_store_n(&publishedEpoch,requestedEpoch,__ATOMIC_RELEASE);
}

void xboxHudInit()
{
    FILE *file=std::fopen("D:\\ge-perf-hud.on","rb");
    if (file) {
        int c=std::fgetc(file);
        testView=(c=='t' || c=='T');
        std::fclose(file);
    } else {
        file=std::fopen("D:\\ge-hud-ab.on","rb");
        if (!file) return;
        std::fclose(file);
        benchmark=testView=true;
    }
    LARGE_INTEGER hz;
    if (!QueryPerformanceFrequency(&hz) || hz.QuadPart<=0) return;
    clockHz=double(hz.QuadPart);
    tile=static_cast<uint32_t *>(std::malloc(width*height*sizeof(*tile)));
    xboxHudConfigured=tile!=nullptr;
}

bool xboxHudBeginFrame()
{
    const bool visible=!benchmark || ftHudVisible();
    if (visible && !xboxHudScanEpoch) {
        ++epoch;
        __atomic_store_n(&xboxHudScanEpoch,epoch,__ATOMIC_RELEASE);
        lastRefresh=costFrames=0;
        prepTotal=waitTotal=0;
    } else if (!visible && xboxHudScanEpoch) {
        __atomic_store_n(&xboxHudScanEpoch,0,__ATOMIC_RELEASE);
    }
    return visible;
}

void xboxHudDraw(int screenWidth,int screenHeight)
{
    unsigned now=unsigned(__builtin_ia32_rdtsc()>>10);
    bool refresh=!lastRefresh || (now-lastRefresh)>=clockHz/(4.0*1024);
    if (refresh) { paint(now); lastRefresh=now; }
    if (!xboxDrawPerfTile((screenWidth-width)/2,screenHeight/24,width,height,refresh?tile:nullptr))
        lastRefresh=0;
}

void xboxHudFrameCost(uint64_t prepTicks,uint64_t waitTicks)
{
    ++costFrames;
    prepTotal+=prepTicks;
    waitTotal+=waitTicks;
}
