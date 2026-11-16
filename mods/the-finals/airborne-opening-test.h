/* Offline Temple high-window acceptance. Fixtures never run normally. */
static int openingTicks,openingCase,openingStep,openingFlightTick,openingLaunched,openingCrossed,openingPaused,openingSawSill,openingCaptureDone;
static float openingMaxX,openingMinX,openingStartFloor,openingHealth;static JumpVec openingPrior;static int openingPriorValid;static volatile LONG openingCapture;
static void openingCheck(int ok,const char*why){if(!ok){fprintf(logfile,"FAIL opening: %s case=%d tick=%d flight=%d xz=%.2f/%.2f foot=%.2f floor=%.2f bridge=%d\n",why,openingCase,openingStep,gadget.flight,PF(1864),PF(1872),PF(140),PF(144),gadget.portalBridge);fflush(logfile);ExitProcess(2);}}
static void openingClearanceChecks(void){
 padTestPlacePlayer(450,-1870,470,270);PF(140)=1300;JumpVec to={1100,1300,-1870};openingCheck(dropBodyClear(&to),"clear centered aperture fits full body");
 PF(140)=500;openingCheck(!dropBodyClear(&to),"low wall remains solid");PF(140)=1900;openingCheck(!dropBodyClear(&to),"head cannot pass through upper frame");
 PF(140)=1300;PF(1872)=-1650;to.z=-1650;openingCheck(!dropBodyClear(&to),"adjacent wall remains solid");PF(1872)=-2000;to.z=-2000;openingCheck(!dropBodyClear(&to),"straight body sweep rejects narrow jamb grazing");
 PF(1864)=780;PF(1872)=-1870;PF(140)=1200;gadget.airPortal=1;float support=padPortalSupport(0);openingCheck(support>1080&&support<1110,"physical sill support is found without a floor-plan tile");
 fprintf(logfile,"PASS opening clearance: full aperture, lower wall, upper frame, solid neighbor, jamb and physical sill\n");fflush(logfile);
}
static void openingStart(void){
 float x=openingCase==1?1150:450,z=openingCase==4?-1650:openingCase==5?-2000:openingCase==7?-1759:-1870,yaw=openingCase==1?90:270;
 padTestPlacePlayer(x,z,470,yaw);PF(396)=0;PF(416)=PF(420)=PF(424)=0;PF(432)=PF(436)=PF(440)=0;FN(void(*)(void),0x1000+87712)();
 gadget.owner=CURRENT;gadget.stage=*(int*)(base+0x303000+211452);gadget.position=(JumpVec){x,PF(140),z};gadget.placed=1;gadget.cooldown=0;gadget.airPortal=gadget.portalBridge=0;
 void*tile=NULL;openingCheck(padFloor(gadget.position,&tile),"supported deployable launch footprint");openingStartFloor=PF(140);openingHealth=PF(260);openingMaxX=openingMinX=x;openingStep=openingFlightTick=openingLaunched=openingCrossed=openingPaused=openingSawSill=openingPriorValid=0;
 fprintf(logfile,"opening case=%d start %.1f/%.1f floor=%.2f\n",openingCase,x,z,PF(140));fflush(logfile);
}
static void openingBefore(signed char*x,signed char*y,unsigned short*buttons){
 if(!getenv("GE32_OPENING_TEST"))return;*x=*y=0;*buttons=0;if(!jumpAllowed())return;if(++openingTicks<180)return;if(openingTicks==180){openingClearanceChecks();if(getenv("GE32_OPENING_START_CASE"))openingCase=atoi(getenv("GE32_OPENING_START_CASE"));openingStart();}
 openingStep++;PF(380)=openingCase==1?90:270;FN(void(*)(void),0x1000+87712)();
 if(gadget.flight){openingLaunched=1;openingFlightTick++;}
 int stopCase=openingCase==2||openingCase==3;
 if(openingLaunched){
  if(stopCase){
   if(!openingPaused&&PF(1864)<770)*buttons=8;else{openingPaused++;if(PF(140)>1080&&PF(140)<1110&&fabsf(PF(152))<.1f)openingSawSill=1;if(openingPaused>130)*buttons=openingCase==2?8:4;}
  }else if(openingCase==6){if(openingFlightTick>=165&&openingFlightTick<230)*buttons=8;}
  else if(openingFlightTick<=(openingCase==1?150:110))*buttons=8;
 }
 openingMaxX=fmaxf(openingMaxX,PF(1864));openingMinX=fminf(openingMinX,PF(1864));if(openingCase==1?PF(1864)<550:PF(1864)>970)openingCrossed=1;
 if(!openingCaptureDone&&openingCase==0&&PF(1864)>1040&&gadget.flight){openingCaptureDone=1;InterlockedExchange(&openingCapture,1);}
 JumpVec now={PF(1864),PF(140),PF(1872)};if(openingPriorValid)openingCheck(templePhysicalClear(openingPrior,now,fmaxf(40,PF(13940)+PF(188)+8)),"swept body does not penetrate solid background");openingPrior=now;openingPriorValid=1;
 openingCheck(isfinite(PF(140))&&PF(140)>-1200&&isfinite(PF(1864)),"finite bounded flight");openingCheck(PF(260)>=openingHealth-.001f&&!PI(256),"no fall damage or death");
 if(openingLaunched&&!gadget.flight&&openingFlightTick>30){
  int baseline=getenv("GE32_OPENING_BASELINE")!=NULL,blocked=baseline||openingCase==3||openingCase==4||openingCase==6;
  openingCheck(openingCase==5|| (blocked?!openingCrossed:openingCrossed),"expected open passage or solid-wall rejection");
  if(stopCase&&!baseline)openingCheck(openingSawSill,"stopping inside the window rests on its physical sill");
  openingCheck(fabsf(PF(140)-PF(144))<1.5f,"stable native landing");openingCheck(PP(1856)&&FN(int(*)(void*,float,float),0x1000+871952)(PP(1856),PF(1864),PF(1872)),"landed navigation anchor contains the player");
  fprintf(logfile,"PASS opening case=%d crossed=%d sill=%d x=%.2f..%.2f landing=%.2f room=%d clampSkips=%d\n",openingCase,openingCrossed,openingSawSill,openingMinX,openingMaxX,PF(144),FN(int(*)(void*),0x1000+883888)(PP(1856)),portalClampSkips);fflush(logfile);
  if(baseline||++openingCase==8){fprintf(logfile,"PASS opening suite: %s\n",baseline?"original floor-plan barrier reproduced":"both directions, sill stop/resume/retreat, solid wall, jamb grazing, low approach, reported near-jamb path");fflush(logfile);ExitProcess(0);}openingStart();
 }
 openingCheck(openingStep<550,"route deadline");
}
