/* Sprint-to-crouch momentum uses native movement/collision, never teleports.
 * All mutable slide state is simulation-thread-owned except input edge requests. */
static volatile LONG slideRequests;
static int slideBHeld;
static struct {void*owner;int stage,timer,active,starts,blocked,ready;float elapsed,cooldown,entry,dx,dz,lastX,lastZ,lastForward,lastSide;} slide;
static void slideButton(unsigned char raw,int allowed){if(raw&&!slideBHeld&&allowed)InterlockedExchange(&slideRequests,1);slideBHeld=raw!=0;}
static int slideGrounded(void){return fabsf(PF(140)-PF(144))<1.5f&&fabsf(PF(152))<.6f;}
static void slideStop(void){slide.active=slide.ready=0;slide.cooldown=18;}
static void slideBefore(void){
 int stage=*(int*)(base+0x303000+211452),timer=*(int*)(base+0x303000+211432);
 if(slide.owner!=CURRENT||slide.stage!=stage||timer<slide.timer){memset(&slide,0,sizeof(slide));slide.owner=CURRENT;slide.stage=stage;InterlockedExchange(&slideRequests,0);}slide.timer=timer;
 int request=InterlockedExchange(&slideRequests,0);float dt=*(float*)(base+0x303000+211436);if(!isfinite(dt)||dt<0)dt=0;slide.cooldown=fmaxf(0,slide.cooldown-dt);
 if(!jumpAllowed()||openWheel||padCarrying||slap.phase){slideStop();return;}
 if(slide.active){
  float moved=hypotf(PF(1864)-slide.lastX,PF(1872)-slide.lastZ);slide.blocked=moved<.25f?slide.blocked+1:0;
  if(request||pendingJump||!slideGrounded()||PI(184)==2||(slide.elapsed>6&&slide.blocked>=3))slideStop();
 /* The custom input adapter may already have applied crouch this tick.
  * Prior sprintOn proves the entry came from a standing sprint. */
 }else if(request&&slide.cooldown<=0&&sprintOn&&sprintBlend>.3f&&slideGrounded()&&!pendingJump){
  float a=PF(380)*.01745329252f,f=slide.lastForward,s=slide.lastSide,v=hypotf(f,s);
  if(v>1.15f&&hypotf(PF(1864)-slide.lastX,PF(1872)-slide.lastZ)>fmaxf(.1f,dt)){slide.dx=(-sinf(a)*f-cosf(a)*s)/v;slide.dz=(cosf(a)*f-sinf(a)*s)/v;slide.entry=fminf(v,1.65f);slide.ready=1;}
 }
}
static void slideAfter(void){
 if(slide.ready){slide.ready=0;slide.active=1;slide.starts++;slide.elapsed=0;slide.blocked=0;PI(184)=0;if(logfile){fprintf(logfile,"slide start=%d entry=%.3f\n",slide.starts,slide.entry);fflush(logfile);}sprintLatched=sprintOn=0;sprintBlend=0;}
 if(slide.active){
  if(!jumpAllowed()||openWheel||padCarrying||slap.phase||pendingJump||!slideGrounded()){slideStop();return;}
  float dt=*(float*)(base+0x303000+211436);if(!isfinite(dt)||dt<=0)return;slide.elapsed+=dt;
  if(slide.elapsed>=51){slideStop();return;}
  float a=PF(380)*.01745329252f,f=PF(424),s=PF(416),length=hypotf(f,s);
  if(length>.1f){float dx=(-sinf(a)*f-cosf(a)*s)/length,dz=(cosf(a)*f-sinf(a)*s)/length,dot=dx*slide.dx+dz*slide.dz;
   if(dot<-.5f){slideStop();return;}float blend=fminf(.06f,dt*.012f);slide.dx+=(dx-slide.dx)*blend;slide.dz+=(dz-slide.dz)*blend;float n=hypotf(slide.dx,slide.dz);if(n>.001f){slide.dx/=n;slide.dz/=n;}
  }
  float speed=slide.entry*(1-.70f*slide.elapsed/51.f);PI(184)=0;
  PF(424)=speed*(-sinf(a)*slide.dx+cosf(a)*slide.dz);PF(416)=speed*(-cosf(a)*slide.dx-sinf(a)*slide.dz);
 }
 slide.lastForward=PF(424);slide.lastSide=PF(416);slide.lastX=PF(1864);slide.lastZ=PF(1872);
}

static void testSlideInput(void){
 slideRequests=0;slideBHeld=0;slideButton(1,1);slideButton(1,1);if(slideRequests!=1)ExitProcess(3);slideRequests=0;slideButton(0,1);slideButton(1,0);if(slideRequests)ExitProcess(3);slideButton(1,1);if(slideRequests)ExitProcess(3);slideButton(0,1);slideButton(1,1);if(slideRequests!=1)ExitProcess(3);slideRequests=0;slideBHeld=0;
 if(logfile){fprintf(logfile,"PASS slide input: one B edge, held-button suppression, intro/menu press cannot queue for handoff\n");fflush(logfile);}
}
