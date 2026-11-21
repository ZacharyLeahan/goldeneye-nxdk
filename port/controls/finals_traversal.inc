static void mantlePress(int tick){
 traversal->mantle.press=tick>=0&&tick<=traversal->mantle.clock?tick:traversal->mantle.clock;
 traversal->mantle.age=0;
 traversal->mantle.armed=1;
 traversal->mantle.airPress=jumpActive;
 traversal->mantle.chose=traversal->mantle.found=traversal->mantle.clear=0;
 mantleTrace("press");
}
static JumpVec mantleFeet(void){return (JumpVec){g_CurrentPlayer->field_488.collision_position.x,g_CurrentPlayer->field_70,g_CurrentPlayer->field_488.collision_position.z};}
static int mantleSupport(JumpVec at,StandTile **tile){
 for(int i=0;i<=12;i++){
  float angle=i*.52359878f;
  float x=at.x+(i?MANTLE_RADIUS*cosf(angle):0),z=at.z+(i?MANTLE_RADIUS*sinf(angle):0),h;
  void *t=floorBelow(x,at.y+5,z,&h);
  if(!t||!isfinite(h)||fabsf(h-at.y)>3||
     !stanTestPointWithinTileBoundsMaybe(t,x,z))return 0;
  if(!i)*tile=t;
 }
 return 1;
}
/* Native stairs can already intersect the uphill toe of a standing player.
 * Clip a blocked foot ray to that connected tread while lifting away.
 * The head, sides, disconnected ledges and overhead faces still block. */
static int mantleTread(JumpVec *point){
 float x=g_CurrentPlayer->field_488.collision_position.x,z=g_CurrentPlayer->field_488.collision_position.z;
 if(hypotf(point->x-x,point->z-z)>MANTLE_RADIUS+MANTLE_LIP_ADJUST+1)return 0;
 StandTile *tile=g_CurrentPlayer->field_488.current_tile_ptr;
 if(!tile||!walkTilesBetweenPoints_NoCallback(&tile,x,z,point->x,point->z)||!tile)return 0;
 float top=stanGetPositionYValue(tile,point->x,point->z);
 if(!isfinite(top)||top>g_CurrentPlayer->field_70+32||top<point->y-.5f||top-point->y>32)return 0;
 point->y=top+1;
 return 1;
}
/* Sweep the body along its path and check vertical clearance at the destination. */
static int mantleSweep(JumpVec from,JumpVec to){
 float head=fmaxf(120,g_CurrentPlayer->standheight)+14;
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
 if(traversal->mantle.adjustTime>0){
  if(t<traversal->mantle.adjustTime){
   float slide=mantleEase(t/traversal->mantle.adjustTime);
   return (JumpVec){traversal->mantle.start.x+(traversal->mantle.liftStart.x-traversal->mantle.start.x)*slide,traversal->mantle.start.y,
    traversal->mantle.start.z+(traversal->mantle.liftStart.z-traversal->mantle.start.z)*slide};
  }
  t=(t-traversal->mantle.adjustTime)/(1-traversal->mantle.adjustTime);
 }
 if(traversal->mantle.crossingBarrier){
  float lift=mantleEase(t/.38f),over=mantleEase((t-.38f)/.39f),down=mantleEase((t-.77f)/.23f);
  return (JumpVec){traversal->mantle.liftStart.x+(traversal->mantle.land.x-traversal->mantle.liftStart.x)*over,
   traversal->mantle.start.y+(traversal->mantle.barrierTop-traversal->mantle.start.y)*lift+(traversal->mantle.land.y-traversal->mantle.barrierTop)*down,
   traversal->mantle.liftStart.z+(traversal->mantle.land.z-traversal->mantle.liftStart.z)*over};
 }
 float lift=fminf(1,t/.53f);
 float rise=traversal->mantle.land.y-traversal->mantle.start.y+2;
 float tangent=fminf(2.f,traversal->mantle.entrySpeed*(traversal->mantle.duration*.53f)/rise);
 float up=mantleEase(lift)+tangent*lift*(1-lift)*(1-lift),over=mantleEase((t-.53f)/.47f);
 return (JumpVec){traversal->mantle.liftStart.x+(traversal->mantle.land.x-traversal->mantle.liftStart.x)*over,
 traversal->mantle.start.y+(traversal->mantle.land.y-traversal->mantle.start.y+2)*up-2*over,
 traversal->mantle.liftStart.z+(traversal->mantle.land.z-traversal->mantle.liftStart.z)*over};
}
/* A tangential pass can hit both feelers on a long wall. Check the actual
 * triangle facing before allowing the climb to take over the jump. */
static int mantleTopAt(float x,float z,float h){
 float floor;
 StandTile *tile=floorBelow(x,h+4,z,&floor);
 return tile&&fabsf(floor-h)<3&&stanTestPointWithinTileBoundsMaybe(tile,x,z);
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
 JumpVec p=mantleFeet();float yaw=g_CurrentPlayer->vv_theta*.01745329252f,dx=-sinf(yaw),dz=cosf(yaw);
 /* A rising jump can reach a close overhang before forward motion carries the head underneath. */
 float up=fmaxf(0,g_CurrentPlayer->field_7C);
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
  traversal->mantle.found=1;traversal->mantle.start=p;traversal->mantle.entrySpeed=fmaxf(0,g_CurrentPlayer->field_7C);
  StandTile *tile=NULL;
  for(float inset=MANTLE_RADIUS+3;inset<=2*MANTLE_RADIUS+7;inset+=8){
   traversal->mantle.land=(JumpVec){x+dx*inset,h,z+dz*inset};
   if(mantleSupport(traversal->mantle.land,&tile))break;
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
   if(!mantleSweep(liftStart,crest)||!mantleSweep(crest,traversal->mantle.land))continue;
   traversal->mantle.liftStart=liftStart;traversal->mantle.adjustTime=back/(5.f*MANTLE_DURATION);
   if(back)traversal->mantle.entrySpeed=0;
   traversal->mantle.clear=1;return 1;
  }
  return 0;
 }
 return 0;
}

/* Some railings and dividers have a solid mesh cap but no walkable tile on
 * top. Cross only when the entire body can settle on a native tile beyond. */
static int mantleFindBarrier(void){
 JumpVec start=mantleFeet();
 float yaw=g_CurrentPlayer->vv_theta*.01745329252f,dx=-sinf(yaw),dz=cosf(yaw);
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
 traversal->mantle.found=1;
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
  float mesh;StandTile *tile=NULL;
  if(!floorBelow(land.x,height+4,land.z,&land.y)||!isfinite(land.y)||land.y>height+1||
     height-land.y>MANTLE_BARRIER_DROP||!backgroundFloorBetween(land.x,land.z,land.y-4,height+4,&mesh)||
     fabsf(mesh-land.y)>3||!mantleSupport(land,&tile))continue;
  JumpVec above={land.x,crest.y,land.z};
  if(!mantleSweep(above,land))continue;
  if(!mantleSweep(crest,above))return 0;
  traversal->mantle.start=start;traversal->mantle.liftStart=lift;traversal->mantle.land=land;traversal->mantle.barrierTop=crest.y;
  traversal->mantle.crossingBarrier=1;traversal->mantle.adjustTime=back/(5.f*MANTLE_DURATION);traversal->mantle.clear=1;
  return 1;
 }
 return 0;
}
static int mantleFind(int early){
 traversal->mantle.crossingBarrier=0;
 return mantleFindPlatform(early)||(!early&&mantleFindBarrier());
}
static int mantleVertical(int *useHeight,float *offset){
 if(traversal->mantle.press>=0)traversal->mantle.age=(float)(traversal->mantle.clock-traversal->mantle.press);
 if(!traversal->mantle.armed&&!traversal->mantle.active)return 0;
 float dt=g_GlobalTimerDelta;
 if(!traversal->allowed||!g_CurrentPlayer->prop){mantleStop();return 0;}
 if(!isfinite(dt)||dt<=0)return traversal->mantle.active;
 if(traversal->blocked){
  mantleAbort();return 0;
 }
 if(traversal->mantle.armed){
  /* Keep the press through native stair smoothing. Recheck support and headroom
   * every tick before starting a grounded crossing. */
  int atWall=!jumpActive&&xboxGrounded();
  if(traversal->mantle.age>MANTLE_BUFFER)traversal->mantle.armed=0;
  /* A low step can be below the feet by the ordinary grab tick. */
  else if(traversal->mantle.forward&&(atWall?mantleFindBarrier():jumpActive&&
    mantleFind(traversal->mantle.clock-jumpTakeoffTick<MANTLE_EARLIEST_GRAB))){
   if(atWall)jumpOriginFloor=g_CurrentPlayer->stanHeight;
   traversal->mantle.duration=!traversal->mantle.crossingBarrier&&traversal->mantle.land.y-jumpOriginFloor<=MANTLE_LOW_STEP?MANTLE_LOW_DURATION:MANTLE_DURATION;
   traversal->mantle.chose=traversal->mantle.active=1;traversal->mantle.armed=0;traversal->mantle.elapsed=0;
   jumpActive=0;
   mantleTrace("grab");
  }
 }
 if(!traversal->mantle.active)return 0;
 JumpVec from=mantleFeet();
 traversal->mantle.elapsed=fminf(traversal->mantle.duration,traversal->mantle.elapsed+dt);
 JumpVec next=mantlePoint(traversal->mantle.elapsed/traversal->mantle.duration);
 StandTile *landing=NULL;
 if(!mantleSupport(traversal->mantle.land,&landing)||!mantleSweep(from,next)){
  traversal->mantle.clear=0;mantleAbort();
  return 0;
 }
 g_CurrentPlayer->field_488.collision_position.x=next.x;g_CurrentPlayer->field_488.collision_position.z=next.z;g_CurrentPlayer->field_70=g_CurrentPlayer->stanHeight=next.y;g_CurrentPlayer->field_7C=0;
 float h;void *under=floorBelow(next.x,next.y+3,next.z,&h);
 if(under)g_CurrentPlayer->field_488.current_tile_ptr=under;
 *useHeight=1;*offset=0;
 if(traversal->mantle.elapsed>=traversal->mantle.duration){g_CurrentPlayer->field_488.current_tile_ptr=landing;mantleTrace("land");mantleStop();}
 return 1;
}
static int slideGrounded(void){return fabsf(g_CurrentPlayer->field_70-g_CurrentPlayer->stanHeight)<1.5f&&fabsf(g_CurrentPlayer->field_7C)<.6f;}
static void slideStop(void){traversal->slide.active=traversal->slide.ready=0;traversal->slide.cooldown=18;}
static void slideBefore(int request){
 float dt=g_GlobalTimerDelta;
traversal->slide.cooldown=fmaxf(0,traversal->slide.cooldown-dt);
 if(!traversal->allowed||traversal->blocked){slideStop();return;}
 if(traversal->slide.active){
  float moved=hypotf(g_CurrentPlayer->field_488.collision_position.x-traversal->slide.lastX,g_CurrentPlayer->field_488.collision_position.z-traversal->slide.lastZ);traversal->slide.blocked=moved<.25f?traversal->slide.blocked+1:0;
  if(request||pendingJump||!slideGrounded()||g_CurrentPlayer->crouchpos==2||(traversal->slide.elapsed>6&&traversal->slide.blocked>=3))slideStop();
 /* The custom input adapter may already have applied crouch this tick.
  * Prior sprintOn proves the entry came from a standing sprint. */
 }else if(request&&traversal->slide.cooldown<=0&&sprintOn&&sprintBlend>.3f&&slideGrounded()&&!pendingJump){
  float a=g_CurrentPlayer->vv_theta*.01745329252f,f=traversal->slide.lastForward,s=traversal->slide.lastSide,v=hypotf(f,s);
  if(v>1.15f&&hypotf(g_CurrentPlayer->field_488.collision_position.x-traversal->slide.lastX,g_CurrentPlayer->field_488.collision_position.z-traversal->slide.lastZ)>fmaxf(.1f,dt)){traversal->slide.dx=(-sinf(a)*f-cosf(a)*s)/v;traversal->slide.dz=(cosf(a)*f-sinf(a)*s)/v;traversal->slide.entry=fminf(v,1.65f);traversal->slide.ready=1;}
 }
}
static void slideAfter(void){
 if(traversal->slide.ready){traversal->slide.ready=0;traversal->slide.active=1;traversal->slide.starts++;traversal->slide.elapsed=0;traversal->slide.blocked=0;g_CurrentPlayer->crouchpos=0;sprintOn=0;sprintBlend=0;}
 if(traversal->slide.active){
  if(!traversal->allowed||traversal->blocked||pendingJump||!slideGrounded()){slideStop();return;}
  float dt=g_GlobalTimerDelta;if(!isfinite(dt)||dt<=0)return;traversal->slide.elapsed+=dt;
  if(traversal->slide.elapsed>=51){slideStop();return;}
  float a=g_CurrentPlayer->vv_theta*.01745329252f,f=g_CurrentPlayer->speedforwards,s=g_CurrentPlayer->speedsideways,length=hypotf(f,s);
  if(length>.1f){float dx=(-sinf(a)*f-cosf(a)*s)/length,dz=(cosf(a)*f-sinf(a)*s)/length,dot=dx*traversal->slide.dx+dz*traversal->slide.dz;
   if(dot<-.5f){slideStop();return;}float blend=fminf(.06f,dt*.012f);traversal->slide.dx+=(dx-traversal->slide.dx)*blend;traversal->slide.dz+=(dz-traversal->slide.dz)*blend;float n=hypotf(traversal->slide.dx,traversal->slide.dz);if(n>.001f){traversal->slide.dx/=n;traversal->slide.dz/=n;}
  }
  float speed=traversal->slide.entry*(1-.70f*traversal->slide.elapsed/51.f);g_CurrentPlayer->crouchpos=0;
  g_CurrentPlayer->speedforwards=speed*(-sinf(a)*traversal->slide.dx+cosf(a)*traversal->slide.dz);g_CurrentPlayer->speedsideways=speed*(-cosf(a)*traversal->slide.dx-sinf(a)*traversal->slide.dz);
 }
 traversal->slide.lastForward=g_CurrentPlayer->speedforwards;traversal->slide.lastSide=g_CurrentPlayer->speedsideways;traversal->slide.lastX=g_CurrentPlayer->field_488.collision_position.x;traversal->slide.lastZ=g_CurrentPlayer->field_488.collision_position.z;
}
