/* Offline native Temple gunfire, damage attribution, lifecycle and fade tests.
 * Fixtures run only with GE32_HEALTH_TEST. No gameplay state is written normally. */
static int healthTestTick,healthTestPhase,healthTestWait,healthTestSlot,healthTestHits,healthTestSawHold,healthTestSawFade,healthTestShots;
static int healthTestFire;
static void*healthTestProp,*healthTestOther;
static volatile LONG healthTestCapture;
static ULONGLONG healthTestHitWall;
static void healthCheck(int okay,const char*message){if(!okay){if(logfile){fprintf(logfile,"FAIL health: %s tick=%d phase=%d hits=%d shown=%d\n",message,healthTestTick,healthTestPhase,healthConfirmations,healthPublished.count);fflush(logfile);}ExitProcess(2);}}
static void healthTestDamage(void*prop,int player,int part){int savedPractice=*(int*)(base+0x303000+618564);*(int*)(base+0x303000+618564)=4;HealthValue eligibility;if(healthRead(prop,&eligibility)&&eligibility.slot>=0)*(int*)((unsigned char*)PLAYERS[eligibility.slot]+284)=-1;JumpVec eye={PF(1864),PF(1868),PF(1872)};FN(int(*)(void*,int,JumpVec*,int,int),0x31230)(HB_PTR(prop,8),part,&eye,5,player);*(int*)(base+0x303000+618564)=savedPractice;}
static void healthTestReset(void){memset(healthTargets,0,sizeof(healthTargets));memset(&healthPublished,0,sizeof(healthPublished));}
static void healthTestNext(int phase){healthTestPhase=phase;healthTestWait=0;healthTestHits=healthConfirmations;}
static void healthGuardTest(void){
 if(!getenv("GE32_HEALTH_GUARD_TEST")||!jumpAllowed())return;static int ticks;if(++ticks!=120)return;void*prop=NULL;HealthValue before;for(int i=0;i<600;i++){void*p=(void*)(base+0x322740+i*96);if(*(unsigned char*)p==3&&healthRead(p,&before)&&before.alive){prop=p;break;}}healthCheck(prop!=NULL,"native mission guard found");void*chr=HB_PTR(prop,8);float maximum=HB_FLOAT(chr,320),damage=HB_FLOAT(chr,316);HB_FLOAT(chr,320)=100;HB_FLOAT(chr,316)=0;healthRead(prop,&before);int hits=healthConfirmations;JumpVec eye={PF(1864),PF(1868),PF(1872)};FN(int(*)(void*,int,JumpVec*,int,int),0x31230)(HB_PTR(prop,8),15,&eye,5,1);HealthValue after;healthRead(prop,&after);healthCheck(after.total<before.total&&healthConfirmations==hits+1,"native guard damage uses true remaining-health ratio");
 HB_FLOAT(chr,316)=-100;healthRead(prop,&before);healthCheck(before.hp==1&&fabsf(before.armor-1.f)<.0001f,"guard negative damage represents armor");FN(int(*)(void*,int,JumpVec*,int,int),0x31230)(chr,15,&eye,5,1);healthRead(prop,&after);healthCheck(after.hp==1&&after.armor<before.armor&&healthConfirmations==hits+2,"native armored guard hit reveals true armor loss");HB_FLOAT(chr,316)=-200;healthRead(prop,&before);FN(int(*)(void*,int,JumpVec*,int,int),0x31230)(chr,15,&eye,5,1);healthRead(prop,&after);healthCheck(before.total>2&&after.total<before.total&&after.armor==1&&healthConfirmations==hits+3,"damage above displayed armor capacity is still confirmed");HB_FLOAT(chr,320)=maximum;HB_FLOAT(chr,316)=damage;
 if(logfile){fprintf(logfile,"PASS health mission guard: native damage callback, HP ratio, negative-damage armor semantics\n");fflush(logfile);}ExitProcess(0);
}
static void healthTestBefore(unsigned short*buttons){
 healthGuardTest();
 if(!getenv("GE32_HEALTH_TEST")||!jumpAllowed())return;*buttons=0;healthTestFire=0;healthCheck(++healthTestTick<1800,"integration deadline");if(healthTestTick<120)return;
 if(healthTestTick==120){
  for(int i=0;i<32;i++){void*p=PLAYERS[i];HealthValue v;if(i!=localIndex()&&p&&healthRead(HB_PTR(p,200),&v)&&v.alive){if(!healthTestProp){healthTestProp=HB_PTR(p,200);healthTestSlot=i;}else healthTestOther=HB_PTR(p,200);}}
  healthCheck(healthTestProp&&healthTestOther,"two real native bot targets");*(int*)(base+0x303000+618564)=1;
  padTestPlacePlayer(-150,-100,-460,180);PF(396)=-12;FN(void(*)(void),0x166a0)();
  FN(int(*)(int),0x1000+44224)(5);FN(void(*)(void),0x1000+459232)();swapNow(0,5);healthTestReset();healthTestNext(1);
 }
 if(healthTestPhase<8){FN(void(*)(int),0xa4c10)(healthTestSlot);padTestPlacePlayer(-150,-500,-460,0);FN(void(*)(int),0xa4c10)(localIndex());}
 PF(380)=healthTestPhase==5?90:180;PF(396)=-12;FN(void(*)(void),0x166a0)();healthTestWait++;
 if((healthTestPhase==1||healthTestPhase==2)&&healthTestWait>=40&&healthTestWait%30>=10&&healthTestWait%30<=12)healthTestFire=1;
 if(healthTestPhase==5&&healthTestWait>=10&&healthTestWait<=12)*buttons=0x2000;
 if(healthTestPhase==6&&healthTestWait==1){
  unsigned char*chr=HB_PTR(healthTestProp,8);unsigned flags=*(unsigned*)(chr+24);*(unsigned*)(chr+24)=flags|16;healthTestDamage(healthTestProp,1,15);*(unsigned*)(chr+24)=flags;healthCheck(healthConfirmations==healthTestHits,"invulnerable hit does not reveal health");
 }
 if(healthTestPhase==7&&healthTestWait==1){void*p=PLAYERS[healthTestSlot];HB_FLOAT(p,260)=1;HB_FLOAT(p,264)=.5f;healthTestDamage(healthTestProp,1,15);HealthValue v;healthRead(healthTestProp,&v);healthCheck(v.hp==1&&v.armor<.5f&&v.armor>0,"native armor absorbs hit without HP loss");healthCheck(healthConfirmations==healthTestHits+1,"armor damage reveals bar");}
 if(healthTestPhase==8&&healthTestWait==1){healthTestReset();healthTestDamage(healthTestProp,0,15);healthCheck(healthConfirmations==healthTestHits,"another shooter's damage does not reveal target");}
 if(healthTestPhase==9&&healthTestWait==1){HB_FLOAT(PLAYERS[healthTestSlot],260)=1;HB_FLOAT(PLAYERS[healthTestSlot],264)=0;healthTestDamage(healthTestProp,1,15);HealthValue ov;healthRead(healthTestOther,&ov);if(logfile){fprintf(logfile,"other target hp=%.3f alive=%d slot=%d action=%d flags=%x\n",ov.hp,ov.alive,ov.slot,*((unsigned char*)HB_PTR(healthTestOther,8)+8),*(unsigned*)((unsigned char*)HB_PTR(healthTestOther,8)+24));fflush(logfile);}healthTestDamage(healthTestOther,1,15);healthCheck(healthFind(healthTestProp,0)&&healthFind(healthTestOther,0),"two targets retain independent records");}
 if(healthTestPhase==10&&healthTestWait==1){
  HB_FLOAT(PLAYERS[healthTestSlot],260)=.05f;HB_FLOAT(PLAYERS[healthTestSlot],264)=0;healthTestDamage(healthTestProp,1,15);HealthTarget*t=healthFind(healthTestProp,0);healthCheck(t&&t->dead&&t->hp<=0,"native lethal hit shows zero health");
 }
}
static void healthTestAfter(void){
 if(!getenv("GE32_HEALTH_TEST")||healthTestTick<120)return;
 HealthTarget*t=healthFind(healthTestProp,0);
 if(healthTestPhase==1&&healthTestWait%30==0&&logfile){HealthValue v;healthRead(healthTestProp,&v);fprintf(logfile,"health shot fixture wait=%d local=%.1f %.1f %.1f target=%.1f %.1f %.1f hp=%.3f alive=%d flags=%x clip=%d weapon=%d yaw=%.1f pitch=%.1f\n",healthTestWait,PF(1864),PF(1868),PF(1872),HB_FLOAT(healthTestProp,16),HB_FLOAT(healthTestProp,20),HB_FLOAT(healthTestProp,24),v.hp,v.alive,*(unsigned*)((unsigned char*)HB_PTR(healthTestProp,8)+24),HI(0,44),HI(0,0),PF(380),PF(396));fflush(logfile);}
 if(healthTestPhase==1||healthTestPhase==2){
  if(healthTestWait<40)healthCheck(healthConfirmations==healthTestHits,"no reveal or timer reset before a confirmed shot");
  if(healthConfirmations>healthTestHits){healthCheck(healthConfirmations==healthTestHits+1&&t&&t->shown&&healthPublished.count>0,"actual gunshot publishes target above its head");healthCheck(fabsf(healthPublished.bars[0].hp-t->hp)<.0001f,"display uses actual remaining HP");healthTestShots++;healthTestCapture=1;
   if(healthTestPhase==1){healthTestNext(2);}else{healthCheck(t->age<.05f,"second actual hit restarts hold timer");healthTestHitWall=GetTickCount64();healthTestNext(3);}
  }
  healthCheck(healthTestWait<200,"gunshot hit timeout");
 }else if(healthTestPhase==3){
  healthCheck(t&&t->shown,"target retained throughout hold");if(t->age>HEALTH_HOLD_SECONDS-.10f&&t->age<HEALTH_HOLD_SECONDS){healthCheck(healthPublished.count==1&&healthPublished.bars[0].alpha==1,"full opacity through measured hold");healthTestSawHold=1;}
  if(t->age>=HEALTH_HOLD_SECONDS){healthTestNext(4);}
 }else if(healthTestPhase==4){
  if(t&&t->age>HEALTH_HOLD_SECONDS+.08f&&t->age<HEALTH_HOLD_SECONDS+.12f){healthCheck(healthPublished.count==1&&healthPublished.bars[0].alpha>.10f&&healthPublished.bars[0].alpha<.90f,"smooth fade between full and transparent");healthTestSawFade=1;healthTestCapture=2;}
  if(!t){healthCheck(healthTestSawHold&&healthTestSawFade&&healthPublished.count==0,"clean expiration after hold plus fade");if(logfile){fprintf(logfile,"PASS health timing: two real gunshots, hit refresh, full hold=%.2fs fade=%.2fs elapsed wall=%llu ms\n",HEALTH_HOLD_SECONDS,HEALTH_FADE_SECONDS,(unsigned long long)(GetTickCount64()-healthTestHitWall));fflush(logfile);}healthTestNext(5);}
 }else if(healthTestPhase==5){if(healthTestWait>40){healthCheck(healthConfirmations==healthTestHits&&!healthPublished.count,"missed gunshot does not reveal or refresh");healthTestNext(6);}}
 else if(healthTestPhase==6){if(healthTestWait>40){healthCheck(!healthPublished.count,"invulnerable target stays hidden");healthTestNext(7);}}
 else if(healthTestPhase==7){healthCheck(t&&t->shown&&t->hp==1&&t->armor>0&&t->armor<.5f,"armor-only bar stays accurate");if(healthTestWait>10)healthTestNext(8);}
 else if(healthTestPhase==8){if(healthTestWait>10){healthCheck(!healthPublished.count,"foreign damage stays hidden");healthTestNext(9);}}
 else if(healthTestPhase==9){if(healthTestWait>10){healthCheck(healthConfirmations==healthTestHits+2,"separate confirmed targets");healthTestNext(10);}}
 else if(healthTestPhase==10){if(healthTestWait>30){healthCheck(!t,"lethal target clears promptly");healthTestNext(11);}}
 else if(healthTestPhase==11){
  HealthTarget*other=healthFind(healthTestOther,0);healthCheck(other!=NULL,"remaining live target available for lifecycle tests");
  void*chr=HB_PTR(healthTestOther,8);HB_PTR(healthTestOther,8)=NULL;healthPublish();HB_PTR(healthTestOther,8)=chr;healthCheck(!healthFind(healthTestOther,0),"removed target identity cannot leave a ghost bar");
  healthTestDamage(healthTestOther,1,15);other=healthFind(healthTestOther,0);healthCheck(other&&other->shown,"target reacquired on a fresh hit");
  float age=other->age;healthReady=0;healthPublish();healthCheck(!healthPublished.count&&other->age==age,"pause/menu hides bars and freezes timeout");healthReady=1;
  other->dead=1;other->deathAge=.1f;healthPublish();healthCheck(!healthFind(healthTestOther,0),"new living state clears a previous-life record");
  healthTestDamage(healthTestOther,1,15);int savedStage=healthStage;healthStage=-1234;healthBegin();healthCheck(!healthFind(healthTestOther,0),"stage/context reset clears all old targets");healthStage=savedStage;
  healthCheck(!healthSight((JumpVec){-165,550,-807},(JumpVec){-165,-460,-807}),"solid Temple deck occludes target sight");healthCheck(healthSight((JumpVec){-150,550,-310},(JumpVec){-150,-460,-310}),"open Temple atrium preserves target sight");float hidden[2];healthCheck(!padProject((JumpVec){PF(1864),PF(1868),PF(1872)+500},hidden),"behind-camera target is rejected");
  if(logfile){fprintf(logfile,"PASS health: actual shooting, misses, armor, invulnerable/foreign damage, independent targets, lethal fade, removed identities, pause, respawn/context reset\n");fflush(logfile);}ExitProcess(0);
 }
}
