/* Offline Temple movement exploration. No effect without explicit test flags. */
static int templeTicks;static volatile LONG templeFrameCapture;
static void templeGrid(void){
 FILE*f=fopen("temple-floor-grid.csv","w");if(!f)ExitProcess(2);fprintf(f,"x,z,floor,room\n");
 for(int z=-1600;z<=1200;z+=100)for(int x=-1400;x<=1400;x+=100){float h=NAN;void*t=floorBelow((float)x,470,(float)z,&h);fprintf(f,"%d,%d,%.3f,%d\n",x,z,t?h:-99999.f,t?FN(int(*)(void*),0x1000+883888)(t):-1);}fclose(f);
 int n=*(int*)(base+0x228000+4208);f=fopen("temple-room-grid.csv","w");if(!f)ExitProcess(2);fprintf(f,"room,loaded,bounds,x0,y0,z0,x1,y1,z1\n");for(int r=0;r<n;r++){unsigned char*p=(void*)(base+0x303000+27264+r*0x68);float*lo=(void*)(p+0x50),*hi=(void*)(p+0x5c);fprintf(f,"%d,%d,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",r,p[2],*(void**)(p+0x40)!=NULL,lo[0],lo[1],lo[2],hi[0],hi[1],hi[2]);}fclose(f);
 if(logfile){fprintf(logfile,"Temple native floor grid captured; rooms=%d scale=%.4f\n",n,*(float*)(base+0x228000+4252));fflush(logfile);}ExitProcess(0);
}
typedef struct {float x,z,yaw;int forward,mode;} TempleCase;
static TempleCase templeCases[96];static int templeCaseCount,templeCaseIndex,templeCaseTick,templeReady,templeTransitions,templeEscapes,templePhysicalFailures,templeCameraOutside,templeLaunchBase,templeExecuted;typedef struct {JumpVec pos;float head;} TempleSample;static TempleSample templeSamples[360];static int templeSampleCount;
static JumpVec templePrevious;static int templePreviousValid;static FILE*templeTrace;
static void templeCheck(int okay,const char*why){if(!okay){if(logfile){fprintf(logfile,"FAIL Temple movement: %s case=%d tick=%d xz=%.2f %.2f foot=%.2f floor=%.2f drop=%d falling=%d\n",why,templeCaseIndex,templeCaseTick,PF(1864),PF(1872),PF(140),PF(144),drop.active,drop.falling);fflush(logfile);}ExitProcess(2);}}
static void templeStartCase(void){
 TempleCase*c=&templeCases[templeCaseIndex];padTestPlacePlayer(c->x,c->z,470,c->yaw);PF(396)=0;PF(416)=PF(420)=PF(424)=PF(14092)=0;PF(428)=1;PF(384)=0;PF(432)=PF(436)=PF(440)=0;sprintLatched=sprintOn=0;sprintBlend=0;slideStop();slide.cooldown=0;gadget.placed=gadget.flight=0;drop.active=drop.falling=0;
 if(c->mode==5){gadget.position=(JumpVec){PF(1864),PF(140),PF(1872)};gadget.placed=1;gadget.cooldown=20;}
 templeLaunchBase=gadget.launches;templeSampleCount=0;templePreviousValid=0;templeCaseTick=0;if(logfile){fprintf(logfile,"Temple case %d start %.1f %.1f yaw %.0f forward=%d mode=%d\n",templeCaseIndex,c->x,c->z,c->yaw,c->forward,c->mode);fflush(logfile);}
}
static void templeBuildCases(void){
 for(int edge=0;edge<4;edge++)for(int n=12;n<=32;n+=4){TempleCase c={-150,-700,0,n,0};if(edge==1)c=(TempleCase){390,-310,90,n,0};if(edge==2)c=(TempleCase){-700,-310,270,n,0};if(edge==3)c=(TempleCase){-150,80,180,n,0};templeCases[templeCaseCount++]=c;}
 for(int edge=0;edge<4;edge++)for(int corner=0;corner<2;corner++)for(int mode=1;mode<=4;mode++){
  TempleCase c={corner?240:-550,-700,0,24,mode};if(edge==1)c=(TempleCase){390,corner?-70:-550,90,24,mode};if(edge==2)c=(TempleCase){-700,corner?-70:-550,270,24,mode};if(edge==3)c=(TempleCase){corner?240:-550,80,180,24,mode};templeCases[templeCaseCount++]=c;
 }
 for(int edge=0;edge<4;edge++)for(int corner=0;corner<3;corner++){
  float side=corner==0?-550:corner==1?-150:240;TempleCase c={side,-700,0,46,6};if(edge==1)c=(TempleCase){390,corner==0?-550:corner==1?-310:-70,90,46,6};if(edge==2)c=(TempleCase){-700,corner==0?-550:corner==1?-310:-70,270,46,6};if(edge==3)c=(TempleCase){side,80,180,46,6};templeCases[templeCaseCount++]=c;
 }
 for(int angle=0;angle<360;angle+=45)templeCases[templeCaseCount++]=(TempleCase){-150,-310,(float)angle,0,5};
 templeTrace=fopen("temple-movement.csv","w");templeCheck(templeTrace!=NULL,"trace file");fprintf(templeTrace,"case,tick,x,z,foot,ground,tilefloor,tileRoom,highestFloor,highestRoom,drop,falling\n");templeReady=1;if(getenv("GE32_TEMPLE_START_CASE"))templeCaseIndex=atoi(getenv("GE32_TEMPLE_START_CASE"));templeCheck(templeCaseIndex>=0&&templeCaseIndex<templeCaseCount,"valid start case");templeStartCase();
}
static void templeStressBefore(signed char*x,signed char*y,unsigned short*buttons){
 if(!getenv("GE32_TEMPLE_PROBE")&&!getenv("GE32_TEMPLE_STRESS"))return;*x=*y=0;*buttons=0;if(!jumpAllowed())return;
 if(++templeTicks==300&&getenv("GE32_TEMPLE_PROBE"))templeGrid();
 if(templeTicks<300)return;if(!templeReady)templeBuildCases();
 TempleCase*c=&templeCases[templeCaseIndex];PF(380)=c->yaw;FN(void(*)(void),0x1000+87712)();templeCaseTick++;
 if(c->mode==5){if(gadget.flight&&PF(140)>550)*buttons=8;if(!gadget.flight&&gadget.launches>templeLaunchBase)gadget.placed=0;return;}
 if(templeCaseTick>12&&templeCaseTick<=12+c->forward)*buttons=8;else if(templeCaseTick>12+c->forward&&templeCaseTick<=72+c->forward)*buttons=4;
 if(c->mode==1&&*buttons)*buttons|=(templeCaseTick<36?1:2);
 if((c->mode==2||c->mode==6)&&templeCaseTick==18)InterlockedIncrement(&sprintClicks);
 if(c->mode==6&&templeCaseTick==26){PI(184)=0;InterlockedExchange(&slideRequests,1);}
 if(c->mode==3&&templeCaseTick==18)InterlockedExchange(&pendingJump,1);
 if(c->mode==4)PI(184)=0;
}
/* Independent test oracle warms intersected room geometry, then checks the
 * actual body path with background triangles, not the overridden STAN tile. */
static int templeTriangleClear(JumpVec a,JumpVec b){
 float scale=*(float*)(base+0x228000+4252),dx=b.x-a.x,dy=b.y-a.y,dz=b.z-a.z,len=dx*dx+dy*dy+dz*dz;if(len<.0001f)return 1;
 int total=*(int*)(base+0x228000+4208);for(int r=1;r<total;r++){unsigned char*p=(void*)(base+0x303000+27264+r*0x68);if(!*(void**)(p+0x40))continue;JumpHit hit={0};if(FN(int(*)(JumpVec*,JumpVec*,int,JumpHit*),0x1000+13744)(&a,&b,r,&hit)){float t=((hit.position.x/scale-a.x)*dx+(hit.position.y/scale-a.y)*dy+(hit.position.z/scale-a.z)*dz)/len;if(t>.005f&&t<.995f)return 0;}}
 return 1;
}
static void templePrepareGeometry(JumpVec a,JumpVec b,float height){
 float scale=*(float*)(base+0x228000+4252);float low[3]={fminf(a.x,b.x)-32,fminf(a.y,b.y)+5,fminf(a.z,b.z)-32},high[3]={fmaxf(a.x,b.x)+32,fmaxf(a.y,b.y)+height,fmaxf(a.z,b.z)+32};int total=*(int*)(base+0x228000+4208);
 for(int r=1;r<total;r++){unsigned char*p=(void*)(base+0x303000+27264+r*0x68);float*lo=(void*)(p+0x50),*hi=(void*)(p+0x5c);int overlap=1;for(int i=0;i<3;i++)if(low[i]*scale>hi[i]+1||high[i]*scale<lo[i]-1)overlap=0;if(overlap&&*(int*)(p+36)>0)jumpRoomReady(r);}
}
static int templePhysicalClear(JumpVec before,JumpVec now,float head){
 templePrepareGeometry(before,now,head);
 for(int i=0;i<9;i++){float ox=0,oz=0;if(i){float angle=(i-1)*.7853981634f;ox=26*cosf(angle);oz=26*sinf(angle);}
  JumpVec lo={now.x+ox,now.y+8,now.z+oz},hi={lo.x,now.y+head,lo.z};if(!templeTriangleClear(lo,hi))return 0;
  for(int level=0;level<3;level++){float h=8+(head-8)*level*.5f;JumpVec a={before.x+ox,before.y+h,before.z+oz},b={now.x+ox,now.y+h,now.z+oz};if(!templeTriangleClear(a,b))return 0;if(i){JumpVec c={now.x,now.y+h,now.z};if(!templeTriangleClear(c,b))return 0;}}
 }
 return 1;
}
static void templeStressAfter(void){
 if(!getenv("GE32_TEMPLE_STRESS")||!templeReady)return;
 float high=NAN;void*t=floorBelow(PF(1864),PF(140)+2,PF(1872),&high);void*actual=PP(1856);float tileY=actual?FN(float(*)(void*,float,float),0x1000+879136)(actual,PF(1864),PF(1872)):NAN;
 templeCheck(isfinite(PF(140))&&isfinite(PF(1864))&&isfinite(PF(1872))&&PF(140)>-1200&&t,"finite position above a real floor");
 int room=actual?FN(int(*)(void*),0x1000+883888)(actual):-1,highRoom=t?FN(int(*)(void*),0x1000+883888)(t):-1;
 fprintf(templeTrace,"%d,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%.3f,%d,%d,%d\n",templeCaseIndex,templeCaseTick,PF(1864),PF(1872),PF(140),PF(144),tileY,room,high,highRoom,drop.active,drop.falling);fflush(templeTrace);
 JumpVec now={PF(1864),PF(140),PF(1872)};
 if(getenv("GE32_TEMPLE_GEOMETRY_AUDIT")&&templeCaseTick>12&&templePreviousValid&&!templePhysicalClear(templePrevious,now,fmaxf(40,PF(13940)+PF(188)+8))){templePhysicalFailures++;templeCheck(0,"body crossed or penetrated solid background geometry");}
 if(templeCaseTick>=12){templeCheck(templeSampleCount<360,"movement replay capacity");templeSamples[templeSampleCount++]=(TempleSample){now,fmaxf(40,PF(13940)+PF(188)+8)};}
 templePrevious=now;templePreviousValid=1;
 int cameraRoom=FN(int(*)(void),0x1000+114272)();JumpVec*camera=FN(JumpVec*(*)(void),0x1000+114352)();int total=*(int*)(base+0x228000+4208);if(camera&&cameraRoom>0&&cameraRoom<total){unsigned char*p=(void*)(base+0x303000+27264+cameraRoom*0x68);float*lo=(void*)(p+0x50),*hi=(void*)(p+0x5c);float scale=*(float*)(base+0x228000+4252);if(camera->y*scale>hi[1]+2||camera->y*scale<lo[1]-2){if(!templeCameraOutside&&getenv("GE32_TEMPLE_VISUAL_TEST")){InterlockedExchange(&templeFrameCapture,1);if(logfile){fprintf(logfile,"Temple camera transition sample case=%d tick=%d room=%d eyeY=%.2f roomY=%.2f..%.2f foot=%.2f\n",templeCaseIndex,templeCaseTick,cameraRoom,camera->y,lo[1]/scale,hi[1]/scale,PF(140));fflush(logfile);}}templeCameraOutside++;}}
 if(drop.active)templeTransitions++;
 if(templeCaseTick>=(templeCases[templeCaseIndex].mode==5?300:140)){
  if(getenv("GE32_TEMPLE_DEFERRED_AUDIT"))for(int i=1;i<templeSampleCount;i++)templeCheck(templePhysicalClear(templeSamples[i-1].pos,templeSamples[i].pos,fmaxf(templeSamples[i-1].head,templeSamples[i].head)),"recorded movement crossed solid geometry");
  templeCheck(actual&&FN(int(*)(void*,float,float),0x1000+871952)(actual,PF(1864),PF(1872)),"settled anchor contains player");
  if(fabsf(PF(140)-high)<1.5f&&fabsf(tileY-high)>16){templeEscapes++;templeCheck(0,"standing above the wrong navigation floor after retreat");}
  if(templeCases[templeCaseIndex].mode==5)templeCheck(PF(144)>460,"pad route reaches upper floor");
  templeCheck(fabsf(PF(140)-PF(144))<1.5f,"settled after edge approach/retreat");
  templeExecuted++;if(++templeCaseIndex==templeCaseCount){if(logfile){fprintf(logfile,"PASS Temple movement: %d completed routes, traversal samples=%d, escapes=%d physicalFailures=%d cameraOutsideSamples=%d\n",templeExecuted,templeTransitions,templeEscapes,templePhysicalFailures,templeCameraOutside);fflush(logfile);}fclose(templeTrace);ExitProcess(0);}templeStartCase();
 }
}
