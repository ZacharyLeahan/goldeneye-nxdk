/* Confirmed-hit health display. Engine state is read only on the simulation
 * thread; the render thread receives values and projected positions, no pointers.
 * Timing was measured from the supplied roughly 60 fps gameplay clip. */
#define HEALTH_HOLD_SECONDS 3.25f
#define HEALTH_FADE_SECONDS .20f
#define HEALTH_TARGETS 32
#define HB_PTR(p,o) (*(void**)((unsigned char*)(p)+(o)))
#define HB_FLOAT(p,o) (*(float*)((unsigned char*)(p)+(o)))
typedef struct {float hp,armor,total;int alive,slot;} HealthValue;
typedef struct {void*prop,*chr,*model;short identity;int used,shown,dead;float hp,armor,total,age,trail,pending,baseline,deathAge;} HealthTarget;
typedef struct {float x,y,hp,armor,trail,alpha;char name[32];} HealthBar;
typedef struct {int count;HealthBar bars[HEALTH_TARGETS];} HealthSnapshot;
static HealthTarget healthTargets[HEALTH_TARGETS];
static HealthSnapshot healthPublished;
static SRWLOCK healthLock=SRWLOCK_INIT;
static void*healthOwner;static int healthStage,healthTimer,healthReady,healthConfirmations;
static DWORD healthThread;
static int (*healthOriginalHit)(void*,int,JumpVec*,int,int);
static float healthClamp(float x){return fmaxf(0,fminf(1,x));}
static float healthAlpha(float age){float t=healthClamp((age-HEALTH_HOLD_SECONDS)/HEALTH_FADE_SECONDS);return (1-t)*(1-t);}
static int healthPropIndex(void*p){uintptr_t first=base+0x322740;uintptr_t d=(uintptr_t)p-first;return (uintptr_t)p>=first&&d<600*96&&d%96==0?(int)(d/96):-1;}
static int healthIdentity(HealthTarget*t){return healthPropIndex(t->prop)>=0&&(*(unsigned char*)t->prop==3||*(unsigned char*)t->prop==6)&&HB_PTR(t->prop,8)==t->chr&&t->chr&&HB_PTR(t->chr,32)==t->prop&&*(short*)t->chr==t->identity&&HB_PTR(t->chr,40)==t->model;}
static int healthRead(void*prop,HealthValue*out){
 memset(out,0,sizeof(*out));out->slot=-1;if(healthPropIndex(prop)<0)return 0;
 int type=*(unsigned char*)prop;void*chr=HB_PTR(prop,8);if((type!=3&&type!=6)||!chr||HB_PTR(chr,32)!=prop)return 0;
 int action=*((unsigned char*)chr+8);out->alive=action!=4&&action!=5;
 if(type==6){
  for(int i=0;i<32;i++){void*p=PLAYERS[i];if(p&&HB_PTR(p,200)==prop){out->slot=i;out->hp=HB_FLOAT(p,260);out->armor=HB_FLOAT(p,264);out->alive=out->alive&&!*(int*)((unsigned char*)p+256);break;}}
  if(out->slot<0)return 0;
 }else{float maximum=HB_FLOAT(chr,320),damage=HB_FLOAT(chr,316);if(!isfinite(maximum)||maximum<=0||!isfinite(damage))return 0;out->hp=(maximum-fmaxf(0,damage))/maximum;out->armor=fmaxf(0,-damage)/maximum;}
 if(!isfinite(out->hp)||!isfinite(out->armor))return 0;
 out->total=fmaxf(0,out->hp)+fmaxf(0,out->armor);out->hp=healthClamp(out->hp);out->armor=healthClamp(out->armor);if(out->hp<=0)out->alive=0;return 1;
}
static HealthTarget*healthFind(void*prop,int create){
 HealthTarget*freeSlot=NULL,*oldest=NULL;
 for(int i=0;i<HEALTH_TARGETS;i++){HealthTarget*t=&healthTargets[i];if(t->used&&t->prop==prop){if(healthIdentity(t))return t;memset(t,0,sizeof(*t));}if(!t->used&&!freeSlot)freeSlot=t;if(t->used&&(!oldest||t->age>oldest->age))oldest=t;}
 if(!create)return NULL;HealthTarget*t=freeSlot?freeSlot:oldest;if(!t)return NULL;memset(t,0,sizeof(*t));t->used=1;t->prop=prop;t->chr=HB_PTR(prop,8);t->identity=*(short*)t->chr;t->model=HB_PTR(t->chr,40);t->age=100;return t;
}
static void healthConfirm(HealthTarget*t,HealthValue v){t->trail=fmaxf(t->trail,t->hp);t->hp=v.hp;t->armor=v.armor;t->total=v.total;t->age=0;t->shown=1;t->pending=0;t->dead=!v.alive;t->deathAge=0;healthConfirmations++;}
static int healthHit(void*chr,int part,JumpVec*direction,int weapon,int isPlayer){
 void*prop=chr?HB_PTR(chr,32):NULL;HealthValue before={0},after={0};
 int observe=healthReady&&GetCurrentThreadId()==healthThread&&NUMBER==localIndex()&&CURRENT==healthOwner&&isPlayer&&prop!=PP(200)&&healthRead(prop,&before)&&before.alive;
 int result=healthOriginalHit(chr,part,direction,weapon,isPlayer);
 if(observe&&healthRead(prop,&after)){
  HealthTarget*t=healthFind(prop,1);if(t){if(!t->shown){t->hp=before.hp;t->armor=before.armor;t->total=before.total;t->trail=before.hp;}
   if(after.total<before.total-.00001f)healthConfirm(t,after);
   /* A remote hit may await authoritative health replication. Never invent HP
    * or show a bar for an invulnerable/blocked hit. */
   else if(result&&part!=100&&part!=110){t->pending=.5f;t->baseline=before.total;}
  }
 }
 return result;
}
static void healthBegin(void){
 int stage=*(int*)(base+0x303000+211452),timer=*(int*)(base+0x303000+211432);
 if(healthOwner!=CURRENT||healthStage!=stage||timer<healthTimer){memset(healthTargets,0,sizeof(healthTargets));healthOwner=CURRENT;healthStage=stage;}
 healthTimer=timer;healthThread=GetCurrentThreadId();healthReady=jumpAllowed();
 if(CURRENT&&PI(256))memset(healthTargets,0,sizeof(healthTargets));
}
/* Background + dynamic object sight test. Actor hit caches are never accessed. */
static int healthSight(JumpVec eye,JumpVec target){
 float dx=target.x-eye.x,dy=target.y-eye.y,dz=target.z-eye.z,len2=dx*dx+dy*dy+dz*dz;if(!isfinite(len2)||len2<1)return 0;
 if(!collisionPrepareBox((JumpVec){fminf(eye.x,target.x),fminf(eye.y,target.y),fminf(eye.z,target.z)},(JumpVec){fmaxf(eye.x,target.x),fmaxf(eye.y,target.y),fmaxf(eye.z,target.z)}))return 0;
 int total=*(int*)(base+0x228000+4208);float scale=*(float*)(base+0x228000+4252);if(total<1||total>2048||!isfinite(scale)||scale<=0)return 0;
 unsigned char*rooms=(void*)(base+0x303000+27264);
 for(int r=0;r<total;r++)if(*(void**)(rooms+r*0x68+0x40)){JumpHit hit={0};if(FN(int(*)(JumpVec*,JumpVec*,int,JumpHit*),0x1000+13744)(&eye,&target,r,&hit)){float t=((hit.position.x/scale-eye.x)*dx+(hit.position.y/scale-eye.y)*dy+(hit.position.z/scale-eye.z)*dz)/len2;if(isfinite(t)&&t>.001f&&t<.995f)return 0;}}
 int roomsHere[64];for(int i=0;i<64;i++)roomsHere[i]=-1;FN(void(*)(void*,int*),0x1000+275920)(PP(200),roomsHere);JumpVec pos,normal;
 return !FN(int(*)(void*,JumpVec*,JumpVec*,unsigned,JumpVec*,JumpVec*,int*),0x1000+721344)(PP(200),&eye,&target,0x13,&pos,&normal,roomsHere);
}
static void healthName(int slot,char out[32]){
 const char*name="GUARD";if(slot>=0){if(FN(int(*)(void),0xf4d70)())name=FN(const char*(*)(int),0xf4d80)(slot);else name=FN(const char*(*)(int),0x138b70)(slot);if(!name||!*name)name="PLAYER";}
 int n=0;for(;name[n]&&n<24;n++){unsigned char c=(unsigned char)name[n];out[n]=c>=32&&c<127?(char)c:'?';}out[n]=0;
}
static void healthPublish(void){
 HealthSnapshot s={0};float dt=*(float*)(base+0x303000+211436)/60.f;if(!isfinite(dt)||dt<0)dt=0;dt=fminf(dt,.25f);
 int allowed=healthReady&&jumpAllowed();
 for(int i=0;i<HEALTH_TARGETS;i++){
  HealthTarget*t=&healthTargets[i];if(!t->used)continue;if(!healthIdentity(t)){if(getenv("GE32_HEALTH_TRACE")&&logfile){fprintf(logfile,"health removed identity slot=%d age=%.3f\n",i,t->age);fflush(logfile);}memset(t,0,sizeof(*t));continue;}
  HealthValue v;if(!healthRead(t->prop,&v)){memset(t,0,sizeof(*t));continue;}
  if(t->dead&&v.alive){if(getenv("GE32_HEALTH_TRACE")&&logfile){fprintf(logfile,"health life reset slot=%d age=%.3f hp=%.3f\n",i,t->age,v.hp);fflush(logfile);}memset(t,0,sizeof(*t));continue;}
  if(allowed){t->age+=dt;t->pending=fmaxf(0,t->pending-dt);if(t->pending>0&&v.total<t->baseline-.00001f)healthConfirm(t,v);}
  if(t->shown){t->hp=v.hp;t->armor=v.armor;t->total=v.total;if(!v.alive&&!t->dead){t->dead=1;t->deathAge=0;}if(t->dead&&allowed)t->deathAge+=dt;if(t->age>.16f)t->trail=fmaxf(t->hp,t->trail-dt*1.8f);}
  if((!t->shown&&t->pending<=0)||t->age>=HEALTH_HOLD_SECONDS+HEALTH_FADE_SECONDS||(t->dead&&t->deathAge>.35f)){if(getenv("GE32_HEALTH_TRACE")&&logfile){fprintf(logfile,"health expired slot=%d age=%.3f dead=%d deathAge=%.3f\n",i,t->age,t->dead,t->deathAge);fflush(logfile);}memset(t,0,sizeof(*t));continue;}
  if(!allowed||!t->shown)continue;
  float*polygon=NULL,top=0,bottom=0;int edges=0;FN(void(*)(void*,float**,int*,float*,float*),0x44840)(t->prop,&polygon,&edges,&top,&bottom);
  if(!polygon||edges<1||edges>32||!isfinite(top)||!isfinite(bottom))continue;
  JumpVec p={HB_FLOAT(t->prop,16),top+30,HB_FLOAT(t->prop,24)},body={p.x,(top+bottom)*.5f,p.z};float xy[2];
  if(!padProject(p,xy)||xy[0]<.025f||xy[0]>.975f||xy[1]<.035f||xy[1]>.95f)continue;
  float*view=FN(float*(*)(void),0x1000+53600)();if(!view)continue;JumpVec eye={view[12],view[13],view[14]};if(!healthSight(eye,body))continue;
  HealthBar*b=&s.bars[s.count++];b->x=xy[0];b->y=xy[1];b->hp=t->hp;b->armor=t->armor;b->trail=t->trail;b->alpha=healthAlpha(t->age);if(t->dead)b->alpha*=1-healthClamp((t->deathAge-.15f)/.20f);healthName(v.slot,b->name);
 }
 AcquireSRWLockExclusive(&healthLock);healthPublished=s;ReleaseSRWLockExclusive(&healthLock);
}
static HealthSnapshot healthSnapshot(void){HealthSnapshot s;AcquireSRWLockShared(&healthLock);s=healthPublished;ReleaseSRWLockShared(&healthLock);return s;}
static int installHealthHook(void){
 unsigned char expected[]={0x55,0x41,0x55,0x41,0x54,0x57,0x56,0x53,0x48,0x83,0xec,0x68,0x48,0x8d,0x6c,0x24,0x50};
 unsigned char*target=(void*)(base+0x31230);if(memcmp(target,expected,sizeof(expected)))return 0;
 unsigned char*t=VirtualAlloc(NULL,64,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!t)return 0;memcpy(t,target,17);t[17]=0xff;t[18]=0x25;memset(t+19,0,4);*(uintptr_t*)(t+23)=(uintptr_t)(target+17);
 DWORD old;if(!VirtualProtect(t,64,PAGE_EXECUTE_READ,&old)){VirtualFree(t,0,MEM_RELEASE);return 0;}healthOriginalHit=(void*)t;
 unsigned char j[17]={0xff,0x25,0,0,0,0};*(uintptr_t*)(j+6)=(uintptr_t)healthHit;j[14]=j[15]=j[16]=0x90;
 if(!VirtualProtect(target,17,PAGE_EXECUTE_READWRITE,&old))return 0;memcpy(target,j,17);VirtualProtect(target,17,old,&old);FlushInstructionCache(GetCurrentProcess(),target,17);return 1;
}
