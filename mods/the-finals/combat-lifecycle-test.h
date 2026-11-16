/* Offline regression for stage/time resets that reuse the player allocation. */
static void combatLifecycleTest(void){
 if(!getenv("GE32_COMBAT_LIFECYCLE_TEST")||!jumpAllowed())return;static int ticks;if(++ticks!=120)return;
 int*stage=(int*)(base+0x303000+211452),*timer=(int*)(base+0x303000+211432),savedStage=*stage,savedTimer=*timer;
 for(int test=0;test<3;test++){
  unsigned short b=0,o=0;combatBefore(&b,&o);slap.phase=1;slap.cooldown=12;sprintLatched=sprintOn=1;sprintBlend=1;InterlockedExchange(&sprintClicks,1);InterlockedExchange(&slapRequests,1);
  if(test==0)*stage=savedStage+1;if(test==1)*timer=savedTimer-60;if(test==2)combatPlayer=(void*)((uintptr_t)CURRENT+8);
  combatBefore(&b,&o);*stage=savedStage;*timer=savedTimer;
  if(slap.phase||slap.cooldown||sprintLatched||sprintOn||sprintBlend||sprintClicks||slapRequests){fprintf(logfile,"FAIL combat lifecycle case=%d: stale melee/sprint survives context reset phase=%d cooldown=%.2f\n",test,slap.phase,slap.cooldown);fflush(logfile);ExitProcess(2);}
  fprintf(logfile,"PASS combat lifecycle case=%d\n",test);fflush(logfile);
 }
 ExitProcess(0);
}
