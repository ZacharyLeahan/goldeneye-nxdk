/* Offline controller/ADS integration fixtures; never run during normal play. */
static int adsTicks,adsCase;static float adsStartX,adsStartZ;static double adsYaw;
static void adsMotionMath(void){
 for(int hz=30;hz<=144;hz+=(hz==30?30:84)){
  PrecisionAimState state={0};double sum=0,out[2];for(int i=0;i<hz;i++){aimLookResponse(32767,0,4000,1,65,60,60,1./hz,0,out,&state);sum+=out[0];}
  aimCheck(fabs(sum-365)<.000001,"hip 200 plus exact 220 boost ramp integrates consistently");
  memset(&state,0,sizeof(state));aimLookResponse(32767,0,4000,1,65,60,60,1.,1,out,&state);aimCheck(fabs(out[0]-120)<.000001,"ADS uses sixty percent, no hip boost");
  aimLookResponse(0,0,4000,1,65,60,60,1./hz,0,out,&state);aimCheck(out[0]==0&&state.boost==0,"center clears boost without drift");
 }
 fprintf(logfile,"PASS motion math: 200/170 base, 220 boost/.5s, 60 percent ADS and frame independence\n");fflush(logfile);
}
static void adsMovementBefore(signed char*x,signed char*y,unsigned short*buttons){
 if(!getenv("GE32_ADS_MOVE_TEST")||!jumpAllowed())return;
 *x=*y=0;*buttons=0;adsTicks++;
 if(adsTicks==90){adsMotionMath();padTestPlacePlayer(-150,100,-460,180);PI(184)=adsCase==7?0:2;PI(14004)=adsCase==7?0:2;PF(396)=0;PF(416)=PF(420)=PF(424)=0;int gun=adsCase==6?17:5;FN(int(*)(int),0x1000+44224)(gun);swapNow(0,gun);}
 if(adsTicks<120)return;
 if(adsTicks==120){adsStartX=PF(1864);adsStartZ=PF(1872);adsYaw=PF(380);}
 if(adsTicks<=150){
  int lx=adsCase==0?-24000:adsCase==1?24000:adsCase>=4?18000:0;
  int ly=adsCase==2?-24000:adsCase==3?24000:adsCase>=4?-18000:0;
  aimSubmit(1,NUMBER,(uintptr_t)CURRENT,adsCase>=4?12000:0,0);
  AcquireSRWLockExclusive(&aimLock);aimInput.lx=lx;aimInput.ly=ly;aimInput.lt=32767;ReleaseSRWLockExclusive(&aimLock);
  *buttons=0x10|(adsCase==5?0x2000:0)|(lx<0?1:lx>0?2:0)|(ly<0?8:ly>0?4:0);
 }
 if(adsTicks==151){float distance=hypotf(PF(1864)-adsStartX,PF(1872)-adsStartZ);aimCheck(PI(332),"LT remains engaged while moving");aimCheck(distance>40&&distance<400,"ADS left/right/forward/back/diagonal actually displaces native body");aimCheck(PI(184)==(adsCase==7?0:2),"left stick cannot trigger native crouch");if(adsCase>=4)aimCheck(fabs(PF(380)-adsYaw)>1,"can turn and circle while moving/aiming");fprintf(logfile,"PASS ADS movement case=%d displacement=%.2f yaw=%.2f\n",adsCase,distance,PF(380)-adsYaw);fflush(logfile);aimSubmit(0,NUMBER,(uintptr_t)CURRENT,0,0);adsTicks=89;if(++adsCase==8)ExitProcess(0);}
}
