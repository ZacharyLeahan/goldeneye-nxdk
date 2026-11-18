/* Windows level replay. The case file is local to each offline test process. */
typedef struct { char name[48]; float x,y,z,yaw,top; int forward,hold,retry,expect; } MantleRoute;
static MantleRoute route;
static FILE *routeFile;
static int routeTick,routeGrab,routeLand,routeCount,routeFailures,routeJumps,routeReady;
static float routePeak,routeHead,routeRoof;
static int routeHasLanding,routeNearTicks,routeWallCrossed;
static JumpVec routeDestination,routePress,routeVerifiedTop;
static int routeShotTick,routeShotDuring;
static float routeRoofHeight(JumpVec land){
 JumpVec low={land.x,land.y+30,land.z},high={land.x,land.y+3000,land.z};
 if(!collisionPrepareBox(low,high))return INFINITY;
 float roof=INFINITY,scale=*(float*)(base+0x228000+4252);
 for(int r=1;r<*(int*)(base+0x228000+4208);r++){
  JumpHit hit={0};
  if(FN(int(*)(JumpVec*,JumpVec*,int,JumpHit*),0x1000+13744)(&low,&high,r,&hit)){
   float y=hit.position.y/scale;
   if(y>land.y+130&&y<high.y&&y<roof)roof=y;
  }
 }
 return roof;
}
static void routeBefore(unsigned short *buttons){
 *buttons=0;
 if(!jumpAllowed())return;
 if(!routeFile){
  if(++routeReady<85)return;
  routeFile=fopen("mantle-routes.txt","r");
  if(!routeFile)ExitProcess(3);
 }
 if(++routeTick==1){
  int fields=fscanf(routeFile,"%47s %f %f %f %f %f %d %d %d %d",route.name,&route.x,&route.y,&route.z,&route.yaw,&route.top,&route.forward,&route.hold,&route.retry,&route.expect);
  if(fields==EOF){fprintf(logfile,"END routes=%d failures=%d stage=%d\n",routeCount,routeFailures,mantle.stage);fflush(logfile);ExitProcess(routeCount&&!routeFailures?0:2);}
  if(fields!=10)ExitProcess(3);
  routeRoof=INFINITY;routeHead=PF(13940);
  if(route.expect==3&&routeHasLanding)routeRoof=routeRoofHeight(routeVerifiedTop);
  if(route.expect==3&&!isfinite(routeRoof)){fprintf(logfile,"SKIP %s no measured roof over verified landing\n",route.name);routeCount++;routeTick=0;return;}
 }
 if(routeTick==12){
  float floor;
  if(!floorBelow(route.x,route.y+4,route.z,&floor)){
   fprintf(logfile,"SKIP %s no native starting floor at coordinates\n",route.name);routeCount++;routeTick=0;return;
  }
  padTestPlacePlayer(route.x,route.z,route.y+4,route.yaw);
  PF(396)=PF(416)=PF(420)=PF(424)=PF(432)=PF(436)=PF(440)=PF(14092)=0;PF(428)=1;
  *(JumpVec*)(base+3238208)=(JumpVec){0};routeShotDuring=routeShotTick=routeNearTicks=routeWallCrossed=0;mantle.found=mantle.clear=mantle.chose=0;
  mantleStop();jumpCooldown=0;routeGrab=routeLand=0;routePeak=PF(140);routeJumps=jumpCount;
 }
 if(routeTick==12&&route.expect==4){
  FN(int(*)(int),0x1000+44224)(5);swapNow(0,5);HI(0,44)=7;
 }
 if(routeTick==16)routePress=mantleFeet();
 if(route.expect==3&&routeTick>=15&&isfinite(routeRoof))PF(13940)=fmaxf(routeHead,routeRoof-route.top+2);
 if(routeTick>=16+route.forward&&!routeGrab&&routeTick<60)*buttons=8;
 if(strcmp(route.name,"glance")==0&&routeTick>=25)*buttons=0;
 if(route.forward==99)*buttons=0;
 if(route.expect==4&&routeGrab){
  *buttons|=0x2000;
  if(routeLand&&routeTick>routeLand&&routeTick<=routeLand+5)*buttons|=8;
 }
 jumpA(routeTick==16||(route.hold&&routeTick>=16&&routeTick<=52)||(route.retry&&routeTick==16+route.retry),1,(volatile LONG*)(base+0x303000+1983488));
}
static void routeAfter(void){
 if(!routeFile||!routeTick||!jumpAllowed())return;
 routePeak=fmaxf(routePeak,PF(140));
 if(mantle.active&&!routeGrab){routeGrab=routeTick;routeDestination=mantle.land;}
 if(routeGrab&&!mantle.active&&!routeLand)routeLand=routeTick;
 if(routeLand&&fabsf(PF(140)-route.top)<4&&hypotf(PF(1864)-routeDestination.x,PF(1872)-routeDestination.z)<35)routeNearTicks++;
 if(route.expect==4&&routeGrab&&HI(0,44)<7){
  if(mantle.active)routeShotDuring=1;
  else if(!routeShotTick)routeShotTick=routeTick;
 }
 if(routeTick>=16&&strcmp(route.name,"back-wall")==0&&PF(1872)>routePress.z+2)routeWallCrossed=1;
 if(routeTick<100)return;
 int stable=fabsf(PF(140)-PF(144))<2&&fabsf(PF(152))<.6f;
 int builtInJump=route.expect==2&&!routeGrab&&jumpCount==routeJumps+1&&fabsf(PF(140)-route.top)<4
  &&route.top>routePress.y+5&&hypotf(PF(1864)-routePress.x,PF(1872)-routePress.z)>45;
 int ok=stable&&(route.expect?(routeGrab&&routeLand>routeGrab&&mantle.clear&&fabsf(PF(140)-route.top)<4&&routeNearTicks>=8):(!routeGrab&&jumpCount==routeJumps+1&&routePeak>route.y+5));
 /* From the same rear-wall spawn, turning to face the wall must still
  * block forward motion throughout the jump. */
 if(strcmp(route.name,"back-wall")==0)ok=ok&&!routeWallCrossed;
 if(route.expect==2&&stable&&builtInJump)ok=1;
 if(route.expect==3){
  PF(13940)=routeHead;

  ok=!routeGrab;
 }
 if(route.expect==4)ok=routeLand&&stable&&fabsf(PF(140)-route.top)<4&&!routeShotDuring&&routeShotTick>=routeLand
   &&hypotf(PF(1864)-routeDestination.x,PF(1872)-routeDestination.z)>1;
 if(strcmp(route.name,"tap")==0){routeHasLanding=ok&&routeLand;routeVerifiedTop=routeDestination;}
 fprintf(logfile,"%s %s stage=%d jumps=%d grab=%d land=%d stable=%d peak=%.1f end=(%.1f %.1f %.1f) shot=%d found=%d clear=%d\n",ok?"PASS":"FAIL",route.name,mantle.stage,jumpCount-routeJumps,routeGrab,routeLand,stable,routePeak,PF(1864),PF(140),PF(1872),routeShotTick,mantle.found,mantle.clear);
 fflush(logfile);routeCount++;routeFailures+=!ok;routeTick=0;
}
