/* Offline Facility mission fixture. Input and physics use the normal game hooks. */
static struct {int index,tick,began,landed,count,ammo,shot;float initial,peak;} mantleTest;
#include "complex-mantle-test.h"
#include "mantle-route-test.h"
static void mantleTestBefore(unsigned short *buttons){
 if(!getenv("GE32_MANTLE_TEST")||!jumpAllowed())return;
 if(getenv("GE32_MANTLE_ROUTES")){routeBefore(buttons);return;}
 if(complexMantleTestEnabled()){complexBefore(buttons);return;}
 int tick=++mantleTest.tick;

 int index=mantleTest.index;
 *buttons=0;
 if(tick==30){
  float z=-5570.13f-(index==2?27:0);
  if(index==4)padTestPlacePlayer(-383.68f,1555.85f,-415,90);
  else padTestPlacePlayer(7303.59f,z,190,index==3?86:0);
  PF(396)=PF(416)=PF(420)=PF(424)=PF(432)=PF(440)=0;
  jumpCooldown=0;mantleStop();mantle.chose=mantle.found=mantle.clear=0;
  mantleTest.count=jumpCount;mantleTest.shot=0;
  if(index==0){FN(int(*)(int),0x1000+44224)(5);swapNow(0,5);HI(0,44)=7;mantleTest.ammo=7;}
  mantleTest.initial=mantleTest.peak=PF(140);mantleTest.began=mantleTest.landed=0;
 }
 if(tick<30)return;
 /* The Facility ceiling over this step is real. Extend the standing body
  * for this case to leave headroom below the lip but too little on top. */
 if(index==5&&tick<=105)PF(13940)=365;
 if(tick>=32&&tick<=(index==3?39:105))*buttons=8;
 if(index==0&&mantleTest.began&&tick<=96)*buttons|=0x2000;
 volatile LONG *latch=(void*)(base+0x303000+1983488);
 if(tick==32)jumpA(1,1,latch);
 if(tick>32&&tick<=105)jumpA(index==1,1,latch);
 if(tick==106)jumpA(0,1,latch);
}
static void mantleTestAfter(void){
 if(getenv("GE32_MANTLE_ROUTES")){routeAfter();return;}
 if(complexMantleTestEnabled()){complexAfter();return;}
 if(!getenv("GE32_MANTLE_TEST")||mantleTest.tick<30||!jumpAllowed())return;
 int i=mantleTest.index,t=mantleTest.tick;
 mantleTest.peak=fmaxf(mantleTest.peak,PF(140));
 if((mantle.active&&!mantleTest.began)||(mantleTest.began&&!mantle.active&&!mantleTest.landed)||t==32||t==110){
  fprintf(logfile,"case %d step %d %s foot %.1f floor %.1f x %.1f z %.1f\n",i,t,mantleDebugLine,PF(140),PF(144),PF(1864),PF(1872));fflush(logfile);
 }
 if(mantleTest.began&&!mantle.active)mantleTest.landed=1;
 if(i==0&&mantle.active&&HI(0,44)<mantleTest.ammo){fprintf(logfile,"FAIL shot during climb\n");fflush(logfile);ExitProcess(2);}
 if(i==0&&mantleTest.landed&&HI(0,44)<mantleTest.ammo&&!mantleTest.shot){mantleTest.shot=1;fprintf(logfile,"first post-mantle shot step %d\n",t);}
 mantleTest.began|=mantle.active;
 if(t!=110)return;
 int ok=i<3?(mantleTest.landed&&mantle.clear&&fabsf(PF(140)-266.06f)<5):
 (!mantleTest.began&&mantleTest.peak>mantleTest.initial+4);
 if(i==5)ok=ok&&mantle.found&&!mantle.clear;
 ok=ok&&jumpCount==mantleTest.count+1&&(i!=0||mantleTest.shot);
 if(!ok){fprintf(logfile,"FAIL mantle case %d peak %.1f\n",i,mantleTest.peak);fflush(logfile);ExitProcess(2);}
 if(++mantleTest.index==6){fprintf(logfile,"PASS mantle Facility cases\n");fflush(logfile);ExitProcess(0);}
 mantleTest.tick=0;
}
