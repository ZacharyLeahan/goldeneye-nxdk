/* Safe ledge traversal: native navigation remains the first choice. */
static int padTryUpper(JumpVec*,void**,int);
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
static int dropBodyClear(JumpVec*to){
 float dx=to->x-PF(1864),dz=to->z-PF(1872),len=hypotf(dx,dz);if(len<.001f)return 1;
 float sx=-dz/len*29.f,sz=dx/len*29.f;
 float head=fmaxf(40.f,PF(13940)+PF(188)+10.f);
 for(int side=-1;side<=1;side++)for(int level=0;level<=6;level++){
  float y=PF(140)+6.f+(head-6.f)*level/6.f;
  JumpVec a={PF(1864)+side*sx,y,PF(1872)+side*sz},b={to->x+side*sx,y,to->z+side*sz};
  if(!dropSegmentClear(a,b))return 0;
 }
 return 1;
}
static int dropTryStan(JumpVec*to,void**tile){
 int native=originalTryStan(to,tile);native=padTryUpper(to,tile,native);
 if(getenv("GE32_DROP_BASELINE")||NUMBER!=localIndex()||!jumpAllowed()||!PP(200))return native;
 if(drop.player!=CURRENT){memset(&drop,0,sizeof(drop));drop.player=CURRENT;}
 float dx=to->x-PF(1864),dz=to->z-PF(1872),len=hypotf(dx,dz),ground=PF(144);
 if(!isfinite(len)||len<.001f||len>128||!isfinite(ground))return native;
 if(native&&!drop.active)return native;
 if(drop.active&&!dropBodyClear(to))return 0;
 float lower;void*landing=floorBelow(to->x+dx/len*68.f,ground+1,to->z+dz/len*68.f,&lower);
 if(!landing||!isfinite(lower)||ground-lower<16||ground-lower>2048)return native;
 int landingRoom=FN(int(*)(void*),0x1000+883888)(landing),roomCount=*(int*)(base+0x228000+4208);
 if(landingRoom<0||landingRoom>=roomCount||roomCount>2048||!*(void**)(base+0x303000+27264+landingRoom*0x68+0x40))return native;
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
 if(!drop.active||drop.player!=CURRENT)return;
 if(!jumpAllowed()){drop.active=0;return;}
 float support=-INFINITY;
 static const float ring[9][2]={{0,0},{26,0},{-26,0},{0,26},{0,-26},{18,18},{18,-18},{-18,18},{-18,-18}};
 for(int i=0;i<9;i++){float y;void*t=floorBelow(PF(1864)+ring[i][0],PF(140)+2,PF(1872)+ring[i][1],&y);if(t&&isfinite(y))support=fmaxf(support,y);}
 if(!isfinite(support)){drop.active=0;return;}
 PF(144)=support;*useHeight=1;*offset=0;
 if(support<drop.originFloor-16)drop.falling=1;
}
static void dropAfterVertical(void){
 if(drop.active&&drop.falling&&PF(140)<=PF(144)+.2f&&PF(152)<=0){drop.active=drop.falling=0;drop.lands++;}
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
