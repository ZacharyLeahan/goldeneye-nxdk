/* High-resolution controller ADS adapter. Input callbacks publish values only;
 * the local simulation thread applies camera angles once per gameplay tick. */
#define AIM_TAU .030
#define AIM_ADS_GAIN .80
static struct {int active,local,x,y;uintptr_t context;ULONGLONG time;} aimInput;
static SRWLOCK aimLock=SRWLOCK_INIT;
typedef struct {double velocity[2],remainder[2];void*owner;int stage,timer;} PrecisionAimState;
static PrecisionAimState preciseAim;
static void aimSubmit(int active,int local,uintptr_t context,int x,int y){AcquireSRWLockExclusive(&aimLock);aimInput.active=active;aimInput.local=local;aimInput.context=context;aimInput.x=x;aimInput.y=y;aimInput.time=GetTickCount64();ReleaseSRWLockExclusive(&aimLock);}
static double aimIntegrate(double target,double dt,double*velocity){
 if(target==0){*velocity=0;return 0;}if(target*(*velocity)<0)*velocity=0;
 double decay=exp(-dt/AIM_TAU),old=*velocity;*velocity=target+(old-target)*decay;
 return target*dt+(old-target)*AIM_TAU*(1-decay);
}
static void aimResponse(int rawX,int rawY,int deadzone,int invert,int sensX,int sensY,double fov,double dt,double out[2],PrecisionAimState*state){
 double x=rawX,y=rawY,length=hypot(x,y);if(deadzone<0)deadzone=0;if(deadzone>30000)deadzone=30000;
 double strength=length>deadzone?fmin(1,(length-deadzone)/(32767.-deadzone)):0;
 if(length>0){x=x/length*strength;y=y/length*strength;}else x=y=0;
 /* SDL down is positive; preserve the installed profile's pitch convention. */
 y*=invert?-1:1;double input[2]={x,y},sensitivity[2]={sensX/100.,sensY/100.};
 for(int i=0;i<2;i++){double v=input[i]*80./70.*sensitivity[i];v=fmax(-1,fmin(1,v));double target=v*fabs(v)*210.*(fov/60.)*AIM_ADS_GAIN;out[i]=aimIntegrate(target,dt,&state->velocity[i]);if(!target)state->remainder[i]=0;}
}
static int precisionAimCapture(void*p){
 InventorySnapshot s=snapshotRead();if(!playing()||!s.allowJump||!s.aiming||openWheel||*(int*)(base+5141904)!=1){aimSubmit(0,s.local,s.context,0,0);return 0;}
 int x=getAxis(p,2),y=getAxis(p,3);aimSubmit(1,s.local,s.context,x,y);
 /* Native integer look must contribute zero, including its old EMA tail.
  * These input-filter arrays belong to this same controller-poll path. */
 int slot=*(int*)(base+2464948);if(slot<0||slot>3)slot=0;((double*)(base+5141664))[slot]=0;((double*)(base+5141632))[slot]=0;
 int dz=*(int*)(base+2464832);if(hypot((double)x,(double)y)>dz)*(int*)(base+5141892)=0;
 return 1;
}
static void precisionAimReset(void){memset(preciseAim.velocity,0,sizeof(preciseAim.velocity));memset(preciseAim.remainder,0,sizeof(preciseAim.remainder));}
static void precisionAimTick(void){
 int stage=*(int*)(base+0x303000+211452),timer=*(int*)(base+0x303000+211432);
 if(preciseAim.owner!=CURRENT||preciseAim.stage!=stage||timer<preciseAim.timer){precisionAimReset();preciseAim.owner=CURRENT;preciseAim.stage=stage;}preciseAim.timer=timer;
 AcquireSRWLockShared(&aimLock);int active=aimInput.active,local=aimInput.local,x=aimInput.x,y=aimInput.y;uintptr_t context=aimInput.context;ULONGLONG age=GetTickCount64()-aimInput.time;ReleaseSRWLockShared(&aimLock);
 if(!active||local!=NUMBER||context!=(uintptr_t)CURRENT||age>100||!jumpAllowed()||!PI(332)||openWheel||*(int*)(base+5141904)!=1){precisionAimReset();return;}
 double dt=*(float*)(base+0x303000+211436)/60.,fov=FN(float(*)(void),0x1000+53952)();if(!isfinite(dt)||dt<=0||!isfinite(fov)||fov<1||fov>160){precisionAimReset();return;}dt=fmin(dt,.05);
 double delta[2];aimResponse(x,y,*(int*)(base+2464832),*(int*)(base+5141700),*(int*)(base+2464824),*(int*)(base+2464820),fov,dt,delta,&preciseAim);
 int fields[2]={380,396};for(int i=0;i<2;i++){double wanted=PF(fields[i])+delta[i]+preciseAim.remainder[i];if(i==1)wanted=fmax(-90,fmin(90,wanted));float result=(float)wanted;preciseAim.remainder[i]=wanted-result;PF(fields[i])=result;}
}
