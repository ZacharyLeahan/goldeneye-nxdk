/* High-resolution controller look adapter. Input callbacks publish values only;
 * the local simulation thread applies camera angles once per gameplay tick. */
#include "../../port/controls/finals_aim.inc"
static struct {int active,local,x,y,lx,ly,lt;uintptr_t context;ULONGLONG time;} aimInput;
static SRWLOCK aimLock=SRWLOCK_INIT;
static PrecisionAimState preciseAim;
static void aimSubmit(int active,int local,uintptr_t context,int x,int y){AcquireSRWLockExclusive(&aimLock);aimInput.active=active;aimInput.local=local;aimInput.context=context;aimInput.x=x;aimInput.y=y;aimInput.time=GetTickCount64();ReleaseSRWLockExclusive(&aimLock);}
static int precisionAimCapture(void*p){
 InventorySnapshot s=snapshotRead();if(!playing()||!s.allowJump||openWheel||*(int*)(base+5141904)!=1){aimSubmit(0,s.local,s.context,0,0);return 0;}
 int x=getAxis(p,2),y=getAxis(p,3),lx=getAxis(p,0),ly=getAxis(p,1),lt=getAxis(p,4);
 AcquireSRWLockExclusive(&aimLock);aimInput.active=1;aimInput.local=s.local;aimInput.context=s.context;aimInput.x=x;aimInput.y=y;aimInput.lx=lx;aimInput.ly=ly;aimInput.lt=lt;aimInput.time=GetTickCount64();ReleaseSRWLockExclusive(&aimLock);
 /* Native integer look must contribute zero, including its old EMA tail.
  * These input-filter arrays belong to this same controller-poll path. */
 int slot=*(int*)(base+2464948);if(slot<0||slot>3)slot=0;((double*)(base+5141664))[slot]=0;((double*)(base+5141632))[slot]=0;
 int dz=*(int*)(base+2464832);if(hypot((double)x,(double)y)>dz)*(int*)(base+5141892)=0;
 return 1;
}
static void precisionAimReset(void){preciseAim.boost=0;preciseAim.boostDirection=0;memset(preciseAim.velocity,0,sizeof(preciseAim.velocity));memset(preciseAim.remainder,0,sizeof(preciseAim.remainder));}
static void precisionAimTick(void){
 int stage=*(int*)(base+0x303000+211452),timer=*(int*)(base+0x303000+211432);
 if(preciseAim.owner!=CURRENT||preciseAim.stage!=stage||timer<preciseAim.timer){precisionAimReset();preciseAim.owner=CURRENT;preciseAim.stage=stage;}preciseAim.timer=timer;
 AcquireSRWLockShared(&aimLock);int active=aimInput.active,local=aimInput.local,x=aimInput.x,y=aimInput.y;uintptr_t context=aimInput.context;ULONGLONG age=GetTickCount64()-aimInput.time;ReleaseSRWLockShared(&aimLock);
 if(!active||local!=NUMBER||context!=(uintptr_t)CURRENT||age>100||!jumpAllowed()||openWheel||*(int*)(base+5141904)!=1){precisionAimReset();return;}
 double dt=*(float*)(base+0x303000+211436)/60.,fov=FN(float(*)(void),0x1000+53952)();if(!isfinite(dt)||dt<=0||!isfinite(fov)||fov<1||fov>160){precisionAimReset();return;}dt=fmin(dt,.05);
 double delta[2];aimLookResponse(x,y,*(int*)(base+2464832),*(int*)(base+5141700),*(int*)(base+2464824),*(int*)(base+2464820),fov,dt,PI(332)!=0,delta,&preciseAim);
 int fields[2]={380,396};for(int i=0;i<2;i++){double wanted=PF(fields[i])+delta[i]+preciseAim.remainder[i];if(i==1)wanted=fmax(-90,fmin(90,wanted));float result=(float)wanted;preciseAim.remainder[i]=wanted-result;PF(fields[i])=result;}
}

/* Separate modern movement from the original aim-mode C-button commands.
 * Read-only input sampling; all stance/motion writes stay on the local tick. */
static int aimMoveReady(unsigned short buttons){
 AcquireSRWLockShared(&aimLock);int ok=aimInput.active&&aimInput.local==NUMBER&&aimInput.context==(uintptr_t)CURRENT&&GetTickCount64()-aimInput.time<=100&&aimInput.lt>*(int*)(base+2464944)*327;ReleaseSRWLockShared(&aimLock);
 return ok&&jumpAllowed()&&!openWheel&&!slap.phase&&!*(int*)(base+0x303000+65912)&&(buttons&0x30)&&*(int*)(base+5141904)==1;
}
static void aimMoveAfter(void){
 AcquireSRWLockShared(&aimLock);double x=aimInput.lx,y=aimInput.ly;ReleaseSRWLockShared(&aimLock);
 double length=hypot(x,y),dz=*(int*)(base+2464836);if(dz<0||dz>30000)dz=4000;
 double strength=length>dz?fmin(1,(length-dz)/(32767.-dz)):0;
 PF(416)=length?x/length*strength*.90:0;PF(424)=length?-y/length*strength*1.08*.90:0;
}
