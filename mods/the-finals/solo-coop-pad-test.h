/* Isolated Windows acceptance build. A single virtual SDL controller selects
 * Start alone, chooses Bond and walks and jumps twice across two sessions. */

static void coopPadTrial(void *w){
 static void *js;static int frame,play,clicked,quit,backFrames,completed;static float startX,startZ,moved;
 HMODULE sdl=GetModuleHandleA("SDL2.dll");
 int (*button)(void*,int,unsigned char)=(void*)GetProcAddress(sdl,"SDL_JoystickSetVirtualButton");
 int (*axis)(void*,int,short)=(void*)GetProcAddress(sdl,"SDL_JoystickSetVirtualAxis");
 if(!js){int(*attach)(int,int,int,int)=(void*)GetProcAddress(sdl,"SDL_JoystickAttachVirtual");void*(*open)(int)=(void*)GetProcAddress(sdl,"SDL_JoystickOpen");int i=attach(1,6,21,0);if(i<0||!(js=open(i)))ExitProcess(2);}
 frame++;
 int menu=COOP_INT(0x2369c0),hover=COOP_INT(0x235440),stage=*(int*)(base+0x303000+211452);
 int pressed=0,vertical=0,down=0,up=0;
 if(frame==90)FN(void(*)(int,int),0x13c390)(3,1); /* Native Runway selection. */
 if(frame>160&&!clicked&&menu==26&&soloSetup()){
  if(hover>=0&&hover<COOP_INT(0x4c5f60)&&((int*)(base+0x4c5fc0))[hover]==SOLO_COOP_ROW){if(frame%18<5){pressed=1;padTestCapture(w,"coop-pad-menu.bmp");}}
  else {int direction=hover<4?1:-1;if(frame%30<6){vertical=18000*direction;down=direction==1;up=!down;}}
 }
 if(FN(int(*)(void),0x138760)())clicked=1;
 if(menu==15&&clicked&&!quit)pressed=frame%25<5;
 if(clicked&&!quit&&stage==35&&FN(int(*)(void),0x1023c0)()&&jumpAllowed()){
  if(play==0){startX=PF(1864);startZ=PF(1872);}
  play++;vertical=play<=90?-32767:0;
  if(play==40||play==41)pressed=1;
  float delta=hypotf(startX-PF(1864),startZ-PF(1872));if(delta>moved)moved=delta;
  if(play==150){
   padTestCapture(w,"coop-pad-game.bmp");int roster=FN(int(*)(void),0xfe820)(),players=FN(int(*)(void),0xa4050)(),bots=FN(int(*)(void),0xf51b0)();
   fprintf(logfile,"CONTROLLER PLAY stage=%d difficulty=%d roster=%d players=%d bots=%d movement=%.1f\n",stage,COOP_INT(0x2369a8),roster,players,bots,moved);fflush(logfile);
   if(moved<35||roster!=1||players!=1||bots||COOP_INT(0x2369a8)!=1)ExitProcess(4);
   FN(void(*)(void),0x105d50)();quit=1;
  }
 }
 button(js,0,pressed);button(js,12,down);button(js,11,up);axis(js,1,vertical);
 if(frame%90==0){fprintf(logfile,"PAD MENU t=%d menu=%d row=%d page=%d stage=%d connected=%d active=%d play=%d quit=%d\n",frame,menu,hover,COOP_INT(0x4c616c),stage,FN(int(*)(void),0x1387e0)(),FN(int(*)(void),0x1023c0)(),play,quit);fflush(logfile);}
 if(quit&&stage==90&&soloSetup()&&!FN(int(*)(void),0x1023c0)()&&++backFrames==60){fprintf(logfile,"RETURN menu=%d stage=%d roster=%d\n",menu,stage,FN(int(*)(void),0xfe820)());fflush(logfile);if(FN(int(*)(void),0xfe820)()!=0)ExitProcess(5);
  if(++completed==2)ExitProcess(0);
  play=clicked=quit=backFrames=0;moved=0;}
 if(frame>4300)ExitProcess(3);
}
