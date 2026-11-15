/* Deployable jump pad. All world/physics state belongs to the simulation thread. */
#define PAD_ITEM 26
#define PAD_RADIUS 48.f
#define PAD_IMPULSE 26.f
static volatile LONG padCarrying,padPlaceRequest,padTriggerBlock,padCancelRequest;
static int padRTHeld;
static struct {void*owner;int stage,timer,placed,valid,flight,launches,deploys;float cooldown,pulse;JumpVec position,preview;void*tile;} gadget;
typedef struct {int carrying,valid,placed,visible,previewVisible;float face[24][2],ghost[24][2],pulse;} PadSnapshot;
static PadSnapshot padPublished;
static SRWLOCK padLock=SRWLOCK_INIT;
static PadSnapshot padSnapshot(void){PadSnapshot s;AcquireSRWLockShared(&padLock);s=padPublished;ReleaseSRWLockShared(&padLock);return s;}
static void padStow(void){InterlockedExchange(&padCarrying,0);gadget.valid=0;FN(void(*)(int,int),0x1000+458624)(0x40000000,1);}
static void padEquip(void){if(!jumpAllowed()||slap.phase)return;InterlockedExchange(&padCarrying,1);sprintLatched=sprintOn=0;sprintBlend=0;InterlockedExchange(&slapRequests,0);FN(void(*)(int,int),0x1000+458624)(0x40000000,0);}
static void padContext(void){
 int stage=*(int*)(base+0x303000+211452),timer=*(int*)(base+0x303000+211432);
 if(gadget.owner!=CURRENT||gadget.stage!=stage||timer<gadget.timer){padStow();memset(&gadget,0,sizeof(gadget));gadget.owner=CURRENT;gadget.stage=stage;padCarrying=0;padPlaceRequest=0;padCancelRequest=0;}
 gadget.timer=timer;if(PI(256)){padStow();gadget.flight=0;}
}
static int padFloor(JumpVec at,void**tile){
 float y;void*t=floorBelow(at.x,at.y+24,at.z,&y);if(!t||!isfinite(y)||fabsf(y-at.y)>24)return 0;
 static const float ring[8][2]={{46,0},{-46,0},{0,46},{0,-46},{33,33},{33,-33},{-33,33},{-33,-33}};
 for(int i=0;i<8;i++){float h;void*f=floorBelow(at.x+ring[i][0],y+4,at.z+ring[i][1],&h);if(!f||!isfinite(h)||fabsf(h-y)>3)return 0;}
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
  padPreview();if(gadget.valid){gadget.position=gadget.preview;gadget.placed=1;gadget.cooldown=18;gadget.deploys++;padStow();if(logfile){fprintf(logfile,"pad placed=%d at %.2f %.2f %.2f\n",gadget.deploys,gadget.position.x,gadget.position.y,gadget.position.z);fflush(logfile);}}
 }
}
static void padBeforeVertical(void){
 if(!gadget.placed||gadget.owner!=CURRENT||!jumpAllowed())return;
 float dt=*(float*)(base+0x303000+211436);if(!isfinite(dt)||dt<0)return;
 gadget.cooldown=fmaxf(0,gadget.cooldown-dt);gadget.pulse=fmaxf(0,gadget.pulse-dt/36.f);
 float d=hypotf(PF(1864)-gadget.position.x,PF(1872)-gadget.position.z);
 if(gadget.cooldown<=0&&d<PAD_RADIUS+8&&fabsf(PF(140)-gadget.position.y)<8&&PF(152)<=.6f&&fabsf(PF(140)-PF(144))<2){
  float head=fmaxf(PF(13940),120.f)+14.f;
  if(jumpCeiling(PF(140)+head+80)>PF(140)+head+8){
   jumpOriginFloor=PF(144);PI(184)=PI(14004)=2;PF(140)+=.05f;PF(152)=PAD_IMPULSE;jumpActive=1;jumpCooldown=12;gadget.flight=1;gadget.cooldown=45;gadget.pulse=1;gadget.launches++;
   if(logfile){fprintf(logfile,"pad launch=%d foot=%.2f impulse=%.2f\n",gadget.launches,PF(140),PF(152));fflush(logfile);}
  }else gadget.cooldown=20;
 }
}
static int padTryUpper(JumpVec*to,void**tile,int native){
 if(!gadget.flight||NUMBER!=localIndex()||!jumpAllowed())return native;
 float dx=to->x-PF(1864),dz=to->z-PF(1872),len=hypotf(dx,dz);if(len<.001f||len>128)return native;
 float upper;void*t=floorBelow(to->x+dx/len*68,PF(140)+1,to->z+dz/len*68,&upper);
 if(!t||!isfinite(upper)||upper<=gadget.position.y+16||PF(140)<upper+2)return native;
 if(!dropBodyClear(to))return 0;
 float y;void*dest=floorBelow(to->x,PF(140)+1,to->z,&y);if(!dest||!isfinite(y))return native;
 *tile=dest;FN(void(*)(void),0x1000+876000)();return 1;
}
static void padPrepareVertical(int*useHeight,float*offset){
 if(!gadget.flight||!jumpAllowed())return;
 int uniform;float support=traversalFloorSupport(&uniform);
 if(isfinite(support)){PF(144)=support;*useHeight=1;*offset=0;}
}
static void padAfterVertical(void){if(gadget.flight&&PF(140)<=PF(144)+.2f&&PF(152)<=0){gadget.flight=0;if(logfile){fprintf(logfile,"pad landed floor=%.2f\n",PF(144));fflush(logfile);}}}
static int padProject(JumpVec p,float*out){
 void*m=FN(void*(*)(void),0x1000+53376)();if(!m)return 0;FN(void(*)(void*,JumpVec*),0x1000+539920)(m,&p);
 float fov=FN(float(*)(void),0x1000+53952)(),aspect=FN(float(*)(void),0x1000+53984)();
 if(p.z>=-3||fov<5||fov>160||aspect<.2f||!isfinite(p.z))return 0;
 float t=tanf(fov*.00872664626f),width=FN(float(*)(void),0x1000+53824)(),height=FN(float(*)(void),0x1000+53856)(),left=FN(float(*)(void),0x1000+53888)(),top=FN(float(*)(void),0x1000+53920)();
 int fullWidth=FN(int(*)(void),0x1000+1703904)(),fullHeight=FN(int(*)(void),0x1000+1703920)();if(fullWidth<1||fullHeight<1)return 0;
 out[0]=(left+width*(.5f+p.x/(-p.z*t*aspect)*.5f))/fullWidth;out[1]=(top+height*(.5f-p.y/(-p.z*t)*.5f))/fullHeight;return isfinite(out[0])&&isfinite(out[1]);
}
static int padProjectFace(JumpVec c,float out[24][2]){
 float*view=FN(float*(*)(void),0x1000+53600)();JumpVec eye=view?(JumpVec){view[12],view[13],view[14]}:(JumpVec){PF(1864),PF(1868),PF(1872)};JumpVec center={c.x,c.y+8,c.z};
 if(!dropSegmentClear(eye,center))return 0;
 for(int i=0;i<8;i++){float a=6.28318530718f*(i+.5f)/8;JumpVec p={c.x+cosf(a)*PAD_RADIUS,c.y+6,c.z+sinf(a)*PAD_RADIUS};if(!padProject(p,out[i]))return 0;p.y=c.y+1;if(!padProject(p,out[i+8]))return 0;p.x=c.x+cosf(a)*PAD_RADIUS*.68f;p.z=c.z+sinf(a)*PAD_RADIUS*.68f;p.y=c.y+7;if(!padProject(p,out[i+16]))return 0;}
 return 1;
}
static void padPublish(void){
 PadSnapshot s={0};s.carrying=padCarrying&&jumpAllowed();s.valid=gadget.valid;s.placed=gadget.placed;s.pulse=gadget.pulse;
 if(jumpAllowed()){if(gadget.placed)s.visible=padProjectFace(gadget.position,s.face);if(s.carrying){padPreview();s.valid=gadget.valid;s.previewVisible=padProjectFace(gadget.preview,s.ghost);}}

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
static void padGunRender(void**gdl){if(padCarrying&&NUMBER==localIndex()&&jumpAllowed())return;padOriginalGunRender(gdl);}
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
