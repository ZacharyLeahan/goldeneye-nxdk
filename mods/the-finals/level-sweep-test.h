/* Opt-in offline map sweep. Fixtures never run in ordinary gameplay. */
static struct {int phase,tick,ready,fire,jumps,slides,slaps,launches,ammo,deploys,checks,samples;float yaw,peak;JumpVec anchor,start,previous;int previousValid,previousAir;} sweep;
static HealthTarget*sweepTarget;static int sweepHoldSeen,sweepFadeSeen,sweepSlideAttempts,sweepAimAttempts;
static void sweepCheck(int ok,const char*why){if(!ok){fprintf(logfile,"FAIL sweep phase=%d tick=%d stage=%d: %s x=%.2f y=%.2f z=%.2f floor=%.2f\n",sweep.phase,sweep.tick,*(int*)(base+0x303000+211452),why,PF(1864),PF(140),PF(1872),PF(144));fflush(logfile);ExitProcess(2);}sweep.checks++;}
static void sweepNote(const char*why){fprintf(logfile,"PASS sweep phase=%d: %s\n",sweep.phase,why);fflush(logfile);}
static void sweepReset(void){padStow();gadget.placed=gadget.flight=0;slideStop();slide.cooldown=0;sprintLatched=sprintOn=0;sprintBlend=0;PF(432)=PF(436)=PF(440)=0;padTestPlacePlayer(sweep.anchor.x,sweep.anchor.z,sweep.anchor.y+2,sweep.yaw);PF(396)=0;PF(416)=PF(420)=PF(424)=PF(14092)=0;PF(428)=1;*(JumpVec*)(base+3238208)=(JumpVec){0};sweep.previousValid=0;}
static int sweepSearchAt(float ox,float oz,float top,int maxRadius){
 for(int radius=0;radius<=maxRadius;radius+=150)for(int point=0;point<(radius?16:1);point++){
  float a=point*.3926990817f,x=ox+radius*cosf(a),z=oz+radius*sinf(a),floor;void*t=floorBelow(x,top,z,&floor);if(!t||!isfinite(floor)||floor<PF(140)-1600)continue;
  void*support=NULL;if(!padFloor((JumpVec){x,floor,z},&support))continue;
  int headClear=1;for(int p=0;p<17;p++){float px=x,pz=z;if(p){px+=31*cosf((p-1)*.3926990817f);pz+=31*sinf((p-1)*.3926990817f);}if(!dropSegmentClear((JumpVec){px,floor+8,pz},(JumpVec){px,floor+190,pz})){headClear=0;break;}}if(!headClear)continue;
  for(int direction=0;direction<8;direction++){float angle=direction*.7853981634f,dx=-sinf(angle),dz=cosf(angle),h;void*f=floorBelow(x+dx*220,floor+4,z+dz*220,&h);if(!f||fabsf(h-floor)>3||!padFloor((JumpVec){x+dx*220,h,z+dz*220},&support))continue;
   void*walk=t;if(!FN(int(*)(void**,float,float,float,float),0xd6870)(&walk,x,z,x+dx*360,z+dz*360))continue;
   int clear=1;for(int side=-1;side<=1;side++){JumpVec from={x+cosf(angle)*side*31,floor+8,z+sinf(angle)*side*31},to={from.x+dx*360,from.y,from.z+dz*360};if(!dropSegmentClear(from,to)){clear=0;break;}from.y=to.y=floor+175;if(!dropSegmentClear(from,to)){clear=0;break;}}
   if(!clear)continue;sweep.anchor=(JumpVec){x,floor,z};sweep.yaw=direction*45.f;return 1;
  }
 }
 return 0;
}

static int sweepFindSpace(void){
 if(getenv("GE32_SWEEP_ANCHOR")){sweepCheck(sscanf(getenv("GE32_SWEEP_ANCHOR"),"%f,%f,%f,%f",&sweep.anchor.x,&sweep.anchor.y,&sweep.anchor.z,&sweep.yaw)==4,"debug anchor parses");return 1;}
 if(sweepSearchAt(PF(1864),PF(1872),PF(140)+2048,4800))return 1;
 /* Boat starts and distant catwalks need actual floor candidates beyond the spawn. */
 int rooms=*(int*)(base+3573444);if(rooms<1||rooms>139)return 0;
 for(int room=0;room<rooms;room++){unsigned char*tile=((unsigned char**)(base+3575168))[room];for(int count=0;tile&&count<8192&&*(unsigned*)tile&&tile[3]==room;count++){
  unsigned sides=(*(unsigned short*)(tile+6)>>12)&15;if(sides<3||sides>11)break;unsigned size=*((unsigned char*)(base+2439208)+sides);if(size<8||size>128)break;
  JumpVec point;FN(void(*)(void*,JumpVec*),0xd5660)(tile,&point);if(isfinite(point.x)&&isfinite(point.y)&&isfinite(point.z)&&sweepSearchAt(point.x,point.z,point.y+4,0))return 1;tile+=size;
 }}return 0;
}
static void sweepAdvance(void){sweepNote("completed");sweep.phase++;sweep.tick=0;aimSubmit(0,NUMBER,(uintptr_t)CURRENT,0,0);sweepReset();}
static void sweepHealth(void){
 for(int i=0;i<600;i++){void*p=(void*)(base+0x322740+i*96);HealthValue v;if(p==PP(200)||!healthRead(p,&v)||!v.alive)continue;
  int hits=healthConfirmations;void*chr=HB_PTR(p,8);if(v.slot>=0){HB_FLOAT(PLAYERS[v.slot],260)=10;*(int*)((unsigned char*)PLAYERS[v.slot]+284)=-1;}else{HB_FLOAT(chr,320)=100;HB_FLOAT(chr,316)=0;}
  int players=*(int*)(base+0x303000+618564);if(v.slot>=0)*(int*)(base+0x303000+618564)=32;
  HealthValue baseline;healthRead(p,&baseline);
  JumpVec eye={PF(1864),PF(1868),PF(1872)};FN(int(*)(void*,int,JumpVec*,int,int),0x31230)(chr,15,&eye,5,1);*(int*)(base+0x303000+618564)=players;
  if(healthConfirmations==hits){HealthValue after;sweepCheck(healthRead(p,&after)&&after.total>=baseline.total-.00001f,"a real HP reduction must confirm a health bar");sweepNote("invulnerable target does not invent health damage");continue;}
  sweepCheck(healthConfirmations>hits,"actual native target damage confirms health bar");HealthTarget*target=healthFind(p,0);sweepTarget=target;sweepCheck(target&&target->shown&&target->age==0,"hit reveals and refreshes target");sweepCheck(healthAlpha(3.25f)==1&&healthAlpha(3.35f)>0&&healthAlpha(3.35f)<1&&healthAlpha(3.46f)==0,"hold and fade endpoints");sweepNote("native target health and fade");return;
 }
 fprintf(logfile,"SKIP sweep: no eligible native target in this setup\n");fflush(logfile);
}
static void sweepBefore(signed char*x,signed char*y,unsigned short*buttons){
 if(!getenv("GE32_LEVEL_SWEEP"))return;*x=*y=0;*buttons=0;sweep.fire=0;if(!jumpAllowed())return;
 PF(432)=PF(436)=PF(440)=0;PF(260)=100; /* Offline isolation from mission guards; damage is not a movement oracle. */
 if(getenv("GE32_SWEEP_PRACTICE"))*(int*)(base+0x303000+618564)=1;
 sweep.tick++;
 if(!sweep.ready){if(sweep.tick<90)return;fprintf(logfile,"SWEEP start stage=%d spawn=%.2f %.2f %.2f\n",*(int*)(base+0x303000+211452),PF(1864),PF(140),PF(1872));fflush(logfile);sweepCheck(sweepFindSpace(),"find actual clear floor and movement lane");sweepCheck(grenadeAmmo()==3,"exactly three initial grenades");fprintf(logfile,"SWEEP anchor %.3f %.3f %.3f yaw=%.1f\n",sweep.anchor.x,sweep.anchor.y,sweep.anchor.z,sweep.yaw);fflush(logfile);sweep.ready=1;sweep.phase=getenv("GE32_SWEEP_HEALTH_RELOAD_ONLY")?9:1;if(getenv("GE32_SWEEP_START_PHASE"))sweep.phase=atoi(getenv("GE32_SWEEP_START_PHASE"));sweep.tick=0;sweepReset();FN(int(*)(int),0x1000+44224)(5);FN(int(*)(int),0x1000+44224)(8);FN(void(*)(void),0x1000+459232)();swapNow(0,5);*reserveAmmo(ammoType(GRENADE_ITEM))=3;return;}
 int t=sweep.tick;sweepCheck(t<(sweep.phase==11?1250:600),"phase deadline");
 if(sweep.phase==1){
  if(t==1){sweep.start=(JumpVec){PF(1864),PF(140),PF(1872)};}
  *buttons=t<=20?8:t<=40?4:t<=60?1:t<=80?2:0;
  if(t==85){sweepCheck(hypotf(PF(1864)-sweep.start.x,PF(1872)-sweep.start.z)<500,"four-way native movement remains bounded");sweepAdvance();}
 }else if(sweep.phase==2){
  *buttons=0x10;aimSubmit(1,NUMBER,(uintptr_t)CURRENT,12000,0);AcquireSRWLockExclusive(&aimLock);aimInput.lt=32767;aimInput.lx=(int)(18000*sinf(sweepAimAttempts*.7853981634f));aimInput.ly=(int)(-18000*cosf(sweepAimAttempts*.7853981634f));ReleaseSRWLockExclusive(&aimLock);
  if(t==1)sweep.start=(JumpVec){PF(1864),PF(380),PF(1872)};
  if(t==35){if(hypotf(PF(1864)-sweep.start.x,PF(1872)-sweep.start.z)<=5&&sweepAimAttempts<7){sweepAimAttempts++;sweep.tick=0;sweepReset();return;}sweepCheck(PI(332)&&hypotf(PF(1864)-sweep.start.x,PF(1872)-sweep.start.z)>5,"aiming and strafing move native body");sweepCheck(fabsf(PF(380)-sweep.start.y)>1,"look while strafing");sweepAdvance();}
 }else if(sweep.phase==3){
  if(t==1){sweep.jumps=jumpCount;sweep.peak=PF(140);InterlockedExchange(&pendingJump,1);}
  sweep.peak=fmaxf(sweep.peak,PF(140));if(t==4&&jumpActive)InterlockedExchange(&pendingJump,1);
  if(t>25&&!jumpActive&&slideGrounded()){sweepCheck(jumpCount==sweep.jumps+1&&sweep.peak>sweep.anchor.y+2,"one jump, no airborne double jump, real landing");sweepAdvance();}
 }else if(sweep.phase==4){
  if(t==1&&sweepSlideAttempts){PF(380)=sweep.yaw+sweepSlideAttempts*45;FN(void(*)(void),0x1000+87712)();}
  *buttons=t<28?8:0;if(t==2){sweep.slides=slide.starts;InterlockedIncrement(&sprintClicks);}if(t==20){fprintf(logfile,"SWEEP slide attempt=%d sprint=%d blend=%.2f stride=%.3f crouch=%d auto=%d\n",sweepSlideAttempts,sprintOn,sprintBlend,hypotf(PF(1864)-slide.lastX,PF(1872)-slide.lastZ),PI(184),PI(14004));fflush(logfile);PI(184)=0;InterlockedExchange(&slideRequests,1);}if(t==85){if(slide.starts==sweep.slides&&sweepSlideAttempts<7){sweepSlideAttempts++;sweep.tick=0;sweepReset();return;}sweepCheck(slide.starts>sweep.slides&&!slide.active,"sprint crouch starts and finishes slide");sweepAdvance();}
 }else if(sweep.phase==5){
  if(t==20){sweep.slaps=slap.slaps;sweep.ammo=testTotalAmmo(1);InterlockedExchange(&slapRequests,1);}if(t>30&&slap.slaps>sweep.slaps&&!slap.phase){sweepCheck(HI(0,0)==5&&testTotalAmmo(1)==sweep.ammo,"quick slap returns gun and preserves ammunition");sweepAdvance();}
 }else if(sweep.phase==6){
  if(t==1){sweep.launches=gadget.launches;sweep.deploys=gadget.deploys;InterlockedExchange(&grenadeTestSelect,PAD_ITEM);}
  if(padCarrying&&t>20){sweepCheck(gadget.valid,"pad preview fits verified clear lane");InterlockedExchange(&padPlaceRequest,1);}
  if(gadget.deploys>sweep.deploys){sweepCheck(!padCarrying&&gadget.placed,"wheel pad placed by RT request");sweep.phase=7;sweep.tick=0;}
 }else if(sweep.phase==7){
  if(t==100&&gadget.launches==sweep.launches){fprintf(logfile,"FIXTURE blocked pad approach: testing contact at the placed pad center\n");fflush(logfile);padTestPlacePlayer(gadget.position.x,gadget.position.z,gadget.position.y+2,sweep.yaw);PF(396)=0;sweep.previousValid=0;}
  if(gadget.launches==sweep.launches)*buttons=8;
  if(gadget.launches>sweep.launches&&t>30&&!gadget.flight&&slideGrounded()){sweepCheck(isfinite(PF(140))&&PP(1856),"pad launches and lands with native floor");sweepAdvance();}
 }else if(sweep.phase==8){
  if(t==1){InterlockedExchange(&grenadeTestSelect,GRENADE_ITEM);sweep.ammo=grenadeAmmo();}
  if(t>90&&t<100){*buttons=0x2000;sweep.fire=1;}
  if(t==360){sweepCheck(grenadeAmmo()==sweep.ammo-1,"native grenade throw consumes exactly one");sweepAdvance();}
 }else if(sweep.phase==9){
  if(t==1){sweepHealth();InterlockedExchange(&grenadeTestSelect,5);}
  if(sweepTarget&&sweepTarget->used){float age=sweepTarget->age;if(age>3.f&&age<3.25f){sweepCheck(healthAlpha(age)==1,"native health remains fully visible during hold");sweepHoldSeen=1;}if(age>3.25f&&age<3.45f){sweepCheck(healthAlpha(age)>0&&healthAlpha(age)<1,"native health enters fade interval");sweepFadeSeen=1;}}
  if(t==260){if(sweepTarget){sweepCheck(sweepHoldSeen&&sweepFadeSeen&&!sweepTarget->used,"native timed health hold, fade and expiration");}sweepCheck(HI(0,0)==5&&!padCarrying,"wheel returns to gun");sweepAdvance();}
 }else if(sweep.phase==10){
  if(t==5)*buttons=0x4000;
  if(t==20){handReset(0);HI(0,44)=2;*reserveAmmo(ammoType(5))=20;sweep.ammo=testTotalAmmo(1);FN(void(*)(int),0x67fc0)(0);}
  if(t==150){sweepCheck(HI(0,44)>2&&HI(0,36)==0&&testTotalAmmo(1)==sweep.ammo,"native reload fills magazine without creating or losing ammo");sweepAdvance();}

 }else if(sweep.phase==11){
  int route=(t-1)/150,step=(t-1)%150+1;
  if(step==1){sweepReset();PF(380)=sweep.yaw+route*45;FN(void(*)(void),0x1000+87712)();}
  if(step<36)*buttons=8;if(step==2&&!(route&1))InterlockedIncrement(&sprintClicks);
  if(step==8&&(route&1))InterlockedExchange(&pendingJump,1);
  if(step==20&&(route==0||route==4)){PI(184)=0;InterlockedExchange(&slideRequests,1);}
  if(step==150){float floor=NAN;void*tile=floorBelow(PF(1864),PF(140)+2,PF(1872),&floor);if(!tile||fabsf(PF(140)-floor)>=2)fprintf(logfile,"SWEEP query tile=%p floor=%.3f nativeTile=%p nativeY=%.3f inside=%d padFlight=%d drop=%d\n",tile,floor,PP(1856),PP(1856)?FN(float(*)(void*,float,float),0x1000+879136)(PP(1856),PF(1864),PF(1872)):NAN,PP(1856)?FN(int(*)(void*,float,float),0xd5e10)(PP(1856),PF(1864),PF(1872)):0,gadget.flight,drop.active);sweepCheck(tile&&isfinite(floor)&&fabsf(PF(140)-floor)<2&&!gadget.flight,"radial route settles on a real floor");fprintf(logfile,"PASS sweep radial route=%d stage=%d\n",route,*(int*)(base+0x303000+211452));fflush(logfile);if(route==7){fprintf(logfile,"PASS LEVEL SWEEP stage=%d checks=%d samples=%d\n",*(int*)(base+0x303000+211452),sweep.checks,sweep.samples);fflush(logfile);ExitProcess(0);}}

 }
}
static void sweepAfter(void){
 if(!getenv("GE32_LEVEL_SWEEP")||!sweep.ready||!jumpAllowed())return;
 sweepCheck(isfinite(PF(140))&&isfinite(PF(1864))&&isfinite(PF(1872))&&isfinite(PF(152)),"finite native body state");
 if(sweep.phase==10&&sweep.tick==5)sweepCheck(FN(int(*)(void),0x1cdc0)()!=0,"native interaction request is delivered");
 JumpVec now={PF(1864),PF(140),PF(1872)};float head=fmaxf(40,PF(13940)+PF(188)-4);int air=jumpActive||gadget.flight||drop.active;if(sweep.previousValid&&(air||sweep.previousAir)&&hypotf(now.x-sweep.previous.x,now.z-sweep.previous.z)<128){int clear=templePhysicalClear(sweep.previous,now,head);if(!clear){fprintf(logfile,"SWEEP collision before=%.3f %.3f %.3f now=%.3f %.3f %.3f head=%.3f crouch=%d drop=%d jump=%d\n",sweep.previous.x,sweep.previous.y,sweep.previous.z,now.x,now.y,now.z,head,PI(184),drop.active,jumpActive);fflush(logfile);}sweepCheck(clear,"native movement never crosses solid background");}sweep.previous=now;sweep.previousValid=1;sweep.previousAir=air;sweep.samples++;
}
static void sweepLaunch(void){
 if(!getenv("GE32_LEVEL_SWEEP"))return;static ULONGLONG started;if(!started)started=GetTickCount64();if(GetTickCount64()-started>120000){fprintf(logfile,"FAIL sweep: wall-clock deadline phase=%d tick=%d allowed=%d\n",sweep.phase,sweep.tick,CURRENT?jumpAllowed():0);fflush(logfile);ExitProcess(2);}
 static int frames,launched;if(getenv("GE32_SWEEP_PRACTICE")&&!launched&&++frames==60){launched=1;FN(void(*)(int,int),0x1000+999872)(atoi(getenv("GE32_SWEEP_PRACTICE")),1); /* Target for the health fixture. */}
}
