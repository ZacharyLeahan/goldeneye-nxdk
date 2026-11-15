/* Offline simulation fixtures; dormant unless GE32_DROP_TEST is set. */
static int dropTestTicks,dropTestPlaced,dropTestPhase;static float dropTestHealth;
static void dropFixture(void){
 if(logfile){fprintf(logfile,"fixture actual stage=%d before floor=%.2f\n",*(int*)(base+0x303000+211452),PF(144));fflush(logfile);}
 *(int*)(base+0x303000+618564)=1;
 float y=NAN;void*t=floorBelow(-165.3444f,470.f,-807.1017f,&y);
 if(!t||fabsf(y-468.7878f)>2){if(logfile){fprintf(logfile,"FAIL drop fixture upper floor y=%.2f tile=%p\n",y,t);fflush(logfile);}ExitProcess(2);}
 PP(1856)=t;PP(56)=t;PF(1864)=-165.3444f;PF(1872)=-807.1017f;
 PF(140)=PF(144)=y;PF(136)=y/.2004f;PF(152)=0;PF(380)=354.0379f;
 PI(14124)=0;PP(14128)=NULL;PI(184)=PI(14004)=2;jumpActive=0;drop.active=0;
 FN(void(*)(void),0x1000+86960)();FN(void(*)(void),0x1000+114768)();
 dropTestHealth=FN(float(*)(void),0x1000+124352)();
 if(logfile){fprintf(logfile,"Temple fixture placed floor=%.2f health=%.3f\n",y,dropTestHealth);fflush(logfile);}
}
static void dropTestBefore(unsigned short*buttons){
 if(!getenv("GE32_DROP_TEST"))return;
 *buttons=0;
 if(!jumpAllowed())return;
 if(++dropTestTicks==300){dropFixture();dropTestPlaced=1;}
 if(dropTestPlaced){PF(380)=354.0379f;FN(void(*)(void),0x1000+87712)();PF(1876)=.10386993f;PF(1884)=.99459088f;}
 if(dropTestPlaced&&dropTestTicks>330&&dropTestTicks<430)*buttons=8;
 if(dropTestPhase==1&&dropTestTicks==348)InterlockedExchange(&pendingJump,1);
 if(dropTestPhase==2&&dropTestPlaced)PI(184)=1;
}
static void dropTestAfter(void){
 if(!getenv("GE32_DROP_TEST")||!dropTestPlaced)return;
 if(dropTestTicks==350&&logfile){fprintf(logfile,"direction yaw=%.2f forward=%.3f,%.3f xz=%.2f,%.2f\n",PF(380),PF(1876),PF(1884),PF(1864),PF(1872));fflush(logfile);}
 if(dropTestTicks==510){
  int ok=PF(140)<-460&&fabsf(PF(140)-PF(144))<.3f&&fabsf(FN(float(*)(void),0x1000+124352)()-dropTestHealth)<.001f&&drop.accepted>0;
  if(logfile){fprintf(logfile,"%s DROP phase=%d result",ok?"PASS":"FAIL",dropTestPhase);fprintf(logfile," x=%.2f z=%.2f foot=%.2f floor=%.2f health=%.3f accepted=%d\n",PF(1864),PF(1872),PF(140),PF(144),FN(float(*)(void),0x1000+124352)(),drop.accepted);fflush(logfile);}
  if(!ok)ExitProcess(2);
  if(++dropTestPhase<3){dropTestTicks=299;return;}
  JumpVec a={-165.3444f,550.f,-807.1017f},b={a.x,-460.f,a.z};
  int solid=!dropSegmentClear(a,b);a.x+=31.16f;a.z+=298.38f;b.x=a.x;b.z=a.z;int opening=dropSegmentClear(a,b);
  if(logfile){fprintf(logfile,"%s rendered deck blocks trace, opening passes trace\n",solid&&opening?"PASS":"FAIL");fflush(logfile);}
  ExitProcess(solid&&opening?0:2);

 }
}

static void dropTestLaunch(void){static int frames,launched;if((getenv("GE32_DROP_TEST")||getenv("GE32_REGRESSION_PRACTICE"))&&!launched&&++frames==60){launched=1;FN(void(*)(int,int),0x1000+999872)(1,0);if(logfile){fprintf(logfile,"drop test launched native Temple practice\n");fflush(logfile);}}}
