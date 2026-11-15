/* Native Temple acceptance: wheel -> carry -> RT deploy -> launch -> upper landing. */
static volatile LONG padTestRenderAction,padTestRenderPending;
static int templePhysicalClear(JumpVec,JumpVec,float);
static JumpVec padTestPrevious;static float padTestPreviousHead;static int padTestPreviousValid;
static int padTestTicks,padTestPhase,padTestWait,padTestFinished;
static float padTestHealth,padTestPeak;
static float padTestCeiling,padTestHeadPeak;static int padTestCeilingStops,padTestAmmo[2];
static int padTestGuns[2],padTestClips[2],padTestLaunchBase;
static void padTestCheck(int ok,const char*why){if(!ok){if(logfile){fprintf(logfile,"FAIL pad: %s phase=%d xz=%.2f %.2f foot=%.2f floor=%.2f vel=%.2f\n",why,padTestPhase,PF(1864),PF(1872),PF(140),PF(144),PF(152));fflush(logfile);}ExitProcess(2);}}
static void padTestPlacePlayer(float x,float z,float maxY,float yaw){
 padTestPreviousValid=0;
 float y;void*t=floorBelow(x,maxY,z,&y);padTestCheck(t&&isfinite(y),"fixture floor exists");
 PP(1856)=PP(56)=t;PF(1864)=x;PF(1872)=z;PF(140)=PF(144)=y;PF(136)=y/.2004f;PF(152)=0;PI(184)=PI(14004)=2;PI(14124)=0;PP(14128)=NULL;PF(380)=yaw;PF(396)=-60;
 FN(void(*)(void),0x1000+87712)();FN(void(*)(void),0x1000+86960)();FN(void(*)(void),0x1000+114768)();jumpActive=0;drop.active=0;gadget.flight=0;
}
static void padTestBefore(signed char*x,signed char*y,unsigned short*buttons){
 if(!getenv("GE32_PAD_TEST"))return;*x=*y=0;*buttons=0;
 if(!jumpAllowed())return;
 padTestCheck(++padTestTicks<2400,"integration deadline");padTestWait++;
 if(padTestTicks==90){FN(int(*)(int),0x1000+44224)(5);FN(void(*)(void),0x1000+459232)();}
 if(!padTestPhase&&padTestTicks==300){
  if(getenv("GE32_PAD_LIVE_TEST"))dropActorFixture();
  padTestPlacePlayer(-134.18f,-325.f,-460.f,180.f);padTestCheck(PF(144)<-460,"lower Temple fixture");
  void*support=NULL;padTestCheck(padFloor((JumpVec){-134.18f,-468.7878f,-325.f},&support),"enlarged pad fits clear lower floor");padTestCheck(!padFloor((JumpVec){-150,468.7878f,-690},&support),"enlarged footprint rejects upper-rim overhang");padTestCheck(padContact(70,0)&&!padContact(95,0)&&!padContact(68,68),"octagonal contact accepts the larger deck but rejects outside corners");
  if(logfile){fprintf(logfile,"PASS pad size: diameter=%.0f, clear floor, rim-overhang rejection and visible-octagon contact\n",PAD_RADIUS*2);fflush(logfile);}
  FN(int(*)(int),0x1000+44224)(5);FN(void(*)(void),0x1000+459232)();

  padTestAmmo[0]=testTotalAmmo(1);padTestAmmo[1]=testTotalAmmo(2);padTestHealth=FN(float(*)(void),0x1000+124352)();for(int h=0;h<2;h++){padTestGuns[h]=HI(h,0);padTestClips[h]=HI(h,44);}padTestPhase=1;padTestWait=0;InterlockedExchange(&padTestRenderPending,1);
 }
 if(padTestPhase&&padTestPhase<6){PF(380)=180;FN(void(*)(void),0x1000+87712)();}
 if(padTestPhase==1&&padCarrying){padTestPhase=2;padTestWait=0;}
 if(getenv("GE32_PAD_VISUAL_TEST")&&padTestPhase==2){PF(396)=-60;FN(void(*)(void),0x1000+87712)();}
 if(padTestPhase==2&&padTestWait>(getenv("GE32_PAD_VISUAL_TEST")?120:15)){padTestCheck(gadget.valid,"valid floor preview");InterlockedExchange(&padTestRenderPending,2);padTestPhase=3;padTestWait=0;}
 if(padTestPhase==3&&gadget.placed&&!padCarrying){
  for(int h=0;h<2;h++)padTestCheck(HI(h,0)==padTestGuns[h]&&HI(h,44)==padTestClips[h],"placing preserved guns and magazines");
  padTestPhase=4;padTestWait=0;
 }
 if(padTestPhase==4){*buttons=8;if(gadget.launches){float contact=gadget.lastContactDistance;padTestCheck(contact<90,"launch contact stays on visible enlarged pad");if(!getenv("GE32_PAD_LIVE_TEST"))padTestCheck(contact>56,"new outer deck launches before the old tiny footprint");if(logfile){fprintf(logfile,"PASS enlarged pad outer-deck launch distance=%.2f\n",contact);fflush(logfile);}*buttons=0;padTestPhase=5;padTestWait=0;}}
 if(padTestPhase==5&&padTestWait%12==0&&logfile){fprintf(logfile,"pad flight test xz=%.2f %.2f foot=%.2f floor=%.2f v=%.2f ceilingStops=%d\n",PF(1864),PF(1872),PF(140),PF(144),PF(152),ceilingStops);fflush(logfile);}
 if(padTestPhase==5){PF(396)=0;padTestPeak=fmaxf(padTestPeak,PF(140));if(PF(140)>540)*buttons=8;if(PF(1872)<-740)*buttons=0;}
 if(padTestPhase==6){*buttons=0;}
 if(padTestPhase==7&&padCarrying){float foot=PF(140);PF(140)+=20;unsigned short b=0,o=0;padPlaceRequest=1;padBefore(&b,&o);padTestCheck(gadget.deploys==1&&padCarrying,"airborne deployment rejected");PF(140)=foot;void*t=NULL;padTestCheck(!padFloor((JumpVec){100000,foot,100000},&t),"missing-floor deployment rejected");padTestRenderPending=4;padTestPhase=8;}
 if(padTestPhase==9&&padCarrying){padTestRenderPending=3;padTestPhase=10;}
}
static void padTestAfter(void){
 if(!getenv("GE32_PAD_TEST")||!jumpAllowed())return;
 if(getenv("GE32_PAD_LIVE_TEST")){JumpVec now={PF(1864),PF(140),PF(1872)};float head=fmaxf(40,PF(13940)+PF(188)+8);if(padTestPreviousValid)padTestCheck(templePhysicalClear(padTestPrevious,now,fmaxf(head,padTestPreviousHead)),"live bot movement stays clear of solid geometry");padTestPrevious=now;padTestPreviousHead=head;padTestPreviousValid=1;}
 if(padTestPhase==5&&!gadget.flight&&gadget.launches){
  if(getenv("GE32_PAD_LIVE_TEST")){float h;void*t=floorBelow(PF(1864),PF(140)+1,PF(1872),&h);padTestCheck(t&&isfinite(h)&&fabsf(PF(140)-h)<1.5f,"live pad launch lands safely despite bot knockback");}else{padTestCheck(PF(144)>460&&fabsf(PF(140)-PF(144))<.3f,"landed on upper floor");padTestCheck(padTestPeak>540,"arc clears upper floor");}
  padTestCheck(getenv("GE32_PAD_LIVE_TEST")||fabsf(FN(float(*)(void),0x1000+124352)()-padTestHealth)<.001f,"no fall damage");
  if(logfile){fprintf(logfile,"PASS pad: LB wheel selection, RT placement, no gun fire/ammo loss, native launch peak=%.2f upper-floor landing=%.2f\n",padTestPeak,PF(144));fflush(logfile);}
  padTestPhase=6;padTestWait=0;padTestLaunchBase=gadget.launches;
  padTestPlacePlayer(gadget.position.x,gadget.position.z,gadget.position.y+2,180);gadget.cooldown=0;
 }
 if(padTestPhase==6&&gadget.launches>padTestLaunchBase&&!gadget.flight){
  if(getenv("GE32_PAD_LIVE_TEST")){float h;void*t=floorBelow(PF(1864),PF(140)+1,PF(1872),&h);padTestCheck(t&&isfinite(h)&&fabsf(PF(140)-h)<1.5f,"reused pad lands on a real floor despite bot knockback");}else padTestCheck(PF(144)<-460,"reusable pad returned to lower floor");padTestCheck(gadget.deploys==1,"pad reusable without extra deployment");
  if(logfile){fprintf(logfile,"PASS pad: reusable bounce, native gravity/landing and finite physics; health=%.3f\n",FN(float(*)(void),0x1000+124352)());fflush(logfile);}padTestPhase=7;padTestWait=0;padTestPlacePlayer(-134.18f,-325.f,-460.f,180);padTestRenderPending=1;
 }
 if(padTestPhase==8&&!padCarrying){padTestCheck(HI(0,0)==padTestGuns[0]&&HI(0,44)==padTestClips[0],"B cancel restores gun without ammo loss before rapid LB reselect");padTestPhase=9;padTestRenderPending=5;}
 if(padTestPhase==10&&!padCarrying&&HI(0,0)==5){
  padTestCheck(testTotalAmmo(1)==padTestAmmo[0]&&testTotalAmmo(2)==padTestAmmo[1],"LB gun selection preserves ammunition totals");
  if(logfile){fprintf(logfile,"PASS pad: airborne/no-floor rejection, B cancel, LB gun selection and ammo totals\n");fflush(logfile);}
  if(getenv("GE32_PAD_LIVE_TEST"))*(int*)(base+0x303000+618564)=1;
  PF(432)=PF(436)=PF(440)=PF(416)=PF(420)=PF(424)=PF(14092)=0;
  padTestPlacePlayer(-134.18f,100.f,400.f,180);gadget.position=(JumpVec){PF(1864),PF(140),PF(1872)};gadget.cooldown=0;padTestCeiling=jumpCeiling(PF(140)+1600);padTestCheck(isfinite(padTestCeiling)&&padTestCeiling>PF(140)+220&&padTestCeiling<PF(140)+1100,"real low-ceiling fixture");padTestCeilingStops=ceilingStops;padTestLaunchBase=gadget.launches;padTestPhase=11;
 }
 if(padTestPhase==11){
  padTestHeadPeak=fmaxf(padTestHeadPeak,PF(140)+fmaxf(PF(13940),120.f)+14);
  if(gadget.launches>padTestLaunchBase&&!gadget.flight){padTestCheck(ceilingStops>padTestCeilingStops&&padTestHeadPeak<=padTestCeiling+4,"pad ascent stops at ceiling");gadget.owner=NULL;padContext();padTestCheck(!gadget.placed&&!gadget.flight&&!padCarrying,"stage/player reset clears gadget state");if(logfile){fprintf(logfile,"PASS pad: real ceiling stops ascent, native landing, owner reset clears pad\n");fflush(logfile);}padTestFinished=1;ExitProcess(0);}
 }
}

/* Send scripted UI commands only after the current simulation snapshot is
 * published. Otherwise rapid cancel/reselect can race the previous frame. */
static void padTestPublishActions(void){if(getenv("GE32_PAD_TEST")){LONG a=InterlockedExchange(&padTestRenderPending,0);if(a)InterlockedExchange(&padTestRenderAction,a);}}
