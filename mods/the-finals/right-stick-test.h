/* Offline controller regression. A virtual SDL gamepad supplies real R3 edges
 * through GE32's own polling. No test writes to the melee request queue. */
static volatile LONG r3Down,r3Applied;
static void *r3Joystick;
static int (*r3SetButton)(void*,int,unsigned char);
static int r3PressTick,r3Case,r3Tick,r3Slaps,r3Events,r3Ammo,r3PistolAmmo,r3Deploys,r3Right,r3Clip;
static void *r3Target;
static float r3TargetHealth,r3TargetYaw;
static JumpVec r3TargetStart;
static int r3HitCount;
static int r3Wait,r3Grab,r3Landed,r3SawRelease,r3SawStrike;
static void r3Check(int ok,const char *message){
 if(ok)return;
 fprintf(logfile,"FAIL R3 case=%d tick=%d: %s (weapon=%d phase=%d strikes=%d)\n",
  r3Case,r3Tick,message,HI(0,0),slap.phase,slap.events-r3Events);
 fprintf(logfile,"DETAIL applied=%ld desired=%ld clips=%d/%d equipment=%d/%d\n",r3Applied,r3Down,HI(0,44),r3Clip,HI(0,0),r3Right);fflush(logfile);ExitProcess(2);
}
static void r3TestRender(void){
 if(!getenv("GE32_R3_TEST"))return;
 static int frames;
 if(!r3Joystick){
  HMODULE sdl=GetModuleHandleA("SDL2.dll");
  int (*attach)(int,int,int,int)=(void*)GetProcAddress(sdl,"SDL_JoystickAttachVirtual");
  void *(*openJoystick)(int)=(void*)GetProcAddress(sdl,"SDL_JoystickOpen");
  r3SetButton=(void*)GetProcAddress(sdl,"SDL_JoystickSetVirtualButton");
  int index=attach&&openJoystick&&r3SetButton?attach(1,6,21,0):-1;
  if(index<0||!(r3Joystick=openJoystick(index))){fprintf(logfile,"FAIL SDL virtual gamepad setup\n");fflush(logfile);ExitProcess(2);}
 }
 LONG down=InterlockedCompareExchange(&r3Down,0,0);
 if(down!=r3Applied){
  if(r3SetButton(r3Joystick,8,(unsigned char)down)<0)ExitProcess(2);
  InterlockedExchange(&r3Applied,down);
 }
 /* The damage assertion needs one real opponent. */
 if(++frames==60)FN(void(*)(int,int),0x1000+999872)(2,1);
}
static void r3TestBefore(unsigned short *buttons){
 if(!jumpAllowed())return;
 *(int*)(base+0x303000+618564)=1;
 *buttons=0;PF(432)=PF(436)=PF(440)=0;
 int t=++r3Tick;
 if(t==1){
  padTestPlacePlayer(1009.9f,415.f,4,180);PF(396)=PF(416)=PF(420)=PF(424)=0;
 }
 if(t==40){
  r3Check(*(int*)(base+0x303000+211452)==31,"Complex practice loaded");
  if(r3Case==1)FN(int(*)(int),0x1000+44224)(5);
  padStow();r3Right=r3Case==0?1:r3Case==2||r3Case==3?GRENADE_ITEM:5;
  FN(void(*)(int,int,int),0x1000+420240)(0,r3Right,1);
  FN(void(*)(int,int,int),0x1000+420240)(1,0,1);
  r3Wait=0;r3PressTick=0;

 }
 if(r3Case==1&&t>=45){
  int owner=localIndex(),slot=owner?0:1;
  r3Check(PLAYERS[slot]!=NULL,"practice target exists");
  FN(void(*)(int),0xa4c10)(slot);
  if(t==45){r3TargetStart=mantleFeet();r3TargetYaw=PF(380);}
  padTestPlacePlayer(1009.9f,338.f,4,0);
  r3Target=PP(200);if(t==45){PI(284)=-1;PF(260)=1.f;PF(264)=0;}
  FN(void(*)(int),0xa4c10)(owner);
  PF(380)=180;PF(396)=-12;FN(void(*)(void),0x166a0)();
 }
 if(t==65){
  if(HI(0,0)!=r3Right||HI(0,4)>=0||HI(1,4)>=0||HI(0,36)!=0){
   r3Check(++r3Wait<220,"native equip became ready");r3Tick--;return;
  }
  if(r3Case==1){HealthValue v;r3Check(healthRead(r3Target,&v)&&v.alive,"target alive");r3TargetHealth=v.total;r3HitCount=healthConfirmations;}
  if(r3Right==5)HI(0,44)=7;
  if(r3Case==3)padEquip();
  if(r3Case==4){
   padTestPlacePlayer(1009.9f,125.2f,4,180);PF(396)=PF(416)=PF(420)=PF(424)=0;
   jumpCooldown=0;mantleStop();
  }
  r3Slaps=slap.slaps;r3Events=slap.events;r3Ammo=grenadeAmmo();r3PistolAmmo=testTotalAmmo(1);
  r3Deploys=gadget.deploys;r3Clip=HI(0,44);
  if(r3Case!=4)InterlockedExchange(&r3Down,1);
 }
 if(r3Case==4&&t>=65){
  jumpA(t==65,1,(volatile LONG*)(base+0x303000+1983488));
  if(!r3Grab)*buttons=8;
  if(mantle.active&&!r3Grab){r3Grab=t;InterlockedExchange(&r3Down,1);}
  if(r3Grab&&!mantle.active&&!r3Landed){
   r3Landed=t;r3Check(mantle.clear&&fabsf(PF(140)-281.06f)<3,"finished the climb while R3 was held");
  }
  if(r3Landed&&t==r3Landed+10){
   r3Check(r3Applied&&slap.slaps==r3Slaps&&!slap.phase,"held climb click did not attack at landing");
   InterlockedExchange(&r3Down,0);
  }
  if(r3Landed&&t==r3Landed+22){r3SawRelease=!r3Applied;InterlockedExchange(&r3Down,1);}
 }
 /* A short tap with the pistol and grenade, a long hold with the pad and fists. */
 if((r3Case==1||r3Case==2)&&r3Applied){
  if(!r3PressTick)r3PressTick=t;
  if(t-r3PressTick>=3)InterlockedExchange(&r3Down,0);
 }
 int release=r3Case==4?r3Landed+148:205;
 if(slap.phase==2&&HI(0,0)==1&&!padCarrying)r3SawStrike=1;
 if(t==release&&r3Applied)InterlockedExchange(&r3Down,0);
 if(t>release+12&&(r3Case!=4||r3Landed)){
  if(r3Case==1){HealthValue v;r3Check(healthRead(r3Target,&v),"target health available");fprintf(logfile,"R3 target %.3f -> %.3f confirmations %d\n",r3TargetHealth,v.total,healthConfirmations-r3HitCount);r3Check(v.total<r3TargetHealth&&healthConfirmations==r3HitCount+1,"native palm damaged the target once");}
  r3Check(r3Case!=4||r3SawRelease,"post-landing click followed a release");
  r3Check(r3SawStrike&&slap.slaps==r3Slaps+1&&slap.events==r3Events+1&&!slap.phase,"one palm attack and one native strike per click");
  r3Check(!r3Applied&&HI(0,0)==r3Right&&(r3Right==1||HI(0,44)==r3Clip),"restored item and loaded ammunition");
  r3Check(grenadeAmmo()==r3Ammo&&testTotalAmmo(1)==r3PistolAmmo,"preserved grenade and pistol ammunition");
  r3Check(padCarrying==(r3Case==3)&&gadget.deploys==r3Deploys,"restored equipment without deploying pad");
  fprintf(logfile,"PASS SDL R3 case %d item %d strikes 1 grenades %d pad %d\n",r3Case,r3Right,r3Ammo,(int)padCarrying);fflush(logfile);
  if(r3Case==1){int owner=localIndex();FN(void(*)(int),0xa4c10)(owner?0:1);
   padTestPlacePlayer(r3TargetStart.x,r3TargetStart.z,r3TargetStart.y+4,r3TargetYaw);
   FN(void(*)(int),0xa4c10)(owner);}
  if(++r3Case==5)ExitProcess(0);
  r3Tick=r3SawStrike=0;
 }
 r3Check(t<450,"controller or climb deadline");
}
