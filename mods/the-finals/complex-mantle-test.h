/* Offline Complex practice. Each attempt starts on the real level floor. */
static int complexMantleTestEnabled(void){
 const char *mode=getenv("GE32_MANTLE_TEST");
 return mode&&strcmp(mode,"complex")==0;
}
static void complexMantleLaunch(void){
 static int frames;
 if(!getenv("GE_STARTMP")&&complexMantleTestEnabled()&&frames<60&&++frames==60)FN(void(*)(int,int),0x1000+999872)(2,0);
}
typedef struct {float x,floor,z,yaw,top;int expected,hold;float retryAtZ;} ComplexLedge;
static const ComplexLedge complexLedges[]={
 /* Side of the ramp, then the two opposing walkway edges. */
 {264.61f,171.35f,2340.42f,180,281.06f,1,0},
 {198.61f,130.39f,2340.42f,180,281.06f,1,1},
 {1009.9f,0,125.2f,180,281.06f,1,0},
 {1009.9f,0,703.2f,0,281.06f,1,1},
 /* Raised landing, lower pit rim, and a normal jump on open floor. */
 {2183.1f,0,49.2f,0,281.06f,1,0},
 {-722.6f,-281.1f,1762.4f,90,0,1,1},
 {1009.9f,0,415,180,0,0,0},
 /* Start farther from the walkway. Re-press A after the launch buffer expires. */
 {1009.9f,0,205.2f,180,281.06f,1,0,60},
 /* Late catch with the front of the body just under the lip. */
 {1009.9f,0,185.2f,180,281.06f,1,0,38},
 {1009.9f,0,415,180,0,0,0,275}
};
static struct {int index,tick,grab,land,fail,takeoffs,retry;JumpVec landing;float peak;} complexTest;
/* Replay the pad position from the Complex crash report before testing the ledges.
 * Let the native simulation carry the player through the whole flight and landing. */
static int complexPadFlight(unsigned short *buttons){
 static int ticks,launches,landed;
 static float takeoffY,peak;
 if(landed||!getenv("GE32_COMPLEX_PAD_REPLAY"))return 0;
 *buttons=0;
 if(++ticks==3){
  padTestCheck(*(int*)(base+0x303000+211452)==31,"Complex replay stage");
  padTestPlacePlayer(1178.66f,335.52f,4,180);
  takeoffY=peak=PF(140);
  gadget.position=(JumpVec){PF(1864),takeoffY,PF(1872)};
  gadget.placed=1;gadget.cooldown=0;launches=gadget.launches;
 }
 if(ticks>3){
  peak=fmaxf(peak,PF(140));
  if(gadget.flight&&ticks<34)*buttons=8;
  if(gadget.launches>launches&&!gadget.flight){
   padTestCheck(peak>takeoffY+40&&fabsf(PF(140)-PF(144))<2,"Complex pad flight and supported landing");
   fprintf(logfile,"PASS Complex pad flight peak %.2f landing %.2f\n",peak,PF(140));
   fflush(logfile);landed=1;
  }
 }
 padTestCheck(ticks<480,"Complex pad landing deadline");
 return 1;
}
static void complexBefore(unsigned short *buttons){
 if(!jumpAllowed())return;
 /* Validate the native roster before exercising movement. A hidden bot can
  * otherwise make a supposed zero-bot practice test pass. */
 if(!getenv("GE_STARTMP")&&FN(int(*)(void),0xf51b0)()!=1){
  fprintf(logfile,"FAIL Complex practice expected one player including Bond\n");fflush(logfile);ExitProcess(2);
 }
 if(complexPadFlight(buttons))return;
 int tick=++complexTest.tick;
 const ComplexLedge *c=&complexLedges[complexTest.index];*buttons=0;
 if(tick==30){
  if(*(int*)(base+0x303000+211452)!=31)ExitProcess(2);
  padTestPlacePlayer(c->x,c->z,c->floor+4,c->yaw);
  PF(396)=PF(416)=PF(420)=PF(424)=PF(432)=PF(440)=0;
  jumpCooldown=0;mantleStop();complexTest.grab=complexTest.land=complexTest.retry=0;
  complexTest.takeoffs=jumpCount;complexTest.peak=PF(140);
 }
 if(tick<30)return;
 if(tick>=32&&(!complexTest.grab||mantle.active)&&tick<80)*buttons=8;
 volatile LONG *latch=(void*)(base+0x303000+1983488);
 if(c->retryAtZ&&tick>32&&!complexTest.retry&&jumpActive&&PF(1872)<=c->retryAtZ)complexTest.retry=tick;
 jumpA(tick==32||tick==complexTest.retry||(c->hold&&tick>32&&tick<80),1,latch);
}
static void complexAfter(void){
 int tick=complexTest.tick;if(tick<30||!jumpAllowed())return;
 const ComplexLedge *c=&complexLedges[complexTest.index];
 complexTest.peak=fmaxf(complexTest.peak,PF(140));
 if(mantle.active&&!complexTest.grab){complexTest.grab=tick;complexTest.landing=mantle.land;}
 if(complexTest.grab&&!mantle.active&&!complexTest.land)complexTest.land=tick;
 if(tick!=110)return;
 int supported=fabsf(PF(140)-PF(144))<2&&fabsf(PF(152))<.6f;
 int ok=jumpCount==complexTest.takeoffs+1&&supported;
 if(c->expected)ok=ok&&complexTest.land>complexTest.grab&&mantle.clear&&fabsf(PF(140)-c->top)<3&&hypotf(PF(1864)-complexTest.landing.x,PF(1872)-complexTest.landing.z)<25;
 else ok=ok&&!complexTest.grab&&complexTest.peak>c->floor+10;
 if(c->retryAtZ){
  ok=ok&&complexTest.retry>32+MANTLE_BUFFER;
  if(c->expected)ok=ok&&complexTest.grab>=complexTest.retry&&complexTest.grab<=complexTest.retry+2;
  fprintf(logfile,"Airborne A at step %d, grab %d, takeoffs %d\n",complexTest.retry,complexTest.grab,jumpCount-complexTest.takeoffs);
 }
 fprintf(logfile,"%s Complex case %d rise %.1f grab %d land %d peak %.1f foot %.1f pos %.1f %.1f target %.1f %.1f %s\n",ok?"PASS":"FAIL",complexTest.index,c->top-c->floor,complexTest.grab,complexTest.land,complexTest.peak,PF(140),PF(1864),PF(1872),complexTest.landing.x,complexTest.landing.z,mantleDebugLine);fflush(logfile);
 if(!ok)complexTest.fail++;
 if(++complexTest.index==(int)(sizeof(complexLedges)/sizeof(*complexLedges)))ExitProcess(complexTest.fail?2:0);
 complexTest.tick=0;
}
