/* Deployable jump pad. All world/physics state belongs to the simulation thread. */
#define PAD_ITEM 128
#define PAD_RADIUS 78.f
#define PAD_IMPULSE 26.f
static volatile LONG padCarrying,padPlaceRequest,padTriggerBlock,padCancelRequest;
static int padRTHeld,portalClampInstalled,portalClampSkips;
static struct {void*owner;int timer;JumpVec position;} portalStep;
static struct {void*owner;int stage,timer,placed,valid,flight,airPortal,portalBridge,launches,deploys;float cooldown,pulse,lastContactDistance;JumpVec position,preview;void*tile;} gadget;
typedef struct {int carrying,valid,placed,visible,previewVisible;float face[24][3],ghost[24][3],pulse,carryBlend;} PadSnapshot;
static PadSnapshot padPublished;
static float padCarryBlend;
static SRWLOCK padLock=SRWLOCK_INIT;
static PadSnapshot padSnapshot(void){PadSnapshot s;AcquireSRWLockShared(&padLock);s=padPublished;ReleaseSRWLockShared(&padLock);return s;}
static void padStow(void){InterlockedExchange(&padCarrying,0);gadget.valid=0;FN(void(*)(int,int),0x1000+458624)(0x40000000,1);}
static void padEquip(void){if(!jumpAllowed()||slap.phase)return;InterlockedExchange(&padCarrying,1);sprintLatched=sprintOn=0;sprintBlend=0;InterlockedExchange(&slapRequests,0);FN(void(*)(int,int),0x1000+458624)(0x40000000,0);}
static void padContext(void){
 int stage=*(int*)(base+0x303000+211452),timer=*(int*)(base+0x303000+211432);
 if(gadget.owner!=CURRENT||gadget.stage!=stage||timer<gadget.timer){padStow();padCarryBlend=0;memset(&gadget,0,sizeof(gadget));gadget.owner=CURRENT;gadget.stage=stage;padCarrying=0;padPlaceRequest=0;padCancelRequest=0;}
 gadget.timer=timer;if(PI(256)){padStow();padCarryBlend=0;gadget.flight=0;}
}
static int padFloor(JumpVec at,void**tile){
 float y;void*t=floorBelow(at.x,at.y+24,at.z,&y);if(!t||!isfinite(y)||fabsf(y-at.y)>24)return 0;
 for(int i=0;i<16;i++){float a=i*.3926990817f,dx=cosf(a)*PAD_RADIUS,dz=sinf(a)*PAD_RADIUS,h;void*f=floorBelow(at.x+dx,y+4,at.z+dz,&h);if(!f||!isfinite(h)||fabsf(h-y)>3)return 0;
  if(!dropSegmentClear((JumpVec){at.x,y+14,at.z},(JumpVec){at.x+dx,y+14,at.z+dz}))return 0;
 }
 int r=FN(int(*)(void*),0x1000+883888)(t),total=*(int*)(base+0x228000+4208);if(r<0||r>=total||total>2048||!jumpRoomReady(r))return 0;
 *tile=t;gadget.preview=(JumpVec){at.x,y,at.z};return 1;
}
static void padPreview(void){
 gadget.valid=0;float dx=PF(1876),dz=PF(1884),len=hypotf(dx,dz);if(len<.5f)return;
 float distance=220.f,pitch=PF(396);if(isfinite(pitch)&&pitch<-10.f)distance=fmaxf(75.f,fminf(240.f,fmaxf(40.f,PF(1868)-PF(140))/tanf(fminf(89.f,-pitch)*.01745329252f)));
 JumpVec p={PF(1864)+dx/len*distance,PF(140),PF(1872)+dz/len*distance};gadget.preview=p;
 if(!jumpAllowed()||fabsf(PF(140)-PF(144))>2||fabsf(PF(152))>.6f)return;
 void*t=NULL;if(!padFloor(p,&t))return;
 JumpVec a={PF(1864),PF(140)+35,PF(1872)},b={gadget.preview.x,gadget.preview.y+35,gadget.preview.z};
 if(!dropSegmentClear(a,b))return;gadget.tile=t;gadget.valid=1;
}
static void padBefore(unsigned short*buttons,unsigned short*oldbuttons){
 padContext();if(InterlockedExchange(&padCancelRequest,0))padStow();
 if(padCarrying){*buttons&=~(0x2000|0x10|0x20);*oldbuttons&=~(0x2000|0x10|0x20);sprintLatched=0;sprintBlend=0;InterlockedExchange(&slapRequests,0);padPreview();}
 if(padTriggerBlock){*buttons&=~0x2000;*oldbuttons&=~0x2000;}
 if(InterlockedExchange(&padPlaceRequest,0)&&padCarrying&&jumpAllowed()&&!openWheel){
  padPreview();if(gadget.valid){gadget.position=gadget.preview;gadget.placed=1;gadget.cooldown=0;gadget.deploys++;padStow();if(logfile){fprintf(logfile,"pad placed=%d at %.2f %.2f %.2f\n",gadget.deploys,gadget.position.x,gadget.position.y,gadget.position.z);fflush(logfile);}}
 }
}
/* Contact matches the visible octagon, with a small foot-overlap allowance. */
static int padContact(float dx,float dz){for(int i=0;i<8;i++){float a=i*.7853981634f;if(dx*cosf(a)+dz*sinf(a)>PAD_RADIUS*.9238795325f+8)return 0;}return 1;}
static void padBeforeVertical(void){
 if(!gadget.placed||gadget.owner!=CURRENT||!jumpAllowed())return;
 float dt=*(float*)(base+0x303000+211436);if(!isfinite(dt)||dt<0)return;
 gadget.cooldown=fmaxf(0,gadget.cooldown-dt);gadget.pulse=fmaxf(0,gadget.pulse-dt/18.f);
 int touching=padContact(PF(1864)-gadget.position.x,PF(1872)-gadget.position.z);
 if(gadget.cooldown<=0&&touching&&fabsf(PF(140)-gadget.position.y)<8&&PF(152)<=.6f&&fabsf(PF(140)-PF(144))<2){
  float head=fmaxf(PF(13940),120.f)+14.f;
  if(jumpCeiling(PF(140)+head+80)>PF(140)+head+8){
   jumpOriginFloor=PF(144);PI(184)=PI(14004)=2;PF(140)+=.05f;PF(152)=PAD_IMPULSE;jumpActive=1;jumpCooldown=12;gadget.flight=1;gadget.airPortal=gadget.portalBridge=0;gadget.cooldown=45;gadget.pulse=1;gadget.lastContactDistance=hypotf(PF(1864)-gadget.position.x,PF(1872)-gadget.position.z);gadget.launches++;
   if(logfile){fprintf(logfile,"pad launch=%d foot=%.2f impulse=%.2f\n",gadget.launches,PF(140),PF(152));fflush(logfile);}
  }else gadget.cooldown=20;
 }
}
/* Find actual static support under an un-navigable aperture. A distant
 * navigation anchor must never create an invisible floor under the player. */
static int padBackgroundFloor(float x,float foot,float z,float*height){
 if(!collisionPrepareBox((JumpVec){x,foot-2048,z},(JumpVec){x,foot+.5f,z}))return 0;
 int total=*(int*)(base+0x228000+4208);float scale=*(float*)(base+0x228000+4252),best=-INFINITY;if(total<1||total>2048||!isfinite(scale)||scale<=0)return 0;
 JumpVec a={x,foot+.5f,z},b={x,foot-2048,z};for(int room=1;room<total;room++){if(!*(void**)(base+0x303000+27264+room*0x68+0x40))continue;JumpHit hit={0};if(FN(int(*)(JumpVec*,JumpVec*,int,JumpHit*),0x1000+13744)(&a,&b,room,&hit)){float y=hit.position.y/scale;if(isfinite(y)&&y<=foot+.6f&&y>=foot-2048&&y>best)best=y;}}
 if(!isfinite(best))return 0;*height=best;return 1;
}
static int padTryAirborne(JumpVec*to,void**tile,int native){
 if(!gadget.flight||NUMBER!=localIndex()||!jumpAllowed())return native;
 float dx=to->x-PF(1864),dz=to->z-PF(1872),len=hypotf(dx,dz);if(!isfinite(len)||len<.001f||len>128)return native;
 float y;void*dest=floorBelow(to->x,PF(140)+1,to->z,&y);int bridge=dest==NULL;
 if(!dest){
  for(float distance=32;distance<=640;distance+=32){float h;void*t=floorBelow(to->x+dx/len*distance,PF(140)+1,to->z+dz/len*distance,&h);if(t&&isfinite(h)&&h<=PF(140)-2&&PF(140)-h<2048&&FN(int(*)(void*,float,float),0xd5e10)(t,to->x+dx/len*distance,to->z+dz/len*distance)){dest=t;y=h;break;}}
 }
 if(!dest||!isfinite(y)||PF(140)<y+2)return native;
 if(bridge){float support;if(!padBackgroundFloor(to->x,PF(140),to->z,&support))return native;}
 if((!portalClampInstalled||getenv("GE32_OPENING_BASELINE"))&&(bridge||y<=gadget.position.y+16))return native;
 if(!dropBodyClear(to))return 0;
 if(!native||bridge)gadget.airPortal=1;gadget.portalBridge=bridge;portalStep.owner=CURRENT;portalStep.timer=*(int*)(base+0x303000+211432);portalStep.position=*to;
 *tile=dest;FN(void(*)(void),0x1000+876000)();return 1;
}
/* A high window sill has BG geometry but may have no walkable STAN tile.
 * Retain physical foot support while straddling it instead of falling through. */
static float padPortalSupport(float support){
 if(!gadget.airPortal)return support;float foot=PF(140);if(!isfinite(foot)||foot<support)return support;
 float scale=*(float*)(base+0x228000+4252);int total=*(int*)(base+0x228000+4208);if(!isfinite(scale)||scale<=0||total<1||total>2048)return support;
 collisionPrepareBox((JumpVec){PF(1864)-31,support,PF(1872)-31},(JumpVec){PF(1864)+31,foot+1,PF(1872)+31});
 for(int point=0;point<17;point++){float dx=0,dz=0;if(point){float angle=(point-1)*.3926990817f;dx=31*cosf(angle);dz=31*sinf(angle);}JumpVec a={PF(1864)+dx,foot+.5f,PF(1872)+dz},b={a.x,support-.5f,a.z};
  for(int room=1;room<total;room++){if(!*(void**)(base+0x303000+27264+room*0x68+0x40))continue;JumpHit hit={0};if(FN(int(*)(JumpVec*,JumpVec*,int,JumpHit*),0x1000+13744)(&a,&b,room,&hit)){float y=hit.position.y/scale;if(isfinite(y)&&y<=foot+.6f&&y>support)support=y;}}
 }
 return support;
}
static void padPrepareVertical(int*useHeight,float*offset){
 if(!gadget.flight||!jumpAllowed())return;
 int uniform;float support=traversalFloorSupport(&uniform);
 if(!isfinite(support)&&gadget.airPortal)padBackgroundFloor(PF(1864),PF(140),PF(1872),&support);
 if(isfinite(support)){support=padPortalSupport(support);PF(144)=support;*useHeight=1;*offset=0;}
}
static void padAfterVertical(void){if(gadget.flight&&PF(140)<=PF(144)+.2f&&PF(152)<=0){int uniform;float nativeSupport=traversalFloorSupport(&uniform);if(isfinite(nativeSupport)&&fabsf(PF(140)-nativeSupport)<1.5f){gadget.flight=gadget.airPortal=gadget.portalBridge=0;if(logfile){fprintf(logfile,"pad landed floor=%.2f\n",PF(144));fflush(logfile);}}}}
static int padProjectDepth(JumpVec p,float*out,float*depth){
 void*m=FN(void*(*)(void),0x1000+53376)();if(!m)return 0;FN(void(*)(void*,JumpVec*),0x1000+539920)(m,&p);
 float fov=FN(float(*)(void),0x1000+53952)(),aspect=FN(float(*)(void),0x1000+53984)();
 if(p.z>=-3||fov<5||fov>160||aspect<.2f||!isfinite(p.z))return 0;
 float t=tanf(fov*.00872664626f),width=FN(float(*)(void),0x1000+53824)(),height=FN(float(*)(void),0x1000+53856)(),left=FN(float(*)(void),0x1000+53888)(),top=FN(float(*)(void),0x1000+53920)();
 int fullWidth=FN(int(*)(void),0x1000+1703904)(),fullHeight=FN(int(*)(void),0x1000+1703920)();if(fullWidth<1||fullHeight<1)return 0;
 if(depth)*depth=-p.z;
 out[0]=(left+width*(.5f+p.x/(-p.z*t*aspect)*.5f))/fullWidth;out[1]=(top+height*(.5f-p.y/(-p.z*t)*.5f))/fullHeight;return isfinite(out[0])&&isfinite(out[1]);
}
static int padProject(JumpVec p,float*out){return padProjectDepth(p,out,NULL);}
static int padProjectFace(JumpVec c,float out[24][3],float compression){
 float*view=FN(float*(*)(void),0x1000+53600)();JumpVec eye=view?(JumpVec){view[12],view[13],view[14]}:(JumpVec){PF(1864),PF(1868),PF(1872)};JumpVec center={c.x,c.y+12,c.z};
 if(!dropSegmentClear(eye,center))return 0;
 for(int i=0;i<8;i++){float a=6.28318530718f*(i+.5f)/8;JumpVec p={c.x+cosf(a)*PAD_RADIUS,c.y+8,c.z+sinf(a)*PAD_RADIUS};if(!padProjectDepth(p,out[i],out[i]+2))return 0;p.y=c.y+1;p.x=c.x+cosf(a)*PAD_RADIUS*.97f;p.z=c.z+sinf(a)*PAD_RADIUS*.97f;if(!padProjectDepth(p,out[i+8],out[i+8]+2))return 0;p.x=c.x+cosf(a)*PAD_RADIUS*.72f;p.z=c.z+sinf(a)*PAD_RADIUS*.72f;p.y=c.y+12-5*compression;if(!padProjectDepth(p,out[i+16],out[i+16]+2))return 0;}
 return 1;
}
static void padPublish(void){
 float dt=*(float*)(base+0x303000+211436);if(!isfinite(dt)||dt<0)dt=0;float target=padCarrying&&jumpAllowed()?1.f:0.f;padCarryBlend=fminf(1,fmaxf(0,padCarryBlend+(target?1:-1)*dt/11.f));
 PadSnapshot s={0};s.carryBlend=padCarryBlend;s.carrying=padCarrying&&jumpAllowed();s.valid=gadget.valid;s.placed=gadget.placed;s.pulse=gadget.pulse;
 if(jumpAllowed()){if(gadget.placed)s.visible=padProjectFace(gadget.position,s.face,gadget.pulse);if(s.carrying){padPreview();s.valid=gadget.valid;s.previewVisible=padProjectFace(gadget.preview,s.ghost,0);}}

 AcquireSRWLockExclusive(&padLock);padPublished=s;ReleaseSRWLockExclusive(&padLock);
}
static void padTrigger(int rt,int allowed){
 if(!rt)InterlockedExchange(&padTriggerBlock,0);
 if(rt&&!padRTHeld&&padCarrying&&allowed){InterlockedExchange(&padPlaceRequest,1);InterlockedExchange(&padTriggerBlock,1);}
 padRTHeld=rt;
}
static void padPoll(void){
 if(!pad)return;int rt=getAxis(pad,5)>10000;InventorySnapshot s=snapshotRead();
 padTrigger(rt,s.allowJump&&!openWheel);
 if(padCarrying&&s.allowJump&&!openWheel&&getButton(pad,1))InterlockedExchange(&padCancelRequest,1);
}
static void (*padOriginalGunRender)(void**);
static void padGunRender(void**gdl){if((padCarrying||padCarryBlend>.05f)&&NUMBER==localIndex()&&jumpAllowed())return;padOriginalGunRender(gdl);}
static int installPadGunHook(void){
 unsigned char expected[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x57,0x56,0x53,0x48,0x81,0xec,0xc8,0,0,0};
 unsigned char*target=(void*)(base+0x1000+435024);if(memcmp(target,expected,19))return 0;
 unsigned char*t=VirtualAlloc(NULL,64,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!t)return 0;memcpy(t,target,19);t[19]=0x48;t[20]=0xb8;*(uintptr_t*)(t+21)=(uintptr_t)(target+19);t[29]=0xff;t[30]=0xe0;
 DWORD prot;VirtualProtect(t,64,PAGE_EXECUTE_READ,&prot);padOriginalGunRender=(void*)t;
 unsigned char j[19]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0,0x90,0x90,0x90,0x90,0x90,0x90,0x90};*(uintptr_t*)(j+2)=(uintptr_t)padGunRender;
 VirtualProtect(target,19,PAGE_EXECUTE_READWRITE,&prot);memcpy(target,j,19);VirtualProtect(target,19,prot,&prot);FlushInstructionCache(GetCurrentProcess(),target,19);return 1;
}

static void testPadInput(void){
 padCarrying=1;padPlaceRequest=padTriggerBlock=padRTHeld=0;padTrigger(1,0);padTrigger(1,1);if(padPlaceRequest)ExitProcess(3);
 padTrigger(0,1);padTrigger(1,1);if(!padPlaceRequest||!padTriggerBlock)ExitProcess(3);padPlaceRequest=0;padTrigger(1,1);if(padPlaceRequest||!padTriggerBlock)ExitProcess(3);
 padCarrying=0;padTrigger(1,1);if(!padTriggerBlock)ExitProcess(3);padTrigger(0,1);if(padTriggerBlock)ExitProcess(3);padRTHeld=0;
 if(logfile){fprintf(logfile,"PASS pad trigger: fresh press, menu/intro suppression, held-trigger no repeat, gun fire blocked until release\n");fflush(logfile);}
}
