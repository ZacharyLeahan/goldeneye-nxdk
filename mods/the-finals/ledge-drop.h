/* Safe ledge traversal: native navigation remains the first choice. */
static int padTryAirborne(JumpVec*,void**,int);
static int padAirborne(void);
static int (*originalTryStan)(JumpVec*,void**);
/* State is owned exclusively by the simulation thread. */
static struct {void*player;int active,falling,starts,lands,accepted;float originFloor;} drop;
static void*floorBelow(float x,float y,float z,float*height){JumpVec p={x,y,z};return FN(void*(*)(JumpVec*,void*,float*),0x1000+879552)(&p,NULL,height);}
/* Use movement bounds, never render-time ModelHitEntry chains, for actors. */
static int dropActorChecks,dropActorNoCache;
static int dropActorsClear(JumpVec a,JumpVec b,int*occupied){
 FN(void(*)(int*),0x1000+280624)(occupied);
 short*list=*(short**)(base+0x303000+186440);
 if(!list)return 0;
 for(int i=0;i<1024&&list[i]>=0;i++){
  int index=list[i];if(index>=1024)return 0;
  void*prop=(void*)(base+0x303000+128832+index*96);
  if(prop==PP(200)||(*(unsigned char*)prop!=3&&*(unsigned char*)prop!=6)||!*(void**)((unsigned char*)prop+8))continue;
  dropActorChecks++;void*chr=*(void**)((unsigned char*)prop+8);if(chr&&!*(void**)((unsigned char*)chr+40))dropActorNoCache++;
  float*polygon=NULL,top=0,bottom=0;int edges=0;
  FN(void(*)(void*,float**,int*,float*,float*),0x1000+276544)(prop,&polygon,&edges,&top,&bottom);
  if(!polygon||edges<=0)continue;if(edges>32)return 0;
  float lo[3]={INFINITY,bottom,INFINITY},hi[3]={-INFINITY,top,-INFINITY};
  for(int j=0;j<edges;j++){lo[0]=fminf(lo[0],polygon[j*2]);hi[0]=fmaxf(hi[0],polygon[j*2]);lo[2]=fminf(lo[2],polygon[j*2+1]);hi[2]=fmaxf(hi[2],polygon[j*2+1]);}
  float start[3]={a.x,a.y,a.z},delta[3]={b.x-a.x,b.y-a.y,b.z-a.z},enter=0,leave=1;
  for(int axis=0;axis<3;axis++){
   if(fabsf(delta[axis])<.00001f){if(start[axis]<lo[axis]||start[axis]>hi[axis]){enter=2;break;}}
   else{float t0=(lo[axis]-start[axis])/delta[axis],t1=(hi[axis]-start[axis])/delta[axis];enter=fmaxf(enter,fminf(t0,t1));leave=fminf(leave,fmaxf(t0,t1));}
  }
  if(enter<=leave)return 0;
 }
 return 1;
}
/* Background engine rays are infinite; accept hits only on this segment. */
static int dropSegmentClear(JumpVec from,JumpVec to){
 float dx=to.x-from.x,dy=to.y-from.y,dz=to.z-from.z,len2=dx*dx+dy*dy+dz*dz;
 if(len2<.0001f)return 1;
 if(!collisionPrepareBox((JumpVec){fminf(from.x,to.x),fminf(from.y,to.y),fminf(from.z,to.z)},(JumpVec){fmaxf(from.x,to.x),fmaxf(from.y,to.y),fmaxf(from.z,to.z)}))return 0;
 int total=*(int*)(base+0x228000+4208);float scale=*(float*)(base+0x228000+4252);
 if(total<1||total>2048||!isfinite(scale)||scale<=0)return 0;
 unsigned char*rooms=(void*)(base+0x303000+27264);
 for(int r=0;r<total;r++){
  if(!*(void**)(rooms+r*0x68+0x40))continue;
  JumpHit hit={0};
  if(FN(int(*)(JumpVec*,JumpVec*,int,JumpHit*),0x1000+13744)(&from,&to,r,&hit)){
   float t=((hit.position.x/scale-from.x)*dx+(hit.position.y/scale-from.y)*dy+(hit.position.z/scale-from.z)*dz)/len2;
   if(isfinite(t)&&t>.001f&&t<1.001f)return 0;
  }
 }
 int occupied[64];for(int i=0;i<64;i++)occupied[i]=-1;
 FN(void(*)(void*,int*),0x1000+275920)(PP(200),occupied);
 JumpVec pos,normal;
 if(!dropActorsClear(from,to,occupied))return 0;
 return !FN(int(*)(void*,JumpVec*,JumpVec*,unsigned,JumpVec*,JumpVec*,int*),0x1000+721344)(PP(200),&from,&to,0x13,&pos,&normal,occupied);
}
/* A circumscribed 16-point footprint covers front/back as well as sides.
 * Vertical destination probes catch a thin floor/ceiling lip between height samples. */
static int dropBodyClear(JumpVec*to){
 float len=hypotf(to->x-PF(1864),to->z-PF(1872));if(len<.001f)return 1;
 float foot=PF(140),head=fmaxf(40.f,PF(13940)+PF(188)+10.f);
 if(!collisionPrepareBox((JumpVec){fminf(PF(1864),to->x)-31,foot+.1f,fminf(PF(1872),to->z)-31},(JumpVec){fmaxf(PF(1864),to->x)+31,foot+head,fmaxf(PF(1872),to->z)+31}))return 0;
 for(int point=0;point<17;point++){
  float ox=0,oz=0;if(point){float a=(point-1)*.3926990817f;ox=31*cosf(a);oz=31*sinf(a);}
  for(int level=0;level<4;level++){
   float y=foot+.25f+(head-.25f)*level/3.f;JumpVec a={PF(1864)+ox,y,PF(1872)+oz},b={to->x+ox,y,to->z+oz};if(!dropSegmentClear(a,b))return 0;
  }
  JumpVec lo={to->x+ox,foot+.25f,to->z+oz},hi={lo.x,foot+head,lo.z};if(!dropSegmentClear(lo,hi))return 0;
 }
 return 1;
}
static float traversalFloorSupport(int*uniform){
 float support=-INFINITY,lowest=INFINITY;int present=0;
 for(int i=0;i<17;i++){float ox=0,oz=0;if(i){float angle=(i-1)*.3926990817f;ox=31*cosf(angle);oz=31*sinf(angle);}float y;void*t=floorBelow(PF(1864)+ox,PF(140)+1,PF(1872)+oz,&y);if(t&&isfinite(y)){support=fmaxf(support,y);lowest=fminf(lowest,y);present++;}}
 *uniform=present==17&&support-lowest<3;return support;
}
/* Hand grounded support back to native navigation, including connected slopes.
 * Distinct floor layers across an opening must not become slope support.
 * Wall-blocked toes reconcile native 30-unit and traversal 31-unit clearance. */
static void*dropGroundedAnchor(void){
 if(jumpActive||padAirborne()||fabsf(PF(152))>.6f)return NULL;
 float foot=PF(140),floor;void*tile=floorBelow(PF(1864),foot+1,PF(1872),&floor);if(!tile||!isfinite(floor)||foot<floor-1.5f||!FN(int(*)(void*,float,float),0xd5e10)(tile,PF(1864),PF(1872)))return NULL;
 if(!collisionPrepareBox((JumpVec){PF(1864)-31,foot+8,PF(1872)-31},(JumpVec){PF(1864)+31,foot+8,PF(1872)+31}))return NULL;
 float scale=*(float*)(base+0x228000+4252);int total=*(int*)(base+0x228000+4208);if(!isfinite(scale)||scale<=0||total<1||total>2048)return NULL;
 float highest=floor;
 for(int point=0;point<16;point++){float a=point*.3926990817f,dx=31*cosf(a),dz=31*sinf(a),h;void*t=floorBelow(PF(1864)+dx,foot+1,PF(1872)+dz,&h);
  if(t&&isfinite(h)){void*edge=tile;int connected=FN(int(*)(void**,float,float,float,float),0xd6870)(&edge,PF(1864),PF(1872),PF(1864)+dx,PF(1872)+dz);float nativeY=connected&&edge?FN(float(*)(void*,float,float),0x1000+879136)(edge,PF(1864)+dx,PF(1872)+dz):NAN;
   if(connected&&isfinite(nativeY)&&fabsf(nativeY-h)<3){highest=fmaxf(highest,h);continue;}
  }
  /* Unsupported toes are acceptable only where a real static wall prevents
   * the native body from going; open ledges retain the traversal context. */
  JumpVec from={PF(1864),foot+8,PF(1872)},to={from.x+dx,from.y,from.z+dz};int wall=0;
  for(int room=1;room<total;room++){if(!*(void**)(base+0x303000+27264+room*0x68+0x40))continue;JumpHit hit={0};if(FN(int(*)(JumpVec*,JumpVec*,int,JumpHit*),0x1000+13744)(&from,&to,room,&hit)){float u=((hit.position.x/scale-from.x)*dx+(hit.position.z/scale-from.z)*dz)/(31*31);if(u>.001f&&u<1.001f){wall=1;break;}}}
  if(!wall)return NULL;
 }
 /* Sloped support may hold the feet at the uphill toe until this handoff.
  * Never release a body still suspended above all connected support. */
 if(foot>highest+1.5f)return NULL;return tile;
}
static int dropTryStan(JumpVec*to,void**tile){
 /* The mantle advances its own swept path. Native run momentum must not
  * push the body under the lip between two climb steps. */
 if(mantleBusy&&NUMBER==localIndex()&&CURRENT==PLAYERS[NUMBER])return 0;
 /* Backing out of an opening can leave a lower STAN tile under feet that
  * are still supported upstairs. Restore the actual supporting layer before
  * asking native collision, so it checks the upstairs walls and portals. */
 if(NUMBER==localIndex()&&CURRENT==drop.player&&drop.active&&jumpAllowed()&&PP(1856)){
  float support;void*t=floorBelow(PF(1864),PF(140)+1,PF(1872),&support);
  float old=FN(float(*)(void*,float,float),0x1000+879136)(PP(1856),PF(1864),PF(1872));
  if(t&&isfinite(support)&&isfinite(old)&&support>old+16&&fabsf(PF(140)-support)<1.5f&&fabsf(PF(152))<.6f)PP(1856)=t;
 }
 if(NUMBER==localIndex()&&CURRENT==drop.player&&drop.active&&jumpAllowed()){void*grounded=dropGroundedAnchor();if(grounded){PP(1856)=grounded;drop.active=drop.falling=0;drop.lands++;if(logfile){fprintf(logfile,"traversal ended on supported ground beside wall\n");fflush(logfile);}}}
 int native=originalTryStan(to,tile);native=padTryAirborne(to,tile,native);
 if(getenv("GE32_DROP_BASELINE")||NUMBER!=localIndex()||!jumpAllowed()||!PP(200))return native;
 if(drop.player!=CURRENT){memset(&drop,0,sizeof(drop));drop.player=CURRENT;}
 float dx=to->x-PF(1864),dz=to->z-PF(1872),len=hypotf(dx,dz),ground=PF(144);
 if(!isfinite(len)||len<.001f||len>128||!isfinite(ground))return native;
 if(native&&!drop.active)return native;
 if(drop.active&&!dropBodyClear(to))return 0;
 float lower;void*landing=floorBelow(to->x+dx/len*68.f,ground+1,to->z+dz/len*68.f,&lower);
 if(!landing||!isfinite(lower)||ground-lower<16||ground-lower>2048)return native;
 int landingRoom=FN(int(*)(void*),0x1000+883888)(landing),roomCount=*(int*)(base+0x228000+4208);
 if(landingRoom<0||landingRoom>=roomCount||roomCount>2048||!jumpRoomReady(landingRoom))return native;
 static const float ring[9][2]={{0,0},{26,0},{-26,0},{0,26},{0,-26},{18,18},{18,-18},{-18,18},{-18,-18}};
 for(int i=0;i<9;i++){
  float y;void*t=floorBelow(to->x+dx/len*68.f+ring[i][0],lower+8,to->z+dz/len*68.f+ring[i][1],&y);
  if(!t||!isfinite(y)||fabsf(y-lower)>8)return native;
 }
 JumpVec downFrom={to->x+dx/len*68.f,ground+6,to->z+dz/len*68.f},downTo={downFrom.x,lower+3,downFrom.z};
 if(!dropSegmentClear(downFrom,downTo)||!dropBodyClear(to))return native;
 float y;void*t=floorBelow(to->x,PF(140)+2,to->z,&y);if(!t||!isfinite(y))return native;
 if(!drop.active){drop.active=1;drop.originFloor=ground;drop.starts++;}
 *tile=t;drop.accepted++;FN(void(*)(void),0x1000+876000)();return 1;
}
/* Retain ledge support while any sampled foot point is still on the deck.
 * Once clear, native gravity and landing clamps perform the descent. */
static void dropPrepareVertical(int*useHeight,float*offset){
 if(drop.player!=CURRENT){memset(&drop,0,sizeof(drop));drop.player=CURRENT;}
 /* Native horizontal navigation can switch layers during an added jump.
  * Preserve the full-body traversal context until a stable landing, even if
  * the native jump flag clears while only part of the body is supported. */
 if(!drop.active&&jumpActive&&jumpAllowed()){drop.active=1;drop.originFloor=jumpOriginFloor;drop.falling=0;}
 if(!drop.active)return;
 if(!jumpAllowed()){drop.active=0;return;}
 void*grounded=dropGroundedAnchor();if(grounded){PP(1856)=grounded;drop.active=drop.falling=0;drop.lands++;return;}
 int backOnOrigin;float support=traversalFloorSupport(&backOnOrigin);
 /* A toe on the uphill side may be above the rising player's feet.
  * Giving that height to native gravity pins the player to the slope
  * with positive velocity. Use the floor below the body for this ascent. */
 if(jumpActive&&!padAirborne()&&PF(152)>0&&support>=PF(140)-.01f){
  float centerFloor;
  if(floorBelow(PF(1864),PF(140)-.01f,PF(1872),&centerFloor)&&isfinite(centerFloor)&&centerFloor<PF(140))
   support=centerFloor;
 }
 if(backOnOrigin&&fabsf(PF(140)-support)<1.5f&&fabsf(PF(152))<.6f){float h;void*t=floorBelow(PF(1864),PF(140)+1,PF(1872),&h);if(t&&fabsf(h-support)<3)PP(1856)=t;drop.active=drop.falling=0;return;}
 if(!isfinite(support)){drop.active=0;return;}
 PF(144)=support;*useHeight=1;*offset=0;
 if(support<drop.originFloor-16)drop.falling=1;
}
static void dropAfterVertical(void){
 if(drop.active&&PF(140)<=PF(144)+.2f&&PF(152)<=0){
  int uniform;float support=traversalFloorSupport(&uniform);
  if(uniform&&fabsf(PF(140)-support)<.5f){float y;void*t=floorBelow(PF(1864),PF(140)+1,PF(1872),&y);if(t&&fabsf(y-support)<3)PP(1856)=t;drop.active=drop.falling=0;drop.lands++;}
 }
}

static int installDropHook(void){
 unsigned char expected[]={0x55,0x41,0x57,0x41,0x56,0x41,0x54,0x57,0x56,0x53,0x48,0x81,0xec,0xa0,0,0,0};
 unsigned char*target=(void*)(base+0x11f30);if(memcmp(target,expected,17))return 0;
 unsigned char*t=VirtualAlloc(NULL,64,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!t)return 0;
 memcpy(t,target,17);t[17]=0x48;t[18]=0xb8;*(uintptr_t*)(t+19)=(uintptr_t)(target+17);t[27]=0xff;t[28]=0xe0;
 DWORD prot;VirtualProtect(t,64,PAGE_EXECUTE_READ,&prot);originalTryStan=(void*)t;
 unsigned char j[17]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0,0x90,0x90,0x90,0x90,0x90};*(uintptr_t*)(j+2)=(uintptr_t)dropTryStan;
 VirtualProtect(target,17,PAGE_EXECUTE_READWRITE,&prot);memcpy(target,j,17);VirtualProtect(target,17,prot,&prot);FlushInstructionCache(GetCurrentProcess(),target,17);return 1;
}
