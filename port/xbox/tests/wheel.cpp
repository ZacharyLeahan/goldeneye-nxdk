// Host regression test. Compile with the generated finals headers and SDL2.
#include "../finals_controls.cpp"
#include <cassert>

#ifdef main
#undef main
#endif

static Uint32 ticks;
extern "C" Uint32 SDL_GetTicks(void) { return ticks; }
extern "C" bool xboxDrawWheel(int,int,int,int,int,const uint32_t *) { return true; }

struct Sample {
    Uint32 ms;
    unsigned buttons;
    int16_t x,y;
};

int main()
{
    const FinalsInventory inventory={{4,6,GRENADE_ITEM,PAD_ITEM},4,4,3,0,0,1,1,1};
    auto run=[&](std::initializer_list<Sample> events, int expected, bool compare=true,
                 int grenades=3) {
        Wheel desktop;
        desktop.active=true;
        desktop.snapshot=inventory;
        desktop.snapshot.grenades=grenades;
        xboxFinalsReset(0);
        int chosen=-1;
        for (auto e : events) {
            ticks=e.ms;
            FinalsPad pad{0,0,e.x,e.y,e.buttons};
            desktop.requested=-1;
            desktop.wheelUpdate(e.buttons & FIN_WHEEL,e.buttons & FIN_B,e.x/32768.f,e.y/32768.f);
            int open=xboxFinalsWheel(0,1,&pad,&desktop.snapshot,&chosen);
            if (compare) {
                assert(open==desktop.openWheel);
                assert(chosen==desktop.requested);
            }
        }
        assert(chosen==expected);
    };

    constexpr unsigned W=FIN_WHEEL;
    for (unsigned dwell : {1u,17u,65u,200u}) {
        run({{0,W,0,32767},{dwell,0,0,32767}}, PAD_ITEM);
        run({{0,W,32767,0},{dwell,0,32767,0}}, GRENADE_ITEM);
    }
    run({{0,W,0,-32767},{10,0,0,-32767}},6);
    run({{0,W,-32767,0},{10,0,-32767,0}},-1); // already equipped
    run({{0,W,0,0},{15,0,0,0}},-1);
    run({{0,W,0,4000},{15,0,0,0}},-1);
    run({{0,W,32767,0},{10,0,32767,0}},-1,true,0);
    run({{0,W,0,32767},{15,W|FIN_B,0,32767},{25,0,0,0}},-1);
    run({{0,W,0,32767},{15,FIN_B,0,0}},-1);
    run({{0,W,0,32767},{10,W,32767,0},{15,0,32767,0}},GRENADE_ITEM);
    run({{0,W,0,32767},{160,W,0,0},{200,0,0,0}},-1);
    run({{0,W,0,32767},{160,W,0,0},{180,W,32767,0},{190,0,32767,0}},GRENADE_ITEM);
    // Xbox tolerates spring return just before or on the release report.
    for (unsigned delay : {1u,17u,50u,100u})
        run({{0,W,0,32767},{delay,0,0,0}},PAD_ITEM,false);
    run({{0,W,0,32767},{17,W,0,0},{45,0,0,0}},PAD_ITEM,false);
    run({{0,W,0,32767},{101,0,0,0}},-1);
    run({{0xfffffff0u,W,0,32767},{3,0,0,0}},PAD_ITEM,false);
    // Interleaved controllers keep independent sectors and release times.
    for (int slot=0;slot<2;slot++) xboxFinalsReset(slot);
    int choice;
    FinalsPad down{0,0,0,32767,W},right{0,0,32767,0,W},neutral{};
    ticks=0;
    assert(xboxFinalsWheel(0,1,&down,&inventory,&choice));
    assert(xboxFinalsWheel(1,1,&right,&inventory,&choice));
    ticks=15;
    assert(!xboxFinalsWheel(1,1,&neutral,&inventory,&choice) && choice==GRENADE_ITEM);
    assert(!xboxFinalsWheel(0,1,&neutral,&inventory,&choice) && choice==PAD_ITEM);
}
