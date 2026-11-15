#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
static uintptr_t base;
static void *pad;
static volatile LONG openWheel;
static int previousLB,choice,cancelled,ids[32],count;
static int oldCurrent=-1;
static unsigned char (*getButton)(void*,int);
static short (*getAxis)(void*,int);
static void (*swapWindow)(void*);
static void (*getSize)(void*,int*,int*);
static void (*useProgram)(unsigned);
static void (*activeTexture)(unsigned);
static FILE *logfile;
#define FN(type,rva) ((type)(base+(rva)))
/* All engine inventory calls run on the game's tick thread. The render and
 * input threads consume a lock-protected snapshot and queue equip requests. */
#define CURRENT (*(void**)(base+0x303000+392272))
#define NUMBER (*(int*)(base+0x362c44))
#define PLAYERS ((void**)(base+0x303000+399456))
typedef struct {int ids[32],count,current,local,valid,allowJump;DWORD tickThread;} InventorySnapshot;
static InventorySnapshot published;
static SRWLOCK snapshotLock=SRWLOCK_INIT;
static volatile LONG pendingWeapon=-1,gameTestsReady,testAction;
static DWORD renderThread;
static void (*originalTick)(signed char,signed char,unsigned short,unsigned short);
static int localIndex(void){int n=FN(int(*)(void),0x1000+936896)();return n>=0&&n<32?n:0;}
static InventorySnapshot snapshotRead(void){InventorySnapshot r;AcquireSRWLockShared(&snapshotLock);r=published;ReleaseSRWLockShared(&snapshotLock);return r;}
static int playing(void){int m=*(int*)(base+0x228000+59840);InventorySnapshot s=snapshotRead();return (m==11||m==-1)&&s.valid;}
static int available(int id){InventorySnapshot s=snapshotRead();for(int j=0;j<s.count;j++)if(s.ids[j]==id)return 1;return 0;}
static int current(void){return snapshotRead().current;}
static void equip(int id){InterlockedExchange(&pendingWeapon,id);if(logfile){fprintf(logfile,"queue weapon=%d\n",id);fflush(logfile);}}
static void publishInventory(int owner,const InventorySnapshot*s){if(owner!=localIndex())return;AcquireSRWLockExclusive(&snapshotLock);published=*s;ReleaseSRWLockExclusive(&snapshotLock);}
#include "jump.h"
#include "combat.h"
#include "combat-test.h"
#include "ledge-drop.h"
#include "ledge-drop-test.h"
static void gameTickHook(signed char x,signed char y,unsigned short buttons,unsigned short oldbuttons){
 if(getenv("GE32_JUMP_MOVING_TEST")&&NUMBER==localIndex()){x=0;y=0;buttons=(buttons&~15)|jumpTestMotion;}
 int isLocal=NUMBER==localIndex()&&CURRENT&&CURRENT==PLAYERS[localIndex()];
 if(isLocal&&getenv("GE32_REGRESSION_PRACTICE")&&jumpAllowed())*(int*)(base+0x303000+618564)=1;
 if(isLocal&&getenv("GE32_DROP_TEST")){x=0;y=0;}
 if(isLocal){combatTestBefore(&buttons);dropTestBefore(&buttons);}
 unsigned short rawButtons=buttons;
 if(isLocal)combatBefore(&buttons,&oldbuttons);
 originalTick(x,y,buttons,oldbuttons);
 if(isLocal){combatAfter(rawButtons);combatTestAfter(rawButtons);dropTestAfter();}
 int n=localIndex();if(NUMBER!=n||!CURRENT||CURRENT!=PLAYERS[n])return;
 static int testTicks=0;
 if(getenv("GE32_WHEEL_TEST")){
  if(++testTicks==90){FN(int(*)(int),0x1000+44224)(5);FN(int(*)(int),0x1000+44224)(8);FN(void(*)(void),0x1000+459232)();InterlockedExchange(&gameTestsReady,1);}
  int action=InterlockedExchange(&testAction,0);
  if(action==1)FN(int(*)(int),0x1000+44224)(6);
  if(action==2)FN(void(*)(int),0x1000+44864)(6);
  if(action==3){FN(void(*)(int),0x1000+44864)(5);FN(void(*)(int),0x1000+44864)(8);}
 }
 int wanted=InterlockedExchange(&pendingWeapon,-1);
 if(wanted>=4&&wanted<=25&&!combatDeferWeapon(wanted)&&FN(int(*)(int),0x1000+43904)(wanted)){
  if(logfile){fprintf(logfile,"game accepted weapon=%d\n",wanted);fflush(logfile);}
  FN(void(*)(int,int,int),0x1000+420240)(0,wanted,1);
  FN(void(*)(int,int,int),0x1000+420240)(1,0,1);
 }
 InventorySnapshot next={0};next.current=FN(int(*)(int),0x1000+421920)(0);next.local=n;next.valid=1;next.allowJump=jumpAllowed();next.tickThread=GetCurrentThreadId();
 for(int id=4;id<=25;id++){if(id!=23&&FN(int(*)(int),0x1000+43904)(id))next.ids[next.count++]=id;}
 publishInventory(n,&next);
}
static int installGameHook(void){
 unsigned char expected[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x57,0x56,0x53};
 unsigned char*target=(void*)(base+0x172e0);if(memcmp(target,expected,12))return 0;
 unsigned char*trampoline=VirtualAlloc(NULL,64,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!trampoline)return 0;
 memcpy(trampoline,target,12);trampoline[12]=0x48;trampoline[13]=0xb8;*(uintptr_t*)(trampoline+14)=(uintptr_t)(target+12);trampoline[22]=0xff;trampoline[23]=0xe0;
 DWORD prot;VirtualProtect(trampoline,64,PAGE_EXECUTE_READ,&prot);originalTick=(void*)trampoline;
 unsigned char jump[]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};*(uintptr_t*)(jump+2)=(uintptr_t)gameTickHook;
 VirtualProtect(target,12,PAGE_EXECUTE_READWRITE,&prot);memcpy(target,jump,12);VirtualProtect(target,12,prot,&prot);FlushInstructionCache(GetCurrentProcess(),target,12);return 1;
}
static const char *names[]={"Unarmed","Fist","Knife","Throwing knife","PP7","PP7 silenced","DD44 Dostovei","Klobb","KF7 Soviet","ZMG","D5K Deutsche","D5K silenced","Phantom","AR33","RC-P90","Shotgun","Automatic shotgun","Sniper rifle","Cougar Magnum","Golden Gun","Silver PP7","Gold PP7","Moonraker Laser","Watch Laser","Grenade Launcher","Rocket Launcher"};
static void inventory(void){InventorySnapshot s=snapshotRead();count=s.count;memcpy(ids,s.ids,sizeof(ids));}
static unsigned char buttonHook(void *p,int b){
 if(!pad)pad=p;
 if(p==pad && playing()){
  if(b==7||b==8)return combatButton(b,getButton(p,b),snapshotRead().allowJump&&!openWheel&&!FN(int(*)(void),0x1000+1453264)());
  if(b==9)return 0; /* LB belongs to the wheel, not aim */
  if(openWheel && (b==0||b==1||b==10)){if(b==0)jumpAHeld=getButton(p,b)!=0;return 0;}
  if(b==0){int slot=*(int*)(base+0x228000+203956);if(slot<0||slot>3)slot=0;return jumpA(getButton(p,b),snapshotRead().allowJump&&!FN(int(*)(void),0x1000+1453264)(),(volatile LONG*)(base+0x303000+1983488+slot*4));}
 }
 return getButton(p,b);
}
static short axisHook(void*p,int a){
 pad=p;
 if(p==pad && playing() && (openWheel||getButton(p,9)) && (a==2||a==3||a==4||a==5))return 0;
 return getAxis(p,a);
}
#include "wheel-render.h"
static void wheelUpdate(int lb,int back,float x,float y){
 if(!playing()){openWheel=0;previousLB=lb;cancelled=0;return;}
 if(lb&&!previousLB){
  inventory();oldCurrent=current();choice=count?0:-1;
  for(int i=0;i<count;i++)if(ids[i]==oldCurrent)choice=i;
  openWheel=1;cancelled=0;
  if(logfile){fprintf(logfile,"wheel local=%d context=%d owned=%d current=%d\n",snapshotRead().local,-1,count,oldCurrent);fflush(logfile);}
 }
 if(openWheel&&lb){
  if(back){cancelled=1;openWheel=0;}
  else if(x*x+y*y>.28f*.28f&&count){
   float angle=atan2f(y,x)+1.57079632679f+3.14159265359f/count;
   while(angle<0)angle+=6.28318530718f;
   int target=((int)(angle*count/6.28318530718f))%count;
   /* A small angular latch avoids flickering between neighboring wedges. */
   if(target!=choice){float center=-1.57079632679f+target*6.28318530718f/count;
    float delta=atan2f(sinf(atan2f(y,x)-center),cosf(atan2f(y,x)-center));
    if(choice<0||fabsf(delta)<3.14159265359f/count-.06f)choice=target;
   }
  }
 }
 if(!lb&&previousLB){
  if(openWheel&&!cancelled&&choice>=0&&choice<count&&available(ids[choice]))equip(ids[choice]);
  openWheel=0;cancelled=0;
 }
 previousLB=lb;
}
static void testAssert(int value,const char*message){if(!value){if(logfile){fprintf(logfile,"FAIL: %s\n",message);fflush(logfile);}ExitProcess(2);}}
static int testTick(void*w){
 static int frame,completed;
 if(!getenv("GE32_WHEEL_TEST")||!gameTestsReady||!playing())return 0;
 InventorySnapshot s=snapshotRead();
 testAssert(s.tickThread!=GetCurrentThreadId(),"gameplay and drawing threads are separate");
 if(++frame==1){testAssert(s.count==2,"initial local snapshot contains two guns");if(logfile)fprintf(logfile,"TEST: game thread=%lu render thread=%lu local=%d owned=2\n",s.tickThread,GetCurrentThreadId(),s.local);}
 int cycle=(frame-1)/120,phase=(frame-1)%120;
 if(cycle<20){
  int desired=cycle%2?5:8;float y=desired==5?-1.f:1.f;
  if(phase==0){wheelUpdate(1,0,0,0);testAssert(count==2,"reopened wheel consistently contains two guns");}
  else if(phase<5)wheelUpdate(1,0,0,y);
  else wheelUpdate(0,0,0,0);
  if(phase==119){testAssert(current()==desired,"game thread equipped alternating requested weapon");completed++;}
  if(openWheel&&useProgram&&activeTexture){while(glGetError()!=GL_NO_ERROR){}draw(w);testAssert(glGetError()==GL_NO_ERROR,"wheel renders without GL errors");}
 }else{
  int end=(frame-1)-2400;
  if(end==0){
   for(int i=0;i<100;i++){wheelUpdate(1,0,0,0);testAssert(count==2,"100 rapid opens retain correct ownership");wheelUpdate(1,1,0,0);wheelUpdate(0,0,0,0);}
   InventorySnapshot remote={0};publishInventory(31,&remote);testAssert(snapshotRead().count==2,"foreign player cannot overwrite local inventory snapshot");
   int before=current();wheelUpdate(1,0,0,1);wheelUpdate(1,1,0,1);wheelUpdate(0,0,0,0);testAssert(pendingWeapon==-1&&current()==before,"B cancels without enqueueing a switch");InterlockedExchange(&testAction,1);
  }
  if(end==30){wheelUpdate(1,0,0,0);testAssert(count==3,"pickup added on game thread appears");wheelUpdate(1,1,0,0);wheelUpdate(0,0,0,0);InterlockedExchange(&testAction,2);}
  if(end==60){wheelUpdate(1,0,0,0);testAssert(count==2,"removed gun does not persist as ghost entry");wheelUpdate(1,1,0,0);wheelUpdate(0,0,0,0);InterlockedExchange(&testAction,3);}
  if(end==90){wheelUpdate(1,0,0,0);testAssert(count==0,"empty state after real inventory removal");if(useProgram&&activeTexture){while(glGetError()!=GL_NO_ERROR){}draw(w);testAssert(glGetError()==GL_NO_ERROR,"empty state renders without GL errors");}
   if(logfile){fprintf(logfile,"PASS: %d alternating switches, 100 rapid reopen/cancels, foreign-player isolation, pickup/removal, empty state, GL rendering, separate game/render threads\n",completed);fflush(logfile);}ExitProcess(0);
  }
 }
 return 1;
}
static void swapHook(void*w){
 renderThread=GetCurrentThreadId();
 if(!useProgram){useProgram=(void*)wglGetProcAddress("glUseProgram");activeTexture=(void*)wglGetProcAddress("glActiveTexture");}
 dropTestLaunch();
 if(testTick(w)){swapWindow(w);return;}
 int lb=pad?getButton(pad,9):0,back=pad?getButton(pad,1):0;
 float x=pad?getAxis(pad,2)/32767.f:0,y=pad?getAxis(pad,3)/32767.f:0;
 wheelUpdate(lb,back,x,y);
 if(openWheel&&useProgram&&activeTexture)draw(w);
 swapWindow(w);
}
static int replaceImport(const char*name,void*replacement,void**old){
 IMAGE_DOS_HEADER*d=(void*)base;IMAGE_NT_HEADERS64*n=(void*)(base+d->e_lfanew);
 IMAGE_IMPORT_DESCRIPTOR*desc=(void*)(base+n->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
 for(;desc->Name;desc++){
  if(!desc->OriginalFirstThunk)continue;
  IMAGE_THUNK_DATA64*names=(void*)(base+desc->OriginalFirstThunk),*iat=(void*)(base+desc->FirstThunk);
  for(;names->u1.AddressOfData;names++,iat++){
   if(IMAGE_SNAP_BY_ORDINAL64(names->u1.Ordinal))continue;
   IMAGE_IMPORT_BY_NAME*imp=(void*)(base+names->u1.AddressOfData);
   if(strcmp((char*)imp->Name,name)==0){DWORD prot;*old=(void*)iat->u1.Function;VirtualProtect(&iat->u1.Function,8,PAGE_READWRITE,&prot);iat->u1.Function=(ULONGLONG)replacement;VirtualProtect(&iat->u1.Function,8,prot,&prot);return 1;}
  }
 }
 return 0;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved){
 (void)reserved;
 if(reason==DLL_PROCESS_ATTACH){
  DisableThreadLibraryCalls(module);base=(uintptr_t)GetModuleHandleA(NULL);
  logfile=fopen("weapon-wheel.log","w");
  if(getenv("GE32_STAND_TEST")){testJumpInput();testCombatInput();ExitProcess(0);}
  HMODULE sdl=GetModuleHandleA("SDL2.dll");
  getSize=(void*)GetProcAddress(sdl,"SDL_GL_GetDrawableSize");
  int g=installGameHook();
  int j=installJumpHook();
  int d=installDropHook();
  int a=replaceImport("SDL_GameControllerGetButton",buttonHook,(void**)&getButton);
  int b=replaceImport("SDL_GameControllerGetAxis",axisHook,(void**)&getAxis);
  int c=replaceImport("SDL_GL_SwapWindow",swapHook,(void**)&swapWindow);
  if(logfile){fprintf(logfile,"Weapon wheel loaded. Hooks game=%d jump=%d drop=%d button=%d axis=%d swap=%d\n",g,j,d,a,b,c);fflush(logfile);}
 }
 return TRUE;
}
