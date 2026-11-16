/* Offline precision and native-camera acceptance. No synthetic input normally. */
static short axisHook(void*,int);
static unsigned char(*aimTestRealButton)(void*,int);
static unsigned char aimTestButton(void*p,int b);
static short(*aimTestRealAxis)(void*,int);static int aimTestDevice;
static unsigned char aimTestButton(void*p,int b){if(p==&aimTestDevice)return 0;return aimTestRealButton(p,b);}
static short aimTestAxis(void*p,int a){if(p==&aimTestDevice)return a==2?16000:a==3?8000:0;return aimTestRealAxis(p,a);}
static int aimTestTicks,aimTestCase,aimTestStep,aimTestSamples;static double aimTestYaw,aimTestPitch,aimTestExpected[2];static PrecisionAimState aimTestOracle;
static void aimCheck(int ok,const char*why){if(!ok){fprintf(logfile,"FAIL aim: %s case=%d step=%d\n",why,aimTestCase,aimTestStep);fflush(logfile);ExitProcess(2);}}
static double aimTestTotal(int hz){PrecisionAimState s={0};double sum=0,out[2];for(int i=0;i<hz;i++){aimResponse(18000,0,4000,1,65,60,45,1./hz,out,&s);sum+=out[0];}return sum;}
static void testPrecisionAim(void){
 double previous=0;for(int raw=4001;raw<4700;raw++){PrecisionAimState s={0};double out[2];aimResponse(raw,0,4000,1,65,60,60,1./60,out,&s);aimCheck(out[0]>previous&&out[1]==0,"sub-N64-step inputs retain continuous precision");previous=out[0];}
 aimCheck(fabs(aimTestTotal(30)-aimTestTotal(60))<1e-9&&fabs(aimTestTotal(144)-aimTestTotal(60))<1e-9,"time-based response consistent at 30/60/144 Hz");
 PrecisionAimState a={0},b={0};double x[2],y[2];aimResponse(16000,10000,4000,1,65,60,60,1./60,x,&a);aimResponse(16000,10000,4000,1,65,60,30,1./60,y,&b);aimCheck(fabs(x[0]*(tan(3.141592653589793/12.)/tan(3.141592653589793/6.))-y[0])<1e-12&&x[1]<0,"FOV scaling and installed pitch direction");
 PrecisionAimState s={0},fresh={0};double step[2],direct[2];aimResponse(20000,0,4000,1,65,60,60,1./60,step,&s);aimResponse(9800,0,4000,1,65,60,60,1./60,step,&s);aimResponse(9800,0,4000,1,65,60,60,1./60,direct,&fresh);aimCheck(fabs(step[0]-direct[0])<1e-12,"base look has zero time-dependent acceleration");double out[2];aimResponse(0,0,4000,1,65,60,60,1./60,out,&s);aimCheck(out[0]==0&&s.velocity[0]==0,"center stops without drift");aimResponse(-16000,0,4000,1,65,60,60,1./60,out,&s);aimCheck(out[0]<0,"reversal never coasts in the wrong direction");
 fprintf(logfile,"PASS aim math: continuous small inputs, 30/60/144 Hz equivalence, FOV/pitch, zero acceleration, center stop and reversal\n");fflush(logfile);
}
static void precisionAimTestBefore(signed char*x,signed char*y,unsigned short*buttons){
 if(!getenv("GE32_AIM_TEST")||!jumpAllowed())return;*x=*y=0;*buttons=aimTestCase>=12?0:0x10;aimCheck(++aimTestTicks<2400,"native acceptance deadline");
 if(aimTestTicks==90){padTestPlacePlayer(-150,100,-460,180);PF(396)=0;FN(int(*)(int),0x1000+44224)(5);swapNow(0,5);testPrecisionAim();if(getenv("GE32_AIM_START_CASE"))aimTestCase=atoi(getenv("GE32_AIM_START_CASE"));}
}
static void precisionAimTestFeed(void){
 if(!getenv("GE32_AIM_TEST")||aimTestTicks<150||!jumpAllowed())return;aimCheck((PI(332)!=0)==(aimTestCase<12),"native aim/hip state matches fixture");aimTestStep++;
 int x=aimTestCase==0?4200+aimTestStep*25:aimTestCase==1?(aimTestStep&1?10200:9800):aimTestCase==2?0:16000,y=aimTestCase==3?8000:0;
 if(aimTestCase>=12){x=aimTestCase==13?0:32767;y=aimTestCase==13?32767:0;if(aimTestCase==14&&aimTestStep>60)x=0;}
 if(aimTestStep==1){precisionAimReset();memset(&aimTestOracle,0,sizeof(aimTestOracle));}
 aimSubmit(1,NUMBER,(uintptr_t)CURRENT,x,y);aimTestYaw=PF(380);aimTestPitch=PF(396);
 double dt=fmin(*(float*)(base+0x303000+211436)/60.,.05),fov=FN(float(*)(void),0x1000+53952)();aimLookResponse(x,y,*(int*)(base+2464832),*(int*)(base+5141700),*(int*)(base+2464824),*(int*)(base+2464820),fov,dt,aimTestCase<12,aimTestExpected,&aimTestOracle);
 aimTestExpected[1]=fmax(-90,fmin(90,aimTestPitch+aimTestExpected[1]))-aimTestPitch;
 if(aimTestCase==4){aimSubmit(1,31,(uintptr_t)CURRENT,x,y);aimTestExpected[0]=aimTestExpected[1]=0;}
 if(aimTestCase==5){aimSubmit(1,NUMBER,(uintptr_t)CURRENT+8,x,y);aimTestExpected[0]=aimTestExpected[1]=0;}
 if(aimTestCase==6){openWheel=1;aimTestExpected[0]=aimTestExpected[1]=0;}
 if(aimTestCase>=8&&aimTestCase<=11){
  InventorySnapshot saved=snapshotRead(),view=saved;view.aiming=aimTestCase!=9;publishInventory(NUMBER,&view);void*priorPad=pad;aimTestRealAxis=getAxis;getAxis=aimTestAxis;aimTestRealButton=getButton;getButton=aimTestButton;int mode=*(int*)(base+5141904);if(aimTestCase==10)*(int*)(base+5141904)=0;if(aimTestCase==11)openWheel=1;
  short masked=axisHook(&aimTestDevice,2);if(masked!=(aimTestCase==10?16000:0)){fprintf(logfile,"aim adapter diag masked=%d aiming=%d wheel=%ld mode=%d snapshotLocal=%d NUMBER=%d\n",masked,snapshotRead().aiming,openWheel,*(int*)(base+5141904),snapshotRead().local,NUMBER);fflush(logfile);}aimCheck(masked==(aimTestCase==10?16000:0),"centred hip/ADS mask native look; legacy passes raw");
  if(aimTestCase==8){aimCheck(((double*)(base+5141664))[0]==0&&((double*)(base+5141632))[0]==0,"native integer EMA cannot add a second turn");}
  getAxis=aimTestRealAxis;getButton=aimTestRealButton;pad=priorPad;*(int*)(base+5141904)=mode;openWheel=0;publishInventory(NUMBER,&saved);aimSubmit(0,NUMBER,(uintptr_t)CURRENT,0,0);aimTestExpected[0]=aimTestExpected[1]=0;
 }
 if(aimTestCase==7){AcquireSRWLockExclusive(&aimLock);aimInput.time=0;ReleaseSRWLockExclusive(&aimLock);aimTestExpected[0]=aimTestExpected[1]=0;}
}
static void precisionAimTestAfter(void){
 if(!getenv("GE32_AIM_TEST")||aimTestTicks<150||!jumpAllowed())return;
 aimCheck(fabs((PF(380)-aimTestYaw)-aimTestExpected[0])<.00007,"native yaw matches continuous response without duplicate legacy input");aimCheck(fabs((PF(396)-aimTestPitch)-aimTestExpected[1])<.00007,"native pitch matches continuous response");aimTestSamples++;
 if(aimTestCase==6)openWheel=0;
 if(aimTestStep==120){fprintf(logfile,"PASS native aim case=%d samples=%d\n",aimTestCase,aimTestStep);fflush(logfile);aimTestStep=0;if(++aimTestCase==15){fprintf(logfile,"PASS precision hip/ADS: %d native ticks; fine ramp, jitter, release, diagonal, foreign player/life, wheel and stale-input guards\n",aimTestSamples);fflush(logfile);ExitProcess(0);}}
}
