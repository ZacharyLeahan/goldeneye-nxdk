#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define GRENADE_ITEM 26
static uintptr_t base;
static void *pad;
static volatile LONG openWheel;
static int previousLB,choice,cancelled,wheelStickEngaged,ids[32],count;
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
typedef struct {int ids[32],count,current,local,valid,allowJump,grenades,aiming;uintptr_t context;DWORD tickThread;} InventorySnapshot;
static InventorySnapshot published;
static SRWLOCK snapshotLock=SRWLOCK_INIT;
static volatile LONG pendingWeapon=-1,gameTestsReady,testAction;
static DWORD renderThread;
static void (*originalTick)(signed char,signed char,unsigned short,unsigned short);
static int localIndex(void){int n=FN(int(*)(void),0x1000+936896)();return n>=0&&n<32?n:0;}
static InventorySnapshot snapshotRead(void){InventorySnapshot r;AcquireSRWLockShared(&snapshotLock);r=published;ReleaseSRWLockShared(&snapshotLock);return r;}
static int playing(void){int m=*(int*)(base+0x228000+59840);InventorySnapshot s=snapshotRead();return (m==11||m==-1)&&s.valid;}
static int available(int id){InventorySnapshot s=snapshotRead();if(id==GRENADE_ITEM&&s.grenades<=0)return 0;for(int j=0;j<s.count;j++)if(s.ids[j]==id)return 1;return 0;}
static int current(void){return snapshotRead().current;}
static void equip(int id){InterlockedExchange(&pendingWeapon,id);if(logfile){fprintf(logfile,"queue weapon=%d\n",id);fflush(logfile);}}
static void publishInventory(int owner,const InventorySnapshot*s){if(owner!=localIndex())return;AcquireSRWLockExclusive(&snapshotLock);published=*s;ReleaseSRWLockExclusive(&snapshotLock);}
#include "jump.h"
#include "combat.h"
#include "combat-test.h"
#include "ledge-drop.h"
#include "ledge-drop-test.h"
#include "jump-pad.h"
#include "airborne-navigation.h"
#include "grenades.h"
#include "slide.h"
#include "precision-aim.h"
#include "jump-pad-test.h"
#include "temple-movement-test.h"
#include "health-bars.h"
#include "health-bars-test.h"
#include "grenades-test.h"
#include "movement-test.h"
#include "precision-aim-test.h"
#include "ads-movement-test.h"
#include "airborne-opening-test.h"
#include "wall-escape-test.h"
static void gameTickHook(signed char x,signed char y,unsigned short buttons,unsigned short oldbuttons){
 if(getenv("GE32_JUMP_MOVING_TEST")&&NUMBER==localIndex()){x=0;y=0;buttons=(buttons&~15)|jumpTestMotion;}
 int isLocal=NUMBER==localIndex()&&CURRENT&&CURRENT==PLAYERS[localIndex()];
 if(isLocal&&getenv("GE32_REGRESSION_PRACTICE")&&!getenv("GE32_PAD_LIVE_TEST")&&jumpAllowed())*(int*)(base+0x303000+618564)=1;
 if(isLocal&&getenv("GE32_DROP_TEST")){x=0;y=0;}
 if(isLocal){padTestBefore(&x,&y,&buttons);templeStressBefore(&x,&y,&buttons);}
 if(isLocal){combatTestBefore(&buttons);dropTestBefore(&buttons);}
 if(isLocal){wallEscapeBefore(&x,&y,&buttons);openingBefore(&x,&y,&buttons);movementTestBefore(&x,&y,&buttons);precisionAimTestBefore(&x,&y,&buttons);}
 unsigned short rawButtons=buttons;
 if(isLocal){healthBegin();if(getenv("GE32_HEALTH_TEST"))x=y=0;healthTestBefore(&buttons);}
 if(isLocal){grenadeLoadout();grenadeTestBefore(&x,&y,&buttons);}
 if(isLocal)adsMovementBefore(&x,&y,&buttons);
 if(isLocal){padBefore(&buttons,&oldbuttons);combatBefore(&buttons,&oldbuttons);slideBefore();}
 int aimMove=isLocal&&aimMoveReady(buttons),stance=0,autoStance=0;float aimPushForward=0,aimPushSide=0;if(aimMove){aimPushForward=-PF(432)*PF(392)+PF(440)*PF(388);aimPushSide=-PF(432)*PF(388)-PF(440)*PF(392);stance=PI(184);autoStance=PI(14004);buttons&=~15;oldbuttons&=~15;}
 originalTick(x,y,buttons,oldbuttons);
 if(aimMove&&PI(332)){PI(184)=stance;PI(14004)=autoStance;aimMoveAfter();PF(424)+=aimPushForward;PF(416)+=aimPushSide;}
 if(isLocal&&getenv("GE32_GRENADE_TEST")&&jumpAllowed())FN(void(*)(int),0x74290)(grenadeTestFire);
 if(isLocal&&getenv("GE32_HEALTH_TEST")&&jumpAllowed())FN(void(*)(int),0x74290)(healthTestFire);
 if(isLocal){combatAfter(rawButtons);slideAfter();precisionAimTestFeed();precisionAimTick();precisionAimTestAfter();combatTestAfter(rawButtons);dropTestAfter();padTestAfter();templeStressAfter();}
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
 if(wanted==PAD_ITEM&&!combatDeferWeapon(wanted))padEquip();
 if(wanted>=4&&wanted<=GRENADE_ITEM&&(wanted!=GRENADE_ITEM||grenadeAmmo()>0)&&!combatDeferWeapon(wanted)&&FN(int(*)(int),0x1000+43904)(wanted)){
  padStow();
  if(logfile){fprintf(logfile,"game accepted weapon=%d\n",wanted);fflush(logfile);}
  FN(void(*)(int,int,int),0x1000+420240)(0,wanted,1);
  FN(void(*)(int,int,int),0x1000+420240)(1,0,1);
 }
 InventorySnapshot next={0};next.current=padCarrying?PAD_ITEM:FN(int(*)(int),0x1000+421920)(0);next.local=n;next.context=(uintptr_t)CURRENT;next.aiming=PI(332)!=0;next.valid=1;next.allowJump=jumpAllowed();next.tickThread=GetCurrentThreadId();
 for(int id=4;id<=25;id++){if(id!=23&&FN(int(*)(int),0x1000+43904)(id))next.ids[next.count++]=id;}
 next.grenades=grenadeAmmo();next.ids[next.count++]=GRENADE_ITEM;
 next.ids[next.count++]=PAD_ITEM;publishInventory(n,&next);padPublish();padTestPublishActions();healthPublish();healthTestAfter();
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
static const char *names[]={"Unarmed","Fist","Knife","Throwing knife","PP7","PP7 silenced","DD44 Dostovei","Klobb","KF7 Soviet","ZMG","D5K Deutsche","D5K silenced","Phantom","AR33","RC-P90","Shotgun","Automatic shotgun","Sniper rifle","Cougar Magnum","Golden Gun","Silver PP7","Gold PP7","Moonraker Laser","Watch Laser","Grenade Launcher","Rocket Launcher","Grenades"};
static const char*itemName(int id){return id==PAD_ITEM?"Jump Pad":id>=0&&id<=GRENADE_ITEM?names[id]:"Equipment";}
#include "wheel-layout.h"
static void inventory(void){InventorySnapshot s=snapshotRead();count=s.count;memcpy(ids,s.ids,sizeof(ids));}
static unsigned char buttonHook(void *p,int b){
 if(!pad)pad=p;
 if(p==pad && playing()){
  if(b==7||b==8)return combatButton(b,getButton(p,b),snapshotRead().allowJump&&!openWheel&&!FN(int(*)(void),0x1000+1453264)());
  if(b==1&&padCarrying&&!openWheel)return 0;
  if(b==9)return 0; /* LB belongs to the wheel, not aim */
  if(openWheel && (b==0||b==1||b==10)){if(b==0)jumpAHeld=getButton(p,b)!=0;return 0;}
  if(b==1)slideButton(getButton(p,b),snapshotRead().allowJump&&!openWheel);
  if(b==0){int slot=*(int*)(base+0x228000+203956);if(slot<0||slot>3)slot=0;return jumpA(getButton(p,b),snapshotRead().allowJump&&!FN(int(*)(void),0x1000+1453264)(),(volatile LONG*)(base+0x303000+1983488+slot*4));}
 }
 return getButton(p,b);
}
static short axisHook(void*p,int a){
 pad=p;
 if(p==pad && playing() && (openWheel||getButton(p,9)) && (a==2||a==3||a==4||a==5)){aimSubmit(0,0,0,0,0);return 0;}
 if(getenv("GE32_AIM_TEST")&&p!=&aimTestDevice&&(a==2||a==3))return 0;
 short raw=getAxis(p,a);if((a==2||a==3)&&precisionAimCapture(p))return 0;return raw;
}
#include "wheel-render.h"
#include "retro-ui.h"
#include "jump-pad-render.h"
#include "health-bars-render.h"
static void wheelUpdate(int lb,int back,float x,float y){
 if(!playing()){openWheel=0;previousLB=lb;cancelled=0;return;}
 if(lb&&!previousLB){
  inventory();oldCurrent=current();choice=-1;
  for(int i=0;i<count;i++)if(ids[i]==oldCurrent)choice=i;
  openWheel=1;cancelled=0;wheelStickEngaged=0;
  if(logfile){fprintf(logfile,"wheel local=%d context=%d owned=%d current=%d\n",snapshotRead().local,-1,count,oldCurrent);fflush(logfile);}
 }
 if(openWheel){
  if(x*x+y*y>.16f*.16f)wheelStickEngaged=1;
  if(back){cancelled=1;openWheel=0;}
  else if(x*x+y*y<.22f*.22f&&wheelStickEngaged)choice=-1;
  else if(x*x+y*y>.28f*.28f&&count){
   wheelStickEngaged=1;
   float angle=atan2f(y,x);int target=wheelTarget(angle);
   if(target<0)choice=-1;
   else if(target!=choice){float start,end;wheelBounds(target,&start,&end);float wrapped=angle;while(wrapped<start)wrapped+=6.28318530718f;while(wrapped>start+6.28318530718f)wrapped-=6.28318530718f;
    if(choice<0||(wrapped>start+.06f&&wrapped<end-.06f))choice=target;
   }
  }
 }
 if(!lb&&previousLB){
  if(openWheel&&!cancelled&&choice>=0&&choice<count&&available(ids[choice])&&wheelStickEngaged&&ids[choice]!=current())equip(ids[choice]);
  openWheel=0;cancelled=0;
 }
 previousLB=lb;
}
static void testAssert(int value,const char*message){if(!value){if(logfile){fprintf(logfile,"FAIL: %s\n",message);fflush(logfile);}ExitProcess(2);}}
static void padTestCapture(void*w,const char*name){
 int width,height;getSize(w,&width,&height);if(width<1||height<1||width>8192||height>8192)return;
 int stride=(width*3+3)&~3;unsigned char*p=calloc((size_t)stride,height);if(!p)return;
 glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);glPixelStorei(GL_PACK_ALIGNMENT,4);glPixelStorei(GL_PACK_ROW_LENGTH,0);glPixelStorei(GL_PACK_SKIP_ROWS,0);glPixelStorei(GL_PACK_SKIP_PIXELS,0);glReadPixels(0,0,width,height,0x80e0,GL_UNSIGNED_BYTE,p);glPopClientAttrib();
 BITMAPFILEHEADER file={0};BITMAPINFOHEADER info={0};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(info);file.bfSize=file.bfOffBits+stride*height;info.biSize=sizeof(info);info.biWidth=width;info.biHeight=height;info.biPlanes=1;info.biBitCount=24;
 FILE*f=fopen(name,"wb");if(f){fwrite(&file,sizeof(file),1,f);fwrite(&info,sizeof(info),1,f);fwrite(p,stride,height,f);fclose(f);}free(p);
}
static void healthTestRender(void*w){
 if(!getenv("GE32_HEALTH_TEST")||!useProgram||!activeTexture)return;testAssert(glGetError()==GL_NO_ERROR,"health bars render without GL errors");int capture=InterlockedExchange(&healthTestCapture,0);if(capture&&getenv("GE32_HEALTH_VISUAL_TEST"))padTestCapture(w,capture==1?"health-hit.bmp":"health-fade.bmp");
}
static void padTestRenderTick(void*w){
 if(!getenv("GE32_PAD_TEST"))return;
 int action=InterlockedExchange(&padTestRenderAction,0);static int rtFrames;
 if(action==4){InterlockedExchange(&padCancelRequest,1);wheelUpdate(1,0,0,0);}
 if(action==1||action==3||action==5){wheelUpdate(1,0,0,0);int index=-1;for(int i=0;i<count;i++)if(ids[i]==(action==3?5:PAD_ITEM))index=i;testAssert(index>=0,"jump pad available in equipment wheel");float a=wheelAngle(index);wheelUpdate(1,0,cosf(a),sinf(a));if(getenv("GE32_PAD_VISUAL_TEST")&&useProgram&&activeTexture){draw(w);padTestCapture(w,"pad-wheel.bmp");}wheelUpdate(0,0,cosf(a),sinf(a));}
 if(action==2){padTrigger(0,1);padTrigger(1,1);rtFrames=20;}
 if(rtFrames){padTrigger(1,1);if(!--rtFrames)padTrigger(0,1);}
 if(useProgram&&activeTexture){while(glGetError()!=GL_NO_ERROR){}drawPad(w);if(getenv("GE32_PAD_VISUAL_TEST")){static int captured;if(!captured&&padTestPhase==2&&padTestWait>60){padTestCapture(w,"pad-carry.bmp");captured=1;}if(captured==1&&padTestPhase==4&&padSnapshot().visible){padTestCapture(w,"pad-placed.bmp");captured=2;}}testAssert(glGetError()==GL_NO_ERROR,"jump-pad preview/world/HUD render without GL errors");}
}
static void testWheelCenter(void){
 InventorySnapshot s=snapshotRead();int index=0;for(int i=0;i<s.count;i++)if(s.ids[i]!=s.current){index=i;break;}
 float angle=wheelAngle(index),x=cosf(angle),y=sinf(angle);
 wheelUpdate(1,0,0,0);testAssert(choice>=0&&ids[choice]==s.current,"opening highlights equipped item");wheelUpdate(1,0,0,0);wheelUpdate(0,0,0,0);testAssert(pendingWeapon==-1,"opening/releasing current item does not re-equip");
 wheelUpdate(1,0,0,0);wheelUpdate(1,0,.18f,0);testAssert(choice==-1,"gentle nudge reaches neutral without another item");wheelUpdate(0,0,0,0);testAssert(pendingWeapon==-1,"gentle-neutral release keeps current item");
 wheelUpdate(1,0,0,0);wheelUpdate(1,0,x,y);testAssert(choice==index,"outer sector selects item");wheelUpdate(1,0,.08f,-.08f);testAssert(choice==-1,"center clears sector selection");wheelUpdate(0,0,0,0);testAssert(pendingWeapon==-1&&current()==s.current,"center release preserves equipment");
 wheelUpdate(1,0,0,0);wheelUpdate(1,0,x,y);wheelUpdate(0,0,0,0);testAssert(pendingWeapon==-1,"same-frame recenter/release cancels");
 wheelUpdate(1,0,0,0);wheelUpdate(1,0,x,y);wheelUpdate(1,0,0,0);wheelUpdate(1,0,x,y);testAssert(choice==index,"selection rearms after leaving center");wheelUpdate(1,1,x,y);wheelUpdate(0,0,0,0);
 if(logfile){fprintf(logfile,"PASS wheel neutral: equipped highlight, no redundant equip, select-center-release, same-frame cancellation, reselect and B cancel\n");fflush(logfile);}
}
static int testTick(void*w){
 static int frame,completed,initialWeapon;
 if(!getenv("GE32_WHEEL_TEST")||!gameTestsReady||!playing())return 0;
 InventorySnapshot s=snapshotRead();
 testAssert(s.tickThread!=GetCurrentThreadId(),"gameplay and drawing threads are separate");
 if(frame==0&&s.current!=5&&s.current!=8)return 1;
 if(++frame==1){initialWeapon=s.current;testAssert(s.count==4,"initial snapshot contains two guns, grenades and jump pad");if(logfile)fprintf(logfile,"TEST: game thread=%lu render thread=%lu local=%d owned=2\n",s.tickThread,GetCurrentThreadId(),s.local);}
 int cycle=(frame-1)/120,phase=(frame-1)%120;
 if(cycle<20){
  int desired=cycle%2?initialWeapon:(initialWeapon==5?8:5),index=0;for(int i=0;i<s.count;i++)if(s.ids[i]==desired)index=i;float angle=wheelAngle(index);float tx=cosf(angle),ty=sinf(angle);
  if(phase==0){wheelUpdate(1,0,0,0);testAssert(count==4,"reopened equipment wheel retains two guns, grenades and pad");}
  else if(phase<5)wheelUpdate(1,0,tx,ty);
  else wheelUpdate(0,0,tx,ty);
  if(phase==119){testAssert(current()==desired,"game thread equipped alternating requested weapon");completed++;}
  if(openWheel&&useProgram&&activeTexture){while(glGetError()!=GL_NO_ERROR){}draw(w);testAssert(glGetError()==GL_NO_ERROR,"wheel renders without GL errors");}
 }else{
  int end=(frame-1)-2400;
  if(end==0){
   testWheelCenter();
   for(int i=0;i<100;i++){wheelUpdate(1,0,0,0);testAssert(count==4,"100 rapid opens retain equipment ownership");wheelUpdate(1,1,0,0);wheelUpdate(0,0,0,0);}
   InventorySnapshot remote={0};publishInventory(31,&remote);testAssert(snapshotRead().count==4,"foreign player cannot overwrite local inventory snapshot");
   int before=current();wheelUpdate(1,0,0,1);wheelUpdate(1,1,0,1);wheelUpdate(0,0,0,0);testAssert(pendingWeapon==-1&&current()==before,"B cancels without enqueueing a switch");InterlockedExchange(&testAction,1);
  }
  if(end==30){wheelUpdate(1,0,0,0);testAssert(count==5,"pickup added on game thread appears");wheelUpdate(1,1,0,0);wheelUpdate(0,0,0,0);InterlockedExchange(&testAction,2);}
  if(end==60){wheelUpdate(1,0,0,0);testAssert(count==4,"removed gun does not persist as ghost entry");wheelUpdate(1,1,0,0);wheelUpdate(0,0,0,0);InterlockedExchange(&testAction,3);}
  if(end==90){wheelUpdate(1,0,0,0);testAssert(count==2&&ids[1]==PAD_ITEM,"pad remains selectable after all guns removed");testAssert(choice==-1,"opening with no represented equipped item stays neutral");if(useProgram&&activeTexture){while(glGetError()!=GL_NO_ERROR){}draw(w);testAssert(glGetError()==GL_NO_ERROR,"empty state renders without GL errors");}
   if(logfile){fprintf(logfile,"PASS: %d alternating switches, 100 rapid reopen/cancels, foreign-player isolation, pickup/removal, pad retained with no guns, GL rendering, separate game/render threads\n",completed);fflush(logfile);}ExitProcess(0);
  }
 }
 return 1;
}
static void swapHook(void*w){
 renderThread=GetCurrentThreadId();
 if(!useProgram){useProgram=(void*)wglGetProcAddress("glUseProgram");activeTexture=(void*)wglGetProcAddress("glActiveTexture");}
 dropTestLaunch();
 if(getenv("GE32_GRENADE_TEST")){int selected=InterlockedExchange(&grenadeTestSelect,0);if(selected){wheelUpdate(1,0,0,0);int index=-1;for(int i=0;i<count;i++)if(ids[i]==selected)index=i;testAssert(index>=0,"equipment present in wheel");float angle=wheelAngle(index),x=cosf(angle),y=sinf(angle);wheelUpdate(1,0,x,y);testAssert(choice>=0&&ids[choice]==selected,"fixed directional slot selects requested gadget");wheelUpdate(0,0,x,y);}}
 padTestRenderTick(w);if(!getenv("GE32_PAD_TEST"))padPoll();
 if(testTick(w)){swapWindow(w);return;}
 int lb=pad?getButton(pad,9):0,back=pad?getButton(pad,1):0;
 float x=pad?getAxis(pad,2)/32767.f:0,y=pad?getAxis(pad,3)/32767.f:0;
 wheelUpdate(lb,back,x,y);
 if(useProgram&&activeTexture){drawPad(w);drawHealth(w);if(openWheel)draw(w);}
 healthTestRender(w);
 if(InterlockedExchange(&openingCapture,0))padTestCapture(w,"airborne-opening.bmp");
 if(getenv("GE32_MOVEMENT_VISUAL_TEST")&&InterlockedExchange(&movementCapture,0))padTestCapture(w,"movement-slide.bmp");
 if(getenv("GE32_TEMPLE_VISUAL_TEST")&&InterlockedExchange(&templeFrameCapture,0))padTestCapture(w,"temple-camera-transition.bmp");
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
  if(getenv("GE32_STAND_TEST")){testJumpInput();testCombatInput();testPadInput();testSlideInput();testPrecisionAim();ExitProcess(0);}
  HMODULE sdl=GetModuleHandleA("SDL2.dll");
  getSize=(void*)GetProcAddress(sdl,"SDL_GL_GetDrawableSize");
  int g=installGameHook();
  int j=installJumpHook();
  int d=installDropHook();
  int p=installPadGunHook();
  int portal=installPortalHook();
  int h=installHealthHook();
  int a=replaceImport("SDL_GameControllerGetButton",buttonHook,(void**)&getButton);
  int b=replaceImport("SDL_GameControllerGetAxis",axisHook,(void**)&getAxis);
  int c=replaceImport("SDL_GL_SwapWindow",swapHook,(void**)&swapWindow);
  if(logfile){fprintf(logfile,"Weapon wheel loaded. Hooks game=%d jump=%d drop=%d gadget=%d health=%d button=%d axis=%d swap=%d portal=%d revision=finals-motion-v3\n",g,j,d,p,h,a,b,c,portal);fflush(logfile);}
 }
 return TRUE;
}
