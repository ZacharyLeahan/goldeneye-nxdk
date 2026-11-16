/* Sprint and quick slap. Simulation-only player mutations; input callbacks
 * only queue edge events. Native collision, gait, hit testing and damage stay in use. */
#define HI(hand,off) (*(int*)((unsigned char*)CURRENT+3208+(hand)*1976+(off)))
#define HB(hand,off) (*(signed char*)((unsigned char*)CURRENT+3208+(hand)*1976+(off)))
static volatile LONG sprintClicks,slapRequests;
static int combatButtonHeld[2],sprintLatched,sprintOn;
#define MOVE_RUN_SCALE 1.15f
#define MOVE_SPRINT_RATIO 1.46f /* native gait is nonlinear; calibrated in-engine */
static float sprintBlend;
static void*combatPlayer;
static struct {
 int phase,saved[2],clip[2],previous[2],priorName,seenStrike,deferredWeapon,slaps,events;
 float elapsed,cooldown;
} slap;
static unsigned char combatButton(int index,unsigned char raw,int allowed){
 int slot=index==7?0:1;
 if(raw&&!combatButtonHeld[slot]&&allowed){
  if(index==7)InterlockedIncrement(&sprintClicks);else InterlockedExchange(&slapRequests,1);
 }
 combatButtonHeld[slot]=raw!=0;
 return allowed?0:raw;
}
static void handReset(int h){HI(h,36)=0;HI(h,40)=0;HI(h,68)=0;HI(h,28)=0;HI(h,32)=0;HB(h,12)=0;}
static int ammoType(int weapon){return FN(int(*)(int),0x1000+473360)(weapon);}
static int *reserveAmmo(int type){return (int*)((unsigned char*)CURRENT+7576+type*4);}
static void swapNow(int hand,int weapon){FN(void(*)(int,int),0x1000+422288)(hand,weapon);handReset(hand);}
static void restoreSlap(void){
 int deferred=slap.deferredWeapon;
 swapNow(0,slap.saved[0]);swapNow(1,slap.saved[1]);
 for(int h=0;h<2;h++){
  int type=ammoType(slap.saved[h]);
  if(type>0&&type<33&&slap.clip[h]>0){int take=slap.clip[h];if(take>*reserveAmmo(type))take=*reserveAmmo(type);if(take<0)take=0;*reserveAmmo(type)-=take;HI(h,44)=take;}
 }
 for(int h=0;h<2;h++)HI(h,8)=slap.previous[h];
 PI(14072)=slap.priorName;
 slap.phase=0;slap.cooldown=12;slap.elapsed=0;
 if(deferred>=4)InterlockedExchange(&pendingWeapon,deferred);
 if(logfile){fprintf(logfile,"slap return right=%d left=%d clips=%d/%d\n",HI(0,0),HI(1,0),HI(0,44),HI(1,44));fflush(logfile);}
}
static void beginSlap(void){
 for(int h=0;h<2;h++){slap.saved[h]=HI(h,0);slap.clip[h]=HI(h,44);slap.previous[h]=HI(h,8);}
 slap.priorName=PI(14072);slap.deferredWeapon=-1;slap.seenStrike=0;slap.elapsed=0;
 PI(14072)=1;swapNow(0,1);swapNow(1,0);slap.phase=1;
 sprintLatched=0;sprintBlend=0;
 if(logfile){fprintf(logfile,"slap begin previous=%d/%d clips=%d/%d\n",slap.saved[0],slap.saved[1],slap.clip[0],slap.clip[1]);fflush(logfile);}
}
static void combatBefore(unsigned short *buttons,unsigned short *oldbuttons){
 if(combatPlayer!=CURRENT){combatPlayer=CURRENT;sprintLatched=sprintOn=0;sprintBlend=0;slap.phase=0;slap.cooldown=0;InterlockedExchange(&sprintClicks,0);InterlockedExchange(&slapRequests,0);}
 float dt=*(float*)(base+0x303000+211436);if(!isfinite(dt)||dt<0)dt=0;
 slap.cooldown=fmaxf(0,slap.cooldown-dt);
 int allowed=jumpAllowed(),request=InterlockedExchange(&slapRequests,0),clicks=InterlockedExchange(&sprintClicks,0);
 if(!allowed||openWheel){sprintLatched=0;sprintBlend=0;}
 else if(clicks&1)sprintLatched=!sprintLatched;
 if(PI(256)||*(int*)(base+0x303000+65912)){slap.phase=0;sprintLatched=0;return;}
 if(!allowed)return; /* pause/cinematic freezes phase instead of restoring through it */
 if(request&&!openWheel&&!slap.phase&&slap.cooldown<=0){
  int weapon=HI(0,0);
  if(weapon>=1&&weapon<=25&&HI(0,4)<0&&HI(1,4)<0&&HI(0,36)<5)beginSlap();
 }
 if(slap.phase){
  *buttons&=~(0x2000|0x10|0x20);*oldbuttons&=~(0x2000|0x10|0x20);
  PI(14072)=1;slap.elapsed+=dt;
  if(slap.phase==1&&PI(3008)==1&&PI(3000)>=0){
   HI(0,36)=30;HI(0,40)=0;HI(0,28)=HI(0,32)=0;HB(0,12)=0;
   slap.phase=2;slap.seenStrike=0;slap.elapsed=0;slap.slaps++;
   if(logfile){fprintf(logfile,"native slap animation=%d\n",slap.slaps);fflush(logfile);}
  }else if(slap.phase==2){
   if(HI(0,36)>=30&&HI(0,36)<=33)slap.seenStrike=1;
   if(slap.seenStrike&&HI(0,36)==0){restoreSlap();}
  }
  if(slap.phase&&slap.elapsed>120){if(logfile){fprintf(logfile,"slap timed out; restoring gun\n");fflush(logfile);}restoreSlap();}
 }
}
static void combatAfter(unsigned short rawButtons){
 if(slap.phase==2&&HB(0,12)){slap.events++;if(logfile){fprintf(logfile,"native slap strike event=%d item=%d\n",slap.events,HI(0,0));fflush(logfile);}}
 /* Normalize diagonal input before speed shaping; never stack multipliers. */
 if(jumpAllowed()&&!PI(332)){float forward=PF(424)/1.08f,side=PF(416),length=hypotf(forward,side);if(length>1&&hypotf(PF(432),PF(440))<.001f){PF(424)/=length;PF(416)/=length;}PF(424)*=MOVE_RUN_SCALE;PF(416)*=MOVE_RUN_SCALE;}
 int can=sprintLatched&&jumpAllowed()&&!openWheel&&!slap.phase&&
   PI(184)==2&&PI(14004)==2&&PF(424)>.15f&&!PI(332)&&!(rawButtons&(0x2000|0x10|0x20));
 if(!can){sprintLatched=0;sprintBlend=0;}
 else{float dt=*(float*)(base+0x303000+211436);sprintBlend=fminf(1,sprintBlend+fmaxf(0,dt)/9.f);}
 int on=can&&sprintBlend>0;
 if(on){float factor=1+(MOVE_SPRINT_RATIO-1)*sprintBlend;PF(424)*=factor;PF(416)*=factor;}
 if(on!=sprintOn){sprintOn=on;if(logfile){fprintf(logfile,"sprint %s\n",on?"on":"off");fflush(logfile);}}
}
static int combatDeferWeapon(int wanted){if(!slap.phase)return 0;slap.deferredWeapon=wanted;return 1;}
static void testCombatInput(void){
 sprintClicks=slapRequests=0;combatButtonHeld[0]=combatButtonHeld[1]=0;
 combatButton(7,1,1);combatButton(7,1,1);if(sprintClicks!=1)ExitProcess(3);
 combatButton(7,0,1);combatButton(7,1,1);if(sprintClicks!=2)ExitProcess(3);
 combatButton(8,1,1);if(slapRequests!=1)ExitProcess(3);slapRequests=0;
 combatButton(8,1,1);if(slapRequests)ExitProcess(3);
 combatButton(8,0,0);if(combatButton(8,1,0)!=1||slapRequests)ExitProcess(3);
 sprintClicks=slapRequests=0;combatButtonHeld[0]=combatButtonHeld[1]=0;
 if(logfile){fprintf(logfile,"PASS: sprint/slap press edges, held-button suppression, menu passthrough\n");fflush(logfile);}
}
