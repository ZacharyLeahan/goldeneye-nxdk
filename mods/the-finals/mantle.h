/* GE32 local simulation. Times are native 60 Hz units, distances are world units. */
#define MANTLE_BUFFER 18.f
#define MANTLE_EARLIEST_GRAB 8.f
#define MANTLE_REACH 62.f
/* Measured from airborne feet. The standing eye is about 167 units high. */
#define MANTLE_HEIGHT 235.f
#define MANTLE_DURATION 32.f
#define MANTLE_LOW_STEP 65.f
#define MANTLE_LOW_DURATION 18.f
#define MANTLE_RADIUS 30.f
#define MANTLE_LIP_ADJUST 24.f
#define MANTLE_BARRIER_SPAN 128.f
#define MANTLE_BARRIER_DROP 175.f

static struct {
 void *owner;
 int stage,clock,press,armed,active,found,clear,chose,forward,airPress;
 float age,elapsed,entrySpeed;
 JumpVec start,liftStart,land;
 float adjustTime,barrierTop,duration;
 int crossingBarrier;
} mantle;

static volatile LONG mantleDebugEnabled;
static SRWLOCK mantleDebugLock=SRWLOCK_INIT;
static char mantleDebugLine[160];
/* Only transition events are flushed so a sudden exit preserves the last action. */
static void mantleTrace(const char *event){
 if(!logfile||!getenv("GE32_MANTLE_TRACE"))return;
 fprintf(logfile,"mantle %s stage=%d tick=%d press=%d pos=(%.2f %.2f %.2f) yaw=%.2f ledge=%d clear=%d\n",
  event,mantle.stage,mantle.clock,mantle.press,PF(1864),PF(140),PF(1872),PF(380),mantle.found,mantle.clear);
 fflush(logfile);
}
static void mantlePress(int tick){
 mantle.press=tick>=0&&tick<=mantle.clock?tick:mantle.clock;
 mantle.age=0;
 mantle.armed=1;
 mantle.airPress=jumpActive;
 mantle.chose=mantle.found=mantle.clear=0;
 mantleTrace("press");
}
static JumpVec mantleFeet(void){return (JumpVec){PF(1864),PF(140),PF(1872)};}
static int mantleSupport(JumpVec at,void **tile){
 for(int i=0;i<=12;i++){
  float angle=i*.52359878f;
  float x=at.x+(i?MANTLE_RADIUS*cosf(angle):0),z=at.z+(i?MANTLE_RADIUS*sinf(angle):0),h;
  void *t=floorBelow(x,at.y+5,z,&h);
  if(!t||!isfinite(h)||fabsf(h-at.y)>3||
     !FN(int(*)(void*,float,float),0xd5e10)(t,x,z))return 0;
  if(!i)*tile=t;
 }
 return 1;
}
/* Native stairs can already intersect the uphill toe of a standing player.
 * Clip a blocked foot ray to that connected tread while lifting away.
 * The head, sides, disconnected ledges and overhead faces still block. */
static int mantleTread(JumpVec *point){
 float x=PF(1864),z=PF(1872);
 if(hypotf(point->x-x,point->z-z)>MANTLE_RADIUS+MANTLE_LIP_ADJUST+1)return 0;
 void *tile=PP(1856);
 if(!tile||!FN(int(*)(void**,float,float,float,float),0xd6870)(&tile,x,z,point->x,point->z)||!tile)return 0;
 float top=FN(float(*)(void*,float,float),0x1000+879136)(tile,point->x,point->z);
 if(!isfinite(top)||top>PF(140)+32||top<point->y-.5f||top-point->y>32)return 0;
 point->y=top+1;
 return 1;
}
/* Sweep the body along its path and check vertical clearance at the destination. */
static int mantleSweep(JumpVec from,JumpVec to){
 float head=fmaxf(120,PF(13940))+14;
 for(int i=0;i<=12;i++){
  float angle=i*.52359878f,x=i?MANTLE_RADIUS*cosf(angle):0,z=i?MANTLE_RADIUS*sinf(angle):0;
  JumpVec lo={to.x+x,to.y+1,to.z+z},hi={lo.x,to.y+head,lo.z};
  if(!dropSegmentClear(lo,hi)&&(!mantleTread(&lo)||!dropSegmentClear(lo,hi)))return 0;
  for(int j=0;j<4;j++){
   float height=1+(head-1)*j/3;
   JumpVec a={from.x+x,from.y+height,from.z+z},b={to.x+x,to.y+height,to.z+z};
   if(!dropSegmentClear(a,b)){
    if(j!=0||!mantleTread(&a))return 0;
    mantleTread(&b);
    if(!dropSegmentClear(a,b))return 0;
   }
  }
 }
 return 1;
}
static float mantleEase(float t){t=fminf(1,fmaxf(0,t));return t*t*(3-2*t);}
static JumpVec mantlePoint(float t){
 if(mantle.adjustTime>0){
  if(t<mantle.adjustTime){
   float slide=mantleEase(t/mantle.adjustTime);
   return (JumpVec){mantle.start.x+(mantle.liftStart.x-mantle.start.x)*slide,mantle.start.y,
    mantle.start.z+(mantle.liftStart.z-mantle.start.z)*slide};
  }
  t=(t-mantle.adjustTime)/(1-mantle.adjustTime);
 }
 if(mantle.crossingBarrier){
  float lift=mantleEase(t/.38f),over=mantleEase((t-.38f)/.39f),down=mantleEase((t-.77f)/.23f);
  return (JumpVec){mantle.liftStart.x+(mantle.land.x-mantle.liftStart.x)*over,
   mantle.start.y+(mantle.barrierTop-mantle.start.y)*lift+(mantle.land.y-mantle.barrierTop)*down,
   mantle.liftStart.z+(mantle.land.z-mantle.liftStart.z)*over};
 }
 float lift=fminf(1,t/.53f);
 float rise=mantle.land.y-mantle.start.y+2;
 float tangent=fminf(2.f,mantle.entrySpeed*(mantle.duration*.53f)/rise);
 float up=mantleEase(lift)+tangent*lift*(1-lift)*(1-lift),over=mantleEase((t-.53f)/.47f);
 return (JumpVec){mantle.liftStart.x+(mantle.land.x-mantle.liftStart.x)*over,
 mantle.start.y+(mantle.land.y-mantle.start.y+2)*up-2*over,
 mantle.liftStart.z+(mantle.land.z-mantle.liftStart.z)*over};
}
/* A tangential pass can hit both feelers on a long wall. Check the actual
 * triangle facing before allowing the climb to take over the jump. */
static int mantleFacesPlayer(JumpVec a,JumpVec b){
 float dx=b.x-a.x,dz=b.z-a.z,length=hypotf(dx,dz);
 float scale=*(float*)(base+0x228000+4252),nearest=1.01f,alignment=0;
 int total=*(int*)(base+0x228000+4208);
 for(int r=1;r<total;r++){
  if(!*(void**)(base+0x303000+27264+r*0x68+0x40))continue;
  JumpHit hit={0};
  if(!FN(int(*)(JumpVec*,JumpVec*,int,JumpHit*),0x1000+13744)(&a,&b,r,&hit))continue;
  float distance=((hit.position.x/scale-a.x)*dx+(hit.position.z/scale-a.z)*dz)/(length*length);
  float n=hypotf(hit.normal.x,hit.normal.z);
  if(distance>0&&distance<nearest&&n>.0001f){
   nearest=distance;alignment=fabsf(hit.normal.x*dx+hit.normal.z*dz)/(n*length);
  }
 }
 return alignment>.45f;
}
/* A walkway made from a single horizontal polygon has no wall for the
 * chest rays to hit. Find the edge of its walkable surface at both hands,
 * then verify that the collision mesh actually ends at that edge. */
static int mantleTopAt(float x,float z,float h){
 float floor;
 void *tile=floorBelow(x,h+4,z,&floor);
 return tile&&fabsf(floor-h)<3&&FN(int(*)(void*,float,float),0xd5e10)(tile,x,z);
}
static int mantleOpenRim(JumpVec p,float dx,float dz,float distance,float h){
 float edges[2];
 for(int hand=0;hand<2;hand++){
  float side=hand?14.f:-14.f;
  float x=p.x+dz*side,z=p.z-dx*side;
  float lo=-MANTLE_RADIUS,hi=distance+4;
  if(mantleTopAt(x+dx*lo,z+dz*lo,h)||!mantleTopAt(x+dx*hi,z+dz*hi,h))return 0;
  for(int i=0;i<7;i++){
   float mid=(lo+hi)*.5f;
   if(mantleTopAt(x+dx*mid,z+dz*mid,h))hi=mid;else lo=mid;
  }
  float surface;
  if(backgroundFloorBetween(x+dx*(lo-3),z+dz*(lo-3),h-3,h+3,&surface)||
     !backgroundFloorBetween(x+dx*(hi+3),z+dz*(hi+3),h-3,h+3,&surface))return 0;
  edges[hand]=hi;
 }
 return fabsf(edges[0]-edges[1])<=32.f;
}
static int mantleFindPlatform(int early){
 JumpVec p=mantleFeet();float yaw=PF(380)*.01745329252f,dx=-sinf(yaw),dz=cosf(yaw);
 /* A rising jump can reach a close overhang before forward motion carries the head underneath. */
 float up=fmaxf(0,PF(152));
 float apex=p.y+fminf(61.f,up*up/(2.f*.27777779f));
 for(float d=MANTLE_RADIUS;d<=MANTLE_REACH;d+=4){
  float h,x=p.x+dx*d,z=p.z+dz*d;
  int closeOverhang=d<=MANTLE_RADIUS+12&&jumpActive&&up>0;
  float upper=(closeOverhang?apex:p.y)+MANTLE_HEIGHT;
  if(!floorBelow(x,upper,z,&h)||!isfinite(h)||h<p.y+4||h>upper)continue;
  if(early&&!closeOverhang&&h>jumpOriginFloor+MANTLE_LOW_STEP)continue;
  /* Require a face on both sides of the approach, so grazing a corner or
   * jumping toward a ramp does not redirect the player. */
  int face=1;
  for(int side=-1;side<=1;side+=2){
   JumpVec a={p.x+side*dz*14,h-6,p.z-side*dx*14};
   JumpVec b={x+dx*3+side*dz*14,h-6,z+dz*3-side*dx*14};
   if(dropSegmentClear(a,b)){face=0;break;}
  }
  int openRim=!face||!mantleFacesPlayer((JumpVec){p.x,h-6,p.z},(JumpVec){x+dx*4,h-6,z+dz*4});
  if(openRim&&!mantleOpenRim(p,dx,dz,d,h))continue;
  mantle.found=1;mantle.start=p;mantle.entrySpeed=fmaxf(0,PF(152));
  void *tile=NULL;
  for(float inset=MANTLE_RADIUS+3;inset<=2*MANTLE_RADIUS+7;inset+=8){
   mantle.land=(JumpVec){x+dx*inset,h,z+dz*inset};
   if(mantleSupport(mantle.land,&tile))break;
   tile=NULL;
  }
  if(!tile)continue;
  /* Late presses can catch an overhang with the front of the body already
   * beneath it. Sweep a short retreat before lifting around the lip. */
  float retreat=MANTLE_LIP_ADJUST+(openRim?MANTLE_RADIUS:0);
  for(float back=0;back<=retreat;back+=8){
   JumpVec liftStart={p.x-dx*back,p.y,p.z-dz*back};
   JumpVec crest={liftStart.x,h+2,liftStart.z};
   if(back&&!mantleSweep(p,liftStart))break;
   if(!mantleSweep(liftStart,crest)||!mantleSweep(crest,mantle.land))continue;
   mantle.liftStart=liftStart;mantle.adjustTime=back/(5.f*MANTLE_DURATION);
   if(back)mantle.entrySpeed=0;
   mantle.clear=1;return 1;
  }
  return 0;
 }
 return 0;
}

/* Some railings and dividers have a solid mesh cap but no walkable tile on
 * top. Cross only when the entire body can settle on a native tile beyond. */
static int mantleFindBarrier(void){
 JumpVec start=mantleFeet();
 float yaw=PF(380)*.01745329252f,dx=-sinf(yaw),dz=cosf(yaw);
 float height=0,edge;
 for(edge=MANTLE_RADIUS;edge<=MANTLE_REACH;edge+=4){
  float x=start.x+edge*dx,z=start.z+edge*dz,nav;
  if(!backgroundFloorBetween(x,z,start.y+5,start.y+MANTLE_HEIGHT,&height)||height<start.y+10)continue;
  if(floorBelow(x,height+4,z,&nav)&&fabsf(nav-height)<4)continue;
  JumpVec from={start.x,height-5,start.z},to={x+dx*4,height-5,z+dz*4};
  if(!mantleFacesPlayer(from,to))continue;
  break;
 }
 if(edge>MANTLE_REACH)return 0;
 mantle.found=1;
 JumpVec lift=start,crest=start;
 crest.y=height+3;
 float back=0;
 while(!mantleSweep(lift,crest)){
  back+=6;
  if(back>MANTLE_LIP_ADJUST)return 0;
  lift.x=crest.x=start.x-back*dx;lift.z=crest.z=start.z-back*dz;
  if(!mantleSweep(start,lift))return 0;
 }
 for(float depth=MANTLE_RADIUS+4;depth<=MANTLE_BARRIER_SPAN+MANTLE_RADIUS;depth+=8){
  JumpVec land={start.x+(edge+depth)*dx,0,start.z+(edge+depth)*dz};
  float mesh;void *tile=NULL;
  if(!floorBelow(land.x,height+4,land.z,&land.y)||!isfinite(land.y)||land.y>height+1||
     height-land.y>MANTLE_BARRIER_DROP||!backgroundFloorBetween(land.x,land.z,land.y-4,height+4,&mesh)||
     fabsf(mesh-land.y)>3||!mantleSupport(land,&tile))continue;
  JumpVec above={land.x,crest.y,land.z};
  if(!mantleSweep(above,land))continue;
  if(!mantleSweep(crest,above))return 0;
  mantle.start=start;mantle.liftStart=lift;mantle.land=land;mantle.barrierTop=crest.y;
  mantle.crossingBarrier=1;mantle.adjustTime=back/(5.f*MANTLE_DURATION);mantle.clear=1;
  return 1;
 }
 return 0;
}
static int mantleFind(int early){
 mantle.crossingBarrier=0;
 return mantleFindPlatform(early)||(!early&&mantleFindBarrier());
}
static void mantleStop(void){
 mantle.armed=mantle.active=0;InterlockedExchange(&mantleBusy,0);
}
static void mantleAbort(void){
 if(mantle.active){
  mantleTrace("abort");
  float floor;void *tile=floorBelow(PF(1864),PF(140)+1,PF(1872),&floor);
  if(tile&&isfinite(floor)){PP(1856)=tile;PF(144)=floor;}
  jumpActive=1;drop.active=1;drop.originFloor=jumpOriginFloor;
 }
 mantleStop();
}
static void mantleContext(void){
 int stage=*(int*)(base+0x303000+211452),clock=*(int*)(base+0x303000+211432);
 if(mantle.owner!=CURRENT||mantle.stage!=stage||clock<mantle.clock){
  memset(&mantle,0,sizeof(mantle));mantle.owner=CURRENT;mantle.stage=stage;
  mantle.press=-1;mantleStop();
 }
 mantle.clock=clock;
}
static int mantleVertical(int *useHeight,float *offset){
 if(mantle.press>=0)mantle.age=(float)(mantle.clock-mantle.press);
 if(!mantle.armed&&!mantle.active)return 0;
 float dt=*(float*)(base+0x303000+211436);
 if(!jumpAllowed()||!PP(200)){mantleStop();return 0;}
 if(!isfinite(dt)||dt<=0)return mantle.active;
 if(openWheel||gadget.flight||slap.phase||padCarrying){
  mantleAbort();return 0;
 }
 if(mantle.armed){
  /* Keep the press through native stair smoothing. Recheck support and headroom
   * every tick before starting a grounded crossing. */
  int atWall=!jumpActive&&jumpStandingOnFloor(PF(140),PF(144),PF(152));
  if(mantle.age>MANTLE_BUFFER)mantle.armed=0;
  /* A low step can be below the feet by the ordinary grab tick. */
  else if(mantle.forward&&(atWall?mantleFindBarrier():jumpActive&&
    mantleFind(mantle.clock-jumpTakeoffTick<MANTLE_EARLIEST_GRAB))){
   if(atWall)jumpOriginFloor=PF(144);
   mantle.duration=!mantle.crossingBarrier&&mantle.land.y-jumpOriginFloor<=MANTLE_LOW_STEP?MANTLE_LOW_DURATION:MANTLE_DURATION;
   mantle.chose=mantle.active=1;mantle.armed=0;mantle.elapsed=0;
   InterlockedExchange(&mantleBusy,1);drop.active=jumpActive=0;
   mantleTrace("grab");
  }
 }
 if(!mantle.active)return 0;
 JumpVec from=mantleFeet();
 mantle.elapsed=fminf(mantle.duration,mantle.elapsed+dt);
 JumpVec next=mantlePoint(mantle.elapsed/mantle.duration);
 void *landing=NULL;
 if(!mantleSupport(mantle.land,&landing)||!mantleSweep(from,next)){
  mantle.clear=0;mantleAbort();
  return 0;
 }
 PF(1864)=next.x;PF(1872)=next.z;PF(140)=PF(144)=next.y;PF(152)=0;
 float h;void *under=floorBelow(next.x,next.y+3,next.z,&h);
 if(under)PP(1856)=under;
 *useHeight=1;*offset=0;
 if(mantle.elapsed>=mantle.duration){PP(1856)=landing;mantleTrace("land");mantleStop();}
 return 1;
}
static void mantlePublish(void){
 if(!mantleDebugEnabled&&!getenv("GE32_MANTLE_TEST"))return;
 AcquireSRWLockExclusive(&mantleDebugLock);
 snprintf(mantleDebugLine,sizeof(mantleDebugLine),"PRESS %d   AGE %.0f MS   LEDGE %s   CLEAR %s   %s",
 mantle.press,mantle.age*(1000.f/60.f),mantle.found?"YES":"NO",
 mantle.found?(mantle.clear?"YES":"NO"):"NA",mantle.press<0?"READY":mantle.chose?"MANTLE":"JUMP");
 ReleaseSRWLockExclusive(&mantleDebugLock);
}
