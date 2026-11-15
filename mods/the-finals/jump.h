/* Native vertical movement extension for GE32 2026.11.07 e8859fe9.
 * Only the simulation thread reads/writes player physics. No render offsets. */
#define PF(off) (*(float*)((unsigned char*)CURRENT+(off)))
#define PI(off) (*(int*)((unsigned char*)CURRENT+(off)))
#define PP(off) (*(void**)((unsigned char*)CURRENT+(off)))
typedef struct {float x,y,z;} JumpVec;
typedef struct {JumpVec position,normal;void*v[4];short flags[4];} JumpHit;
static volatile LONG pendingJump;
static int jumpAHeld,jumpActive,jumpCount,ceilingStops;
static float jumpCooldown,jumpOriginFloor;
static unsigned short jumpTestMotion;
static void *jumpPlayer;
static void (*originalVertical)(int,float);
static int jumpCameraPlayable(void){int mode=*(int*)(base+0x303000+65816);return mode==0||(mode==4&&*(int*)(base+0x228000+13352));}
static int jumpAllowed(void){
 return CURRENT&&jumpCameraPlayable()&&!PI(256)&&PI(512)&&!PI(588)&&!PI(13948)&&
 !*(int*)(base+0x303000+65912)&&
 !FN(int(*)(void),0x1000+537648)()&&!FN(int(*)(void),0x1000+61392)()&&
 !FN(int(*)(void),0x1000+1453264)()&&!FN(int(*)(void),0x1000+1338832)()&&!FN(int(*)(void),0x1000+1212864)()&&!FN(int(*)(void),0x1000+1193040)();
}
static unsigned char jumpA(unsigned char raw,int allowed,volatile LONG*latch){
 if(raw&&!jumpAHeld&&allowed){InterlockedExchange(latch,0);InterlockedExchange(&pendingJump,1);}
 jumpAHeld=raw!=0;
 return allowed?0:raw;
}
/* Seventeen upward traces cover the center and full circular body extent. Search
 * every loaded room, not just the floor tile, so neighboring-room ceilings
 * across portals also participate. Dynamic doors/scenery use engine tracing. */
/* Native render collision bounds are lazy. Prepare the occupied room on the
 * simulation thread before tracing instead of mistaking a missing cache for a ceiling. */
static int jumpRoomReady(int room){
 int total=*(int*)(base+0x228000+4208);if(room<0||room>=total||total>2048)return 0;
 void**bounds=(void**)(base+0x303000+27264+room*0x68+0x40);
 if(!*bounds){FN(int(*)(int),0x1000+8064)(room);FN(void(*)(int),0x1000+11024)(room);}
 return *bounds!=NULL;
}
/* Collision must not depend on which direction the camera happened to render.
 * Room bounds are in scaled BG coordinates; prepare only intersected geometry. */
static int collisionPrepareBox(JumpVec low,JumpVec high){
 int total=*(int*)(base+0x228000+4208);float scale=*(float*)(base+0x228000+4252);if(total<1||total>2048||!isfinite(scale)||scale<=0)return 0;
 for(int r=1;r<total;r++){
  unsigned char*p=(void*)(base+0x303000+27264+r*0x68);if(*(int*)(p+36)<=0)continue;
  float*lo=(void*)(p+0x50),*hi=(void*)(p+0x5c);float a[3]={low.x,low.y,low.z},b[3]={high.x,high.y,high.z};int overlap=1;
  for(int i=0;i<3;i++){if(!isfinite(lo[i])||!isfinite(hi[i])||lo[i]>hi[i])return 0;if(a[i]*scale>hi[i]+1||b[i]*scale<lo[i]-1)overlap=0;}
  if(overlap&&!jumpRoomReady(r))return 0;
 }
 return 1;
}
static float jumpCeiling(float top){
 int total=*(int*)(base+0x228000+4208);if(total<1||total>2048)return -INFINITY;
 void*tile=PP(1856);void*prop=PP(200);if(!tile||!prop)return -INFINITY;
 int room=FN(int(*)(void*),0x1000+883888)(tile);
 unsigned char*rooms=(void*)(base+0x303000+27264);
 if(!jumpRoomReady(room))return -INFINITY;
 float scale=*(float*)(base+0x228000+4252);if(!isfinite(scale)||scale<=0)return -INFINITY;
 float ceiling=INFINITY,foot=PF(140),head=fmaxf(PF(13940),120.f)+14.f;
 float x=PF(1864),z=PF(1872);if(!collisionPrepareBox((JumpVec){x-31,foot+.1f,z-31},(JumpVec){x+31,fmaxf(top,foot+head+4),z+31}))return -INFINITY;int occupied[64];for(int i=0;i<64;i++)occupied[i]=-1;
 FN(void(*)(void*,int*),0x1000+275920)(prop,occupied);
 for(int ray=0;ray<17;ray++){
  float ox=0,oz=0;if(ray){float angle=(ray-1)*.3926990817f;ox=31*cosf(angle);oz=31*sinf(angle);}
  JumpVec from={x+ox,foot+.25f,z+oz},to={from.x,fmaxf(top,foot+head+4),from.z};
  for(int r=0;r<total;r++){
   if(!*(void**)(rooms+r*0x68+0x40))continue;
   JumpHit hit={0};
   if(FN(int(*)(JumpVec*,JumpVec*,int,JumpHit*),0x1000+13744)(&from,&to,r,&hit)){
    float y=hit.position.y/scale;if(isfinite(y)&&y>from.y-.5f&&y<ceiling)ceiling=y;
   }
  }
  JumpVec position,normal,dynamicFrom=from;dynamicFrom.y=foot+32;
  if(FN(int(*)(void*,JumpVec*,JumpVec*,unsigned,JumpVec*,JumpVec*,int*),0x1000+721344)(prop,&dynamicFrom,&to,0x13,&position,&normal,occupied)){
   if(isfinite(position.y)&&position.y>from.y-.5f&&position.y<ceiling)ceiling=position.y;
  }
 }
 return ceiling;
}
static void jumpBeforeVertical(void){
 if(jumpPlayer!=CURRENT){jumpPlayer=CURRENT;jumpActive=0;jumpCooldown=0;InterlockedExchange(&pendingJump,0);}
 float dt=*(float*)(base+0x303000+211436);if(!isfinite(dt)||dt<0)dt=0;
 jumpCooldown=fmaxf(0,jumpCooldown-dt);
 int requested=InterlockedExchange(&pendingJump,0);
 if(!jumpAllowed()){if(PI(256)||*(int*)(base+0x303000+65912))jumpActive=0;return;}
 float ground=PF(144),foot=PF(140),velocity=PF(152);
 if(!isfinite(ground)||!isfinite(foot)||!isfinite(velocity))return;
 int grounded=fabsf(foot-ground)<1.5f&&fabsf(velocity)<.6f;
 if(requested&&!jumpActive&&grounded&&jumpCooldown<=0){
  float head=fmaxf(PF(13940),120.f)+14.f;
  float ceiling=jumpCeiling(ground+head+70.f);
  if(ceiling>ground+head+5.f){
   jumpOriginFloor=ground;PI(184)=2;PF(140)=fmaxf(foot,ground)+.05f;PF(152)=getenv("GE32_JUMP_CEILING_TEST")?18.f:5.8f;
   jumpActive=1;jumpCount++;jumpCooldown=8;
   if(logfile){fprintf(logfile,"jump takeoff=%d ground=%.2f ceiling=%.2f\n",jumpCount,ground,ceiling);fflush(logfile);}
  }else if(logfile){fprintf(logfile,"jump blocked: insufficient standing headroom\n");fflush(logfile);}
 }
 if(jumpActive&&PF(152)>0){
  float rise=fmaxf(0,PF(152)*dt-.5f*.27777779f*dt*dt);
  float head=fmaxf(PF(13940),120.f)+14.f;
  float ceiling=jumpCeiling(PF(140)+head+rise+3);
  if(ceiling<=PF(140)+head+rise+2){
   if(getenv("GE32_PAD_TEST")&&logfile){fprintf(logfile,"pad ceiling stop: foot=%.2f head=%.2f ceiling=%.2f xz=%.2f %.2f\n",PF(140),head,ceiling,PF(1864),PF(1872));fflush(logfile);}
   PF(152)=0;ceilingStops++;
   /* Do not move the player upward into a ceiling or lower them through a floor. */
   if(isfinite(ceiling))PF(140)=fmaxf(ground,fminf(PF(140),ceiling-head-2));
  }
 }
}
static void jumpAfterVertical(void){
 if(jumpActive&&PF(140)<=PF(144)+.1f&&PF(152)<=0){jumpActive=0;if(logfile){fprintf(logfile,"jump landed=%d\n",jumpCount);fflush(logfile);}}
}
static void jumpTestAfter(void);
static void dropPrepareVertical(int*,float*);
static void dropAfterVertical(void);
static void padBeforeVertical(void);
static void padPrepareVertical(int*,float*);
static void padAfterVertical(void);
static int introTestBlocks,introTestPassed;
static void verticalHook(int useHeight,float offset){
 int local=NUMBER==localIndex()&&CURRENT&&CURRENT==PLAYERS[localIndex()];
 if(local){
  int introTest=getenv("GE32_INTRO_TEST")&&!jumpAllowed();
  float oldFoot=PF(140),oldVelocity=PF(152);int oldCount=jumpCount;
  if(introTest)InterlockedExchange(&pendingJump,1);
  padBeforeVertical();jumpBeforeVertical();
  if(introTest){
   if(PF(140)!=oldFoot||PF(152)!=oldVelocity||jumpCount!=oldCount||pendingJump){if(logfile){fprintf(logfile,"FAIL intro jump changed physics or retained request\n");fflush(logfile);}ExitProcess(2);}
   introTestBlocks++;
  }else if(getenv("GE32_INTRO_TEST")&&jumpAllowed()&&!introTestPassed){
   if(!introTestBlocks||jumpCount){if(logfile){fprintf(logfile,"FAIL intro test lacked blocked intro ticks\n");fflush(logfile);}ExitProcess(2);}
   introTestPassed=1;if(logfile){fprintf(logfile,"PASS intro: %d queued jump attempts consumed without physics changes; gameplay starts with no queued jump\n",introTestBlocks);fflush(logfile);}
  }
  dropPrepareVertical(&useHeight,&offset);padPrepareVertical(&useHeight,&offset);
 }
 originalVertical(useHeight,offset);
 if(local){jumpAfterVertical();dropAfterVertical();padAfterVertical();jumpTestAfter();}
}
static int installJumpHook(void){
 unsigned char expected[]={0x55,0x53,0x48,0x83,0xec,0x78,0x48,0x8d,0x6c,0x24,0x60,0x0f,0x11,0x75,0x00};
 unsigned char*target=(void*)(base+0x15c10);if(memcmp(target,expected,15))return 0;
 unsigned char*t=VirtualAlloc(NULL,64,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!t)return 0;
 memcpy(t,target,15);t[15]=0x48;t[16]=0xb8;*(uintptr_t*)(t+17)=(uintptr_t)(target+15);t[25]=0xff;t[26]=0xe0;
 DWORD prot;VirtualProtect(t,64,PAGE_EXECUTE_READ,&prot);originalVertical=(void*)t;
 unsigned char j[15]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0,0x90,0x90,0x90};*(uintptr_t*)(j+2)=(uintptr_t)verticalHook;
 VirtualProtect(target,15,PAGE_EXECUTE_READWRITE,&prot);memcpy(target,j,15);VirtualProtect(target,15,prot,&prot);FlushInstructionCache(GetCurrentProcess(),target,15);return 1;
}
static void testJumpInput(void){
 volatile LONG latch=1;pendingJump=0;jumpAHeld=0;
 if(jumpA(1,1,&latch)||latch||pendingJump!=1)ExitProcess(3);
 pendingJump=0;if(jumpA(1,1,&latch)||pendingJump)ExitProcess(3);
 jumpA(0,1,&latch);jumpA(1,1,&latch);if(pendingJump!=1)ExitProcess(3);
 pendingJump=0;jumpA(0,0,&latch);if(jumpA(1,0,&latch)!=1||pendingJump)ExitProcess(3);
 jumpA(0,0,&latch);jumpA(1,0,&latch);jumpA(1,1,&latch);if(pendingJump)ExitProcess(3);
 jumpA(0,1,&latch);jumpA(1,1,&latch);if(pendingJump!=1)ExitProcess(3);
 jumpA(0,0,&latch);jumpAHeld=0;pendingJump=0;
 if(logfile){fprintf(logfile,"PASS: A queues once, clears crouch, holding does not repeat, release rearms, menus retain A\n");fflush(logfile);}
}
static void jumpTestAfter(void){
 if(!getenv("GE32_JUMP_TEST"))return;
 static int ticks,completed,startCount,phase,phaseTicks;static float peak,startY,startX,startZ,maxMotion;
 if(++ticks>2400){if(logfile){fprintf(logfile,"FAIL: jump integration timed out phase=%d allowed=%d foot=%.2f ground=%.2f\n",phase,jumpAllowed(),PF(140),PF(144));fflush(logfile);}ExitProcess(2);}
 if(!jumpAllowed())return;
 phaseTicks++;
 if(phase==0&&phaseTicks>40&&fabsf(PF(140)-PF(144))<.2f){
  phase=1;phaseTicks=0;startCount=jumpCount;peak=0;startY=PF(140);startX=PF(1864);startZ=PF(1872);
  volatile LONG*latch=(void*)(base+0x303000+1983488);jumpA(0,1,latch);jumpA(1,1,latch);
 }
 if(phase==1){
  if(getenv("GE32_JUMP_MOVING_TEST"))jumpTestMotion=phaseTicks<12?8:phaseTicks<24?4:0;
  peak=fmaxf(peak,PF(140)-startY);maxMotion=fmaxf(maxMotion,hypotf(PF(1864)-startX,PF(1872)-startZ));
  if(phaseTicks==8){/* Fresh airborne A must not produce a second takeoff. */volatile LONG*latch=(void*)(base+0x303000+1983488);jumpA(0,1,latch);jumpA(1,1,latch);}
  if(phaseTicks>15&&!jumpActive&&jumpCount>startCount){
   if(jumpCount!=startCount+1||peak<2||!isfinite(PF(140))){if(logfile){fprintf(logfile,"FAIL: count/height after jump\n");fflush(logfile);}ExitProcess(2);}
   completed++;jumpTestMotion=0;if(logfile){fprintf(logfile,"TEST jump %d peak=%.2f native landed=%.2f ceiling stops=%d\n",completed,peak,PF(140)-PF(144),ceilingStops);fflush(logfile);}phase=2;phaseTicks=0;
  }
 }
 if(phase==2&&phaseTicks>30){
  if(jumpCount!=startCount+1){if(logfile){fprintf(logfile,"FAIL: held A repeated after landing\n");fflush(logfile);}ExitProcess(2);}
  if(completed==8){
   if(getenv("GE32_JUMP_CEILING_TEST")){
    float foot=PF(140),ground=PF(144),vel=PF(152),cool=jumpCooldown;
    float head=fmaxf(PF(13940),120.f)+14.f,ceiling=jumpCeiling(foot+head+1000);
    int countBefore=jumpCount;
    if(!isfinite(ceiling)){if(logfile){fprintf(logfile,"FAIL: missing low-clearance fixture ceiling\n");fflush(logfile);}ExitProcess(2);}
    /* A simulation-thread fixture positions only the test's vertical state
     * immediately below that real ceiling, calls the real takeoff guard,
     * then restores it before any further native movement/body update. */
    PF(140)=PF(144)=ceiling-head-1;PF(152)=0;jumpActive=0;jumpCooldown=0;pendingJump=1;
    jumpBeforeVertical();
    if(jumpCount!=countBefore||jumpActive){if(logfile){fprintf(logfile,"FAIL: low-clearance takeoff accepted\n");fflush(logfile);}ExitProcess(2);}
    PF(140)=foot;PF(144)=ground;PF(152)=vel;jumpCooldown=cool;
    if(logfile){fprintf(logfile,"PASS: takeoff rejected under a real low-clearance ceiling fixture\n");fflush(logfile);}
   }
if(getenv("GE32_JUMP_MOVING_TEST")&&maxMotion<.5f){if(logfile){fprintf(logfile,"FAIL: moving test had no displacement\n");fflush(logfile);}ExitProcess(2);}if(logfile){fprintf(logfile,"TEST maximum horizontal displacement=%.2f\n",maxMotion);fflush(logfile);}if(getenv("GE32_JUMP_CEILING_TEST")&&ceilingStops==0){if(logfile){fprintf(logfile,"FAIL: ceiling stress test did not contact a ceiling\n");fflush(logfile);}ExitProcess(2);}if(logfile){fprintf(logfile,"PASS: 8 native-physics jumps and landings, airborne A rejected, held A does not repeat, finite grounded state\n");fflush(logfile);}ExitProcess(0);}
  phase=0;phaseTicks=0;
 }
}
