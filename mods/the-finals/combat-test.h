/* Offline integration harness. Never enabled for ordinary play. */
static int combatTestFrame,combatTestCase=-1,combatTestState,combatTestWait,combatTestStartSlaps,combatTestStartEvents;
static int combatTestRight,combatTestLeft,combatTestClip[2],combatTestReserve[2],combatTestPrevious[2];
static float combatTestX,combatTestZ,combatTestMaxDistance;
static void combatCheck(int okay,const char*what){if(!okay){if(logfile){fprintf(logfile,"FAIL combat: %s frame=%d case=%d phase=%d state=%d item=%d model=%d visible=%d\n",what,combatTestFrame,combatTestCase,slap.phase,HI(0,36),HI(0,0),PI(3008),PI(3000));fflush(logfile);}ExitProcess(2);}}
static int testTotalAmmo(int type){int total=*reserveAmmo(type);for(int h=0;h<2;h++)if(ammoType(HI(h,0))==type)total+=HI(h,44);return total;}
static void prepareSlapCase(int c){
 FN(int(*)(int),0x1000+44224)(5);FN(int(*)(int),0x1000+44224)(8);
 FN(void(*)(void),0x1000+459232)();
 if(c==1)FN(int(*)(int,int),0x1000+44480)(5,5);
 if(c==2)FN(int(*)(int),0x1000+44224)(17);
 combatTestRight=c==3?8:5;combatTestLeft=c==1?5:0;
 swapNow(0,combatTestRight);swapNow(1,combatTestLeft);
 HI(0,44)=c==3?9:7;HI(1,44)=c==1?11:0;
 PI(14072)=c==2?17:1;
 combatTestClip[0]=HI(0,44);combatTestClip[1]=HI(1,44);combatTestPrevious[0]=HI(0,8);combatTestPrevious[1]=HI(1,8);
 combatTestReserve[0]=testTotalAmmo(1);combatTestReserve[1]=testTotalAmmo(2);
 combatTestStartSlaps=slap.slaps;combatTestStartEvents=slap.events;combatTestWait=0;combatTestState=0;slap.cooldown=0;
 if(logfile){fprintf(logfile,"TEST slap case=%d guns=%d/%d ammo=%d/%d\n",c,combatTestRight,combatTestLeft,combatTestClip[0],combatTestClip[1]);fflush(logfile);}
}
static void combatTestBefore(unsigned short*buttons){
 if(!getenv("GE32_COMBAT_TEST"))return;
 *buttons=0;
 if(!jumpAllowed())return;
 combatTestFrame++;
 if(combatTestFrame<90)return;
 int t=combatTestFrame-90;
 if(t<170){
  *buttons=t<150?8:t<160?4:0;
  if(t==0){combatTestX=PF(1864);combatTestZ=PF(1872);}
  if(t==20||t==60||t==90||t==120)InterlockedIncrement(&sprintClicks);
  if(t==50)*buttons|=0x10;
  if(t==80)*buttons|=0x2000;
  if(t==110)PI(184)=0;
  if(t==111)PI(184)=2;
  if(t==140)openWheel=1;
  if(t==141)openWheel=0;
  return;
 }
 if(combatTestCase<0){combatTestCase=0;prepareSlapCase(0);}
 combatTestWait++;
 if(combatTestState==0&&combatTestWait>=30){InterlockedExchange(&slapRequests,1);combatTestState=1;combatTestWait=0;}
 if(combatTestState==1){
  if(combatTestCase==4&&slap.phase==2&&slap.deferredWeapon<0)InterlockedExchange(&pendingWeapon,8);
  if(combatTestCase==5&&slap.phase==2&&combatTestWait==12)InterlockedExchange(&slapRequests,1);
 }
}
static void combatTestAfter(unsigned short raw){
 if(!getenv("GE32_COMBAT_TEST")||!jumpAllowed()||combatTestFrame<90)return;
 int t=combatTestFrame-90;
 if(t<170){
  combatTestMaxDistance=fmaxf(combatTestMaxDistance,hypotf(PF(1864)-combatTestX,PF(1872)-combatTestZ));
  if(t==40)combatCheck(sprintOn&&sprintBlend>.99f,"forward click accelerates to sprint");
  if(t==50||t==80||t==110||t==140||t==150||t==165)combatCheck(!sprintLatched&&!sprintOn,"aim/fire/crouch/wheel/backward/stop cancels sprint");
  if(t==169){combatCheck(combatTestMaxDistance>1,"native horizontal movement occurred");if(logfile){fprintf(logfile,"PASS sprint: forward boost, aim/fire/crouch/wheel/backward/stop cancellation, native displacement=%.2f\n",combatTestMaxDistance);fflush(logfile);}}
  return;
 }
 if(slap.phase==2){combatCheck(HI(0,0)==1&&PI(14072)==1,"slap uses fist item and palm model rather than gun/sniper butt");}
 if(combatTestWait>220)combatCheck(0,"slap completion timeout");
 if(combatTestState==1&&slap.slaps==combatTestStartSlaps+1&&!slap.phase&&combatTestWait>10){
  combatCheck(slap.events==combatTestStartEvents+1,"exactly one native melee strike event");
  combatCheck(testTotalAmmo(1)==combatTestReserve[0]&&testTotalAmmo(2)==combatTestReserve[1],"total ammunition preserved");
  if(combatTestCase!=4){
   combatCheck(HI(0,0)==combatTestRight&&HI(1,0)==combatTestLeft,"both guns restored");
   combatCheck(HI(0,44)==combatTestClip[0]&&HI(1,44)==combatTestClip[1],"partially loaded magazines preserved");
   combatCheck(HI(0,8)==combatTestPrevious[0]&&HI(1,8)==combatTestPrevious[1],"previous-weapon history preserved");
  }
  combatTestState=2;combatTestWait=0;
 }
 if(combatTestState==2&&combatTestWait>70){
  combatCheck(slap.slaps==combatTestStartSlaps+1,"held/repeated-active request does not auto-repeat");
  if(combatTestCase==4)combatCheck(HI(0,0)==8,"wheel choice during slap is honored");
  combatTestCase++;
  if(combatTestCase==6){if(logfile){fprintf(logfile,"PASS: 6 quick-slap cases, native strike dispatch, gun return, single/dual magazine preservation, sniper palm override, wheel deferral and spam rejection\n");fflush(logfile);}ExitProcess(0);}
  prepareSlapCase(combatTestCase);
 }
 (void)raw;
}
