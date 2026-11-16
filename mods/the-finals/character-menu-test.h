/* Native offline menu acceptance. Physical input is replaced only in this
 * opt-in fixture; normal character/menu behavior remains engine-owned. */
static void padTestCapture(void*,const char*);
static int replaceImport(const char*,void*,void**);
static int(*charRealPoll)(void*);static void*charRealKeys;
static const unsigned char*charKeys(int*length){static unsigned char keys[512];if(length)*length=512;return keys;}
static int charPoll(void*event){for(int i=0;i<2048;i++){int result=charRealPoll(event);if(!result)return 0;unsigned type=*(unsigned*)event;if((type>=0x300&&type<=0x403)||(type>=0x600&&type<=0x652))continue;return result;}return 0;}
static volatile LONG charButtons[32],charStick[32];
static uintptr_t charBackLo,charBackHi;
static void*charWindow;
static int charFrames,charPhase,charWait,charIndex,charPlayers,charChecks;
#define CHAR_ARRAY(rva) ((int*)(base+(rva)))
#define CHAR_CUR CHAR_ARRAY(3347936)
#define CHAR_PREV CHAR_ARRAY(3347808)
#define CHAR_OFFSET CHAR_ARRAY(3347680)
#define CHAR_READY CHAR_ARRAY(3347552)
#define CHAR_SIZE CHAR_ARRAY(3347424)
static void charCheck(int ok,const char*why){if(ok){charChecks++;return;}fprintf(logfile,"FAIL character phase=%d wait=%d index=%d: %s menu=%d cur=%d prev=%d offset=%d\n",charPhase,charWait,charIndex,why,*(int*)(base+2320832),CHAR_CUR[0],CHAR_PREV[0],CHAR_OFFSET[0]);fflush(logfile);if(charWindow)padTestCapture(charWindow,"character-failure.bmp");ExitProcess(2);}
__attribute__((noinline)) static unsigned short charPressed(int slot,unsigned short mask){
 uintptr_t caller=(uintptr_t)__builtin_return_address(0);if(slot<0||slot>=32||!((caller>=base+0x5b5b0&&caller<base+0x5bb20)||(caller>=charBackLo&&caller<charBackHi)))return 0;
 LONG bits=InterlockedCompareExchange(&charButtons[slot],0,0)&mask;if(bits)InterlockedAnd(&charButtons[slot],~bits);return (unsigned short)bits;
}
static signed char charGetStick(int slot,int low,int high){(void)low;(void)high;return slot>=0&&slot<32?(signed char)InterlockedCompareExchange(&charStick[slot],0,0):0;}
static int charTestPatch(uintptr_t rva,void*fn,const unsigned char expected[14]){unsigned char*target=(void*)(base+rva);if(memcmp(target,expected,14))return 0;unsigned char code[14]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0,0x90,0x90};*(uintptr_t*)(code+2)=(uintptr_t)fn;DWORD prior;if(!VirtualProtect(target,14,PAGE_EXECUTE_READWRITE,&prior))return 0;memcpy(target,code,14);VirtualProtect(target,14,prior,&prior);FlushInstructionCache(GetCurrentProcess(),target,14);return 1;}
static int installCharacterTest(void){
 if(!getenv("GE32_CHARACTER_TEST"))return 0;
 IMAGE_DOS_HEADER*d=(void*)base;IMAGE_NT_HEADERS64*n=(void*)(base+d->e_lfanew);IMAGE_SECTION_HEADER*s=IMAGE_FIRST_SECTION(n);for(int i=0;i<n->FileHeader.NumberOfSections;i++)if(!memcmp(s[i].Name,".bback",6)){charBackLo=base+s[i].VirtualAddress;charBackHi=charBackLo+s[i].Misc.VirtualSize;}
 unsigned char buttons[]={0x48,0x8b,0x05,0x31,0x85,0x0d,0,0x44,0x8b,0x80,0x50,0x0f,0,0},stick[]={0x48,0x8b,0x05,0x31,0x84,0x0d,0,0x44,0x8b,0x88,0x50,0x0f,0,0};
 return charTestPatch(0x19f790,(void*)charPressed,buttons)&&charTestPatch(0x19f890,(void*)charGetStick,stick)&&charBackLo!=0&&replaceImport("SDL_PollEvent",(void*)charPoll,(void**)&charRealPoll)&&replaceImport("SDL_GetKeyboardState",(void*)charKeys,&charRealKeys);
}
static int charSettled(int slot){return CHAR_CUR[slot]==CHAR_PREV[slot]&&CHAR_OFFSET[slot]==0;}
static void charPulse(int slot,int bits){InterlockedExchange(&charButtons[slot],bits);}
static void charNext(int phase){charPhase=phase;charWait=0;}
static void charEnter(void){
 memset((void*)charButtons,0,sizeof(charButtons));memset((void*)charStick,0,sizeof(charStick));*(int*)(base+2320820)=1;*(int*)(base+3346328)=charPlayers;*(int*)(base+3346540)=0;*(int*)(base+2317920)=64;for(int i=0;i<4;i++)CHAR_ARRAY(2316864)[i]=i;FN(void(*)(int,int),0x63e40)(15,0);
}
/* Check white portrait borders near all four corners. Paper/background cannot
 * satisfy these probes when depth clipping removes the upper portrait. */
static void charVisual(int slot,int expanded){
 int w,h;getSize(charWindow,&w,&h);float scale=fminf(w/440.f,h/330.f),left=(w-440*scale)*.5f,top=(h-330*scale)*.5f;
 float panelX=charPlayers==2?38.f:38.f+(slot&1)*175.f,panelW=charPlayers==2?350.f:175.f,panelY=30.f+(slot>=(charPlayers==2?1:2)?140.f:0.f);
 float cx=left+(panelX+panelW*.5f)*scale,cy=top+(panelY+70-expanded)*scale,hw=(35+expanded)*scale,hh=(42+expanded)*scale;int tolerance=(int)ceilf(scale*4)+2;
 int x0=(int)floorf(cx-hw)-tolerance,y0=(int)floorf(cy-hh)-tolerance,x1=(int)ceilf(cx+hw)+tolerance,y1=(int)ceilf(cy+hh)+tolerance;
 charCheck(x0>=0&&y0>=0&&x1<w&&y1<h,"portrait bounds remain inside framebuffer");int width=x1-x0+1,height=y1-y0+1,stride=(width*3+3)&~3;unsigned char*p=calloc((size_t)stride,height);charCheck(p!=NULL,"portrait readback allocation");
 glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);glPixelStorei(GL_PACK_ALIGNMENT,4);glPixelStorei(GL_PACK_ROW_LENGTH,0);glPixelStorei(GL_PACK_SKIP_ROWS,0);glPixelStorei(GL_PACK_SKIP_PIXELS,0);glReadPixels(x0,h-y1-1,width,height,GL_RGB,GL_UNSIGNED_BYTE,p);glPopClientAttrib();charCheck(glGetError()==GL_NO_ERROR,"portrait readback has no GL error");
 int hits=0;for(int side=-1;side<=1;side+=2)for(int sample=0;sample<3;sample++){int sx=(int)lroundf(cx+side*(hw-1)),sy=(int)lroundf(cy+hh*(sample-1)*.65f),found=0;for(int dy=-tolerance;dy<=tolerance&&!found;dy++)for(int dx=-tolerance;dx<=tolerance;dx++){int x=sx+dx-x0,y=y1-(sy+dy);if(x<0||x>=width||y<0||y>=height)continue;unsigned char*q=p+y*stride+x*3;if(q[0]>210&&q[1]>210&&q[2]>210&&abs(q[0]-q[1])<12&&abs(q[1]-q[2])<12){found=1;break;}}hits+=found;}
 free(p);if(hits!=6){fprintf(logfile,"character pixels slot=%d expanded=%d border probes=%d/6 bounds=%d,%d..%d,%d\n",slot,expanded,hits,x0,y0,x1,y1);fflush(logfile);}charCheck(hits==6,"all portrait border heights are visible, including the head");
}
static void charCapture(void*w,const char*name){if(getenv("GE32_CHARACTER_VISUAL"))padTestCapture(w,name);}
static void characterTestRender(void*w){
 if(!getenv("GE32_CHARACTER_TEST"))return;charWindow=w;charFrames++;charWait++;charCheck(charFrames<2400,"menu test deadline");
 if(charFrames==60){charPlayers=getenv("GE32_CHARACTER_PLAYERS")?atoi(getenv("GE32_CHARACTER_PLAYERS")):2;charCheck(charPlayers>=2&&charPlayers<=4,"supported local player count");charEnter();charNext(0);}
 if(charFrames<60)return;
 if(charPhase==0&&charWait==120){charCheck(*(int*)(base+2320832)==15,"native character menu entered");for(int i=0;i<charPlayers;i++)charVisual(i,0);charCapture(w,"character-menu.bmp");if(getenv("GE32_CHARACTER_CAPTURE_ONLY"))ExitProcess(0);charPulse(0,0x4000);charNext(1);}
 else if(charPhase==1&&charWait>=8){charCheck(*(int*)(base+2320832)==14,"B backs out before selection");charCapture(w,"character-back-options.bmp");charEnter();charNext(2);}
 else if(charPhase==2&&charWait==40){InterlockedExchange(&charStick[0],80);charNext(3);}
 else if(charPhase==3&&CHAR_CUR[0]==1){InterlockedExchange(&charStick[0],0);charNext(4);}
 else if(charPhase==4&&charWait>=10&&charSettled(0)){charCheck(CHAR_CUR[0]==1,"left analog scroll advances once");charPulse(0,0x200);charNext(5);}
 else if(charPhase==5&&charWait>=10&&charSettled(0)){charCheck(CHAR_CUR[0]==0,"left button returns to first character");charPulse(0,0x200);charNext(6);}
 else if(charPhase==6&&charWait>=8){charCheck(CHAR_CUR[0]==0,"first-character boundary does not underflow");charIndex=0;charNext(7);}
 else if(charPhase==7&&charWait>=8&&charSettled(0)){
  charCheck(CHAR_CUR[0]==charIndex,"native scrolling reaches expected roster entry");charVisual(0,0);if(charIndex==32)charCapture(w,"character-middle.bmp");if(charIndex<63){charPulse(0,0x100);charIndex++;charWait=0;}else{charCapture(w,"character-last.bmp");charPulse(0,0x100);charNext(8);}
 }
 else if(charPhase==8&&charWait>=8){charCheck(CHAR_CUR[0]==63,"last-character boundary does not overflow");charPulse(0,0x200);charNext(9);}
 else if(charPhase==9&&charWait>=10&&charSettled(0)){charCheck(CHAR_CUR[0]==62,"reverse scroll works at end of roster");charPulse(0,0x8000);charNext(10);}
 else if(charPhase==10&&charWait>=20){charCheck(CHAR_READY[0]&&CHAR_SIZE[0]==11,"A selects and finishes enlargement");charVisual(0,11);charCapture(w,"character-selected.bmp");charPulse(0,0x4000);charNext(11);}
 else if(charPhase==11&&charWait>=8){charCheck(*(int*)(base+2320832)==14,"B backs out after enlargement");charCapture(w,"character-selected-back-options.bmp");charEnter();charNext(12);}
 else if(charPhase==12&&charWait==40){charPulse(1,0x8000);charNext(13);}
 else if(charPhase==13&&charWait>=20){charCheck(CHAR_READY[1]&&!CHAR_READY[0],"second player selects independently");charPulse(1,0x4000);charNext(14);}
 else if(charPhase==14&&charWait>=20){charCheck(!CHAR_READY[1]&&!CHAR_READY[0]&&*(int*)(base+2320832)==15,"second-player B cancels only that selection");charPulse(0,0x8000);charNext(15);}
 else if(charPhase==15&&charWait>=20){charCheck(CHAR_READY[0],"first player selected for duplicate check");CHAR_CUR[1]=CHAR_PREV[1]=CHAR_OFFSET[1]=0;charPulse(1,0x8000);charNext(16);}
 else if(charPhase==16&&charWait>=10){charCheck(!CHAR_READY[1]&&CHAR_READY[0],"duplicate character rejected without changing first player");charPulse(1,0x100);charNext(17);}
 else if(charPhase==17&&charWait>=10&&charSettled(1)){charCheck(CHAR_CUR[1]==1,"second-player navigation remains independent");for(int i=1;i<charPlayers;i++)charPulse(i,0x8000);charNext(18);}
 else if(charPhase==18&&charWait>=30){charCheck(*(int*)(base+2320832)==14,"all ready returns to multiplayer options");for(int i=0;i<charPlayers;i++)charCheck(CHAR_ARRAY(2316864)[i]==i,"chosen character persists for each player");charCapture(w,"character-ready-options.bmp");int width,height;getSize(w,&width,&height);fprintf(logfile,"PASS CHARACTER MENU players=%d portraits=64 checks=%d frames=%d size=%dx%d\n",charPlayers,charChecks,charFrames,width,height);fflush(logfile);ExitProcess(0);}
}
