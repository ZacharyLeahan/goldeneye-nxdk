// Native host for the fork's equipment wheel and full-resolution look response.
#include "finals_controls.h"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "finals_constants.h"
#define GRENADE_ITEM 26
#include "finals_items.inc"
#include "finals_aim.inc"
#include "finals_font.inc"

namespace {
struct Wheel {
    FinalsInventory snapshot{};
    int ids[32]{}, count = 0, choice = -1, oldCurrent = -1, requested = -1;
    bool active = false, openWheel = false, previousLB = false;
    bool cancelled = false, wheelStickEngaged = false;
    float lastX = 0, lastY = 0;
    Uint32 lastDirection = 0;
    bool hasDirection = false;
    bool playing() const { return active; }
    const FinalsInventory &snapshotRead() const { return snapshot; }
    int current() const { return snapshot.current; }
    void inventory() {
        count = snapshot.count;
        memcpy(ids, snapshot.items, sizeof(ids));
    }
    bool available(int id) const {
        if (id == GRENADE_ITEM && !snapshot.grenades) return false;
        return std::find(snapshot.items, snapshot.items + snapshot.count, id) != snapshot.items + snapshot.count;
    }
    void equip(int id) { requested = id; }
#include "finals_selection.inc"
};

Wheel wheels[4];
PrecisionAimState look[4]{};
SDL_SpinLock wheelLock;

// The Windows renderer emits screen-space polygons and lines. Rasterize those
// same primitives into a transparent texture when the visible wheel changes.
enum { GL_QUADS, GL_POLYGON, GL_LINE_LOOP, GL_LINE_STRIP, GL_TRIANGLE_STRIP };
struct Point { float x,y; };
struct Transform { float x,y,sx,sy; };

struct Painter : Wheel {
    uint32_t *pixels;
    int stride, left, top, right, bottom, primitive, size;
    float uiScale, lineWidth;
    Transform transform, saved;
    Point points[160];
    unsigned alpha, red, green, blue;

    void glColor4f(float r,float g,float b,float a) {
        red = unsigned(r*255); green = unsigned(g*255); blue = unsigned(b*255); alpha = unsigned(a*255);
    }
    void glPushMatrix() { saved = transform; }
    void glPopMatrix() { transform = saved; }
    void glTranslatef(float x,float y,float) { transform.x+=x*transform.sx; transform.y+=y*transform.sy; }
    void glScalef(float x,float y,float) { transform.sx*=x; transform.sy*=y; }
    void glLineWidth(float w) { lineWidth=w; }
    void glBegin(int mode) { primitive=mode; size=0; }
    void glVertex2f(float x,float y) {
        if (size < 160) points[size++]={transform.x+x*transform.sx,transform.y+y*transform.sy};
    }
    void span(int y,int x1,int x2) {
        if (y<top || y>=bottom) return;
        x1=std::max(x1,left); x2=std::min(x2,right-1);
        if (x1 > x2) return;
        uint32_t *row=pixels+y*stride;
        uint32_t color=(alpha<<24)|(red<<16)|(green<<8)|blue;
        if (alpha==255 || (x1==left && x2==right-1 && size==4 && !row[x1])) {
            std::fill(row+x1,row+x2+1,color);
            return;
        }
        for (int x=x1;x<=x2;x++) {
            uint32_t d=row[x];
            unsigned dstAlpha=((d>>24)*(255-alpha)+127)/255;
            unsigned outAlpha=alpha+dstAlpha;
            if (!outAlpha) continue;
            unsigned r=(red*alpha+((d>>16)&255)*dstAlpha)/outAlpha;
            unsigned g=(green*alpha+((d>>8)&255)*dstAlpha)/outAlpha;
            unsigned b=(blue*alpha+(d&255)*dstAlpha)/outAlpha;
            row[x]=(outAlpha<<24)|(r<<16)|(g<<8)|b;
        }
    }
    void fill(const Point *p,int n) {
        float low=p[0].y,high=low;
        for(int i=1;i<n;i++){low=std::min(low,p[i].y);high=std::max(high,p[i].y);}
        for(int y=std::max(top,int(floorf(low)));y<std::min(bottom,int(ceilf(high)));y++){
            float hits[32],scan=y+.5f; int used=0;
            for(int i=0,j=n-1;i<n;j=i++){
                if((p[i].y>scan)==(p[j].y>scan)) continue;
                hits[used++]=p[i].x+(scan-p[i].y)*(p[j].x-p[i].x)/(p[j].y-p[i].y);
            }
            std::sort(hits,hits+used);
            for(int i=1;i<used;i+=2) span(y,int(ceilf(hits[i-1]-.5f)),int(ceilf(hits[i]-.5f))-1);
        }
    }
    void line(Point a,Point b) {
        float dx=b.x-a.x,dy=b.y-a.y;
        int steps=std::max(1,int(ceilf(std::max(fabsf(dx),fabsf(dy)))));
        for(int i=0;i<=steps;i++){
            float f=float(i)/steps;
            int x=int(a.x+dx*f);
            span(int(a.y+dy*f),x,x+(lineWidth>1.5f));
        }
    }
    void glEnd() {
        if(size<2) return;
        if(primitive==GL_TRIANGLE_STRIP){
            for(int i=2;i<size;i++) fill(points+i-2,3);
        } else if(primitive==GL_LINE_LOOP || primitive==GL_LINE_STRIP){
            for(int i=1;i<size;i++) line(points[i-1],points[i]);
            if(primitive==GL_LINE_LOOP) line(points[size-1],points[0]);
        } else if(primitive==GL_QUADS){
            for(int i=0;i+3<size;i+=4) fill(points+i,4);
        } else if(size<=32) fill(points,size);
    }
    float textWidth(const char *s) { return strlen(s)*16.f; }
    void text(float x,float y,const char *s) {
        for(;*s;s++,x+=16){
            int c=*s;
            if(c>='a'&&c<='z') c-=32;
            int idx=c>='A'&&c<='Z'?c-'A':c>='0'&&c<='9'?26+c-'0':-1;
            if(idx<0) continue;
            for(int row=0;row<7;row++) for(int col=0;col<5;col++){
                if(!(retroGlyphs[idx][row]&(16>>col))) continue;
                int px=int(transform.x+(x+col*2.6f)*uiScale);
                int py=int(transform.y+(y-18+row*2.6f)*uiScale);
                int side=std::max(1,int(2.6f*uiScale));
                for(int i=0;i<side;i++) span(py+i,px,px+side-1);
            }
        }
    }
    void centered(float x,float y,const char*s){ text(x-textWidth(s)*.5f,y,s); }
    void drawPadHint(bool valid) {
        const char *hint=valid?"RT PLACE   B CANCEL":"CLEAR LEVEL FLOOR REQUIRED";
        uiScale=std::min(right/460.f,std::max(2.f/2.6f,uiScale));
        transform={0,0,uiScale,uiScale};
        float bottomY=bottom/uiScale-19;
        glColor4f(.035f,.045f,.035f,.78f);
        glBegin(GL_QUADS);
        float end=24+textWidth(hint);
        glVertex2f(10,bottomY-48); glVertex2f(end,bottomY-48);
        glVertex2f(end,bottomY+6); glVertex2f(10,bottomY+6);
        glEnd();
        glColor4f(.89f,.87f,.69f,1);
        text(17,bottomY-24,"JUMP PAD");
        glColor4f(valid?.8f:1.f,valid?.94f:.55f,valid?.65f:.44f,1);
        text(17,bottomY,hint);
    }
#include "finals_art.inc"
};
}

extern "C" void xboxFinalsReset(int slot) {
    look[slot]={};
    SDL_AtomicLock(&wheelLock);
    wheels[slot]=Wheel{};
    SDL_AtomicUnlock(&wheelLock);
}
extern "C" int xboxFinalsWheel(int slot,int active,const FinalsPad *p,const FinalsInventory *items,int *equip) {
    SDL_AtomicLock(&wheelLock);
    Wheel &w=wheels[slot]; w.active=active; w.snapshot=*items; w.requested=-1;
    const bool held = p->buttons & FIN_WHEEL;
    const Uint32 now = SDL_GetTicks();
    float x = p->rx / 32768.f, y = p->ry / 32768.f;
    if (!w.openWheel) w.hasDirection = false;
    // A spring-centred stick often arrives at neutral just before White comes
    // up. Keep the last sector briefly. B and a deliberate recenter still cancel.
    if (x*x + y*y > .28f*.28f) {
        w.hasDirection = true;
        w.lastDirection = now;
        w.lastX = x;
        w.lastY = y;
    } else if (w.hasDirection && now - w.lastDirection <= 100) {
        x = w.lastX;
        y = w.lastY;
    }
    w.wheelUpdate(held,p->buttons&FIN_B,x,y);
    *equip=w.requested;
    bool open=w.openWheel;
    SDL_AtomicUnlock(&wheelLock);
    return open;
}
extern "C" void xboxFinalsLook(int slot,const FinalsPad *p,float fov,float dt,float *yaw,float *pitch) {
    double d[2];
    aimLookResponse(p->rx,p->ry,4000,1,65,60,std::clamp(fov,1.f,160.f),
                    std::clamp(dt/60.,0.,.05),!!(p->buttons&FIN_AIM),d,&look[slot]);
    *yaw+=float(d[0]);
    *pitch=std::clamp(*pitch+float(d[1]),-90.f,90.f);
}
extern "C" void xboxFinalsOverlay(int width,int height) {
    struct CachedWheel {
        int width, height, choice, count, grenades, padHint, ids[32];
    };
    static CachedWheel cached[4]{};
    static std::vector<uint32_t> scratch;
    for(int i=0;i<4;i++){
        Painter painter;
        SDL_AtomicLock(&wheelLock);
        bool open=wheels[i].openWheel;
        bool carrying=wheels[i].active && wheels[i].snapshot.current==PAD_ITEM;
        if(open || carrying) static_cast<Wheel&>(painter)=wheels[i];
        SDL_AtomicUnlock(&wheelLock);
        if(!open && !carrying) continue;
        auto &v=painter.snapshot;
        int left=std::clamp(int(v.left*width),0,width);
        int top=std::clamp(int(v.top*height),0,height);
        int w=std::clamp(int((v.left+v.width)*width+.5f),left,width)-left;
        int h=std::clamp(int((v.top+v.height)*height+.5f),top,height)-top;
        if(w<1||h<1) continue;
        CachedWheel key{w,h};
        if (open) {
            key.choice=painter.choice; key.count=painter.count; key.grenades=v.grenades;
            std::copy_n(painter.ids,painter.count,key.ids);
        } else key.padHint=v.validPlacement?2:1;
        bool changed=memcmp(&key,&cached[i],sizeof(key))!=0;
        if(changed) {
            scratch.assign(w*h,0);
            painter.pixels=scratch.data(); painter.stride=w;
            painter.left=0; painter.top=0; painter.right=w; painter.bottom=h;
            painter.uiScale=std::min(w/1280.f,h/720.f);
            painter.transform={0,0,painter.uiScale,painter.uiScale};
            painter.lineWidth=1;
            if (open) painter.drawSectors(w/painter.uiScale,h/painter.uiScale);
            else painter.drawPadHint(v.validPlacement);
        }
        if(xboxDrawWheel(i,left,top,w,h,changed?scratch.data():nullptr))
            cached[i]=key;
    }
}
