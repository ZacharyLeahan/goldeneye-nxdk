/* Offline native-engine speed/slide acceptance. No fixtures in normal play. */
static volatile LONG movementCapture;
static int movementTicks,movementCase,movementSawSlide,movementSawJump,movementStopped;static float movementLastX,movementLastZ,movementDistance,movementTime,movementStartX,movementStartZ;
static void movementCheck(int ok,const char*why){if(!ok){fprintf(logfile,"FAIL movement: %s case=%d tick=%d slide=%d starts=%d crouch=%d foot=%.2f floor=%.2f\n",why,movementCase,movementTicks,slide.active,slide.starts,PI(184),PF(140),PF(144));fflush(logfile);ExitProcess(2);}}
static void movementTestBefore(signed char*x,signed char*y,unsigned short*buttons){
 if(!getenv("GE32_MOVEMENT_TEST")||!jumpAllowed())return;*x=*y=0;*buttons=0;movementTicks++;int t=movementTicks;
 if(t==120){padTestPlacePlayer(-150,100,-460,180);PF(396)=0;PF(416)=PF(420)=PF(424)=0;PF(432)=PF(436)=PF(440)=0;sprintLatched=sprintOn=0;sprintBlend=0;slideStop();slide.cooldown=0;movementDistance=movementTime=0;movementSawSlide=movementSawJump=movementStopped=0;}
 if(t>=121&&t<160)*buttons=movementCase==2?10:8;
 if(movementCase>=1&&movementCase!=2&&t==125)InterlockedIncrement(&sprintClicks);
 if(movementCase>=3&&t==148){PI(184)=0;InterlockedExchange(&slideRequests,1);}
 if(movementCase>=3&&t>=149)*buttons=0;
 if(movementCase==4&&t==162)InterlockedExchange(&pendingJump,1);
 if(movementCase==5&&t==162)openWheel=1;
 if(movementCase==5&&t==164)openWheel=0;
 if(movementCase==6&&t>=160&&t<175){*buttons=4;}
 if(movementCase==7&&t==130){InterlockedExchange(&slideRequests,1);sprintLatched=sprintOn=0;sprintBlend=0;PI(184)=0;}
 if(getenv("GE32_MOVEMENT_VISUAL_TEST")&&movementCase==3&&t==155)InterlockedExchange(&movementCapture,1);
 if(t==140){movementLastX=PF(1864);movementLastZ=PF(1872);}
 if(t>140&&t<=150&&movementCase<3){movementDistance+=hypotf(PF(1864)-movementLastX,PF(1872)-movementLastZ);movementTime+=*(float*)(base+0x303000+211436);movementLastX=PF(1864);movementLastZ=PF(1872);}
 if(t==151&&movementCase<3){float speed=movementDistance*60/movementTime/100;fprintf(logfile,"MEASURE movement case=%d speed=%.3f m/s (100 units/m)\n",movementCase,speed);fflush(logfile);movementCheck(speed>4.0f&&speed<(movementCase==1?7.8f:5.8f),"measured speed bounded");if(movementCase==1)movementCheck(speed>6.5f,"sprint reaches target pace");}
 if(movementCase==8&&t==162){PI(184)=2;InterlockedExchange(&slideRequests,1);}
 if(movementCase==9&&t==162){int dead=PI(256);PI(256)=1;slideBefore();PI(256)=dead;movementCheck(!slide.active,"death clears active slide");}
 if(movementCase==10&&t==140)InterlockedExchange(&pendingJump,1);
 if(t==150&&movementCase>=3&&movementCase<=9&&movementCase!=7){movementCheck(slide.active&&PI(184)==0,"sprint plus crouch starts native crouched slide");movementStartX=PF(1864);movementStartZ=PF(1872);}
 if(slide.active)movementSawSlide=1;if(jumpActive)movementSawJump=1;if(t>=163&&!slide.active)movementStopped=1;
 if(t==205&&movementCase==3){movementCheck(movementSawSlide&&!slide.active&&hypotf(PF(1864)-movementStartX,PF(1872)-movementStartZ)>100,"neutral-stick slide coasts and ends");movementCheck(PI(184)==0,"finished slide leaves crouch");}
 if(t==205&&movementCase==4)movementCheck(movementSawSlide&&movementSawJump&&!slide.active,"A exits slide into native jump");
 if(t==205&&(movementCase==5||movementCase==6))movementCheck(movementSawSlide&&movementStopped&&!slide.active,"wheel or backward input stops slide");
 if(t==205&&movementCase==7)movementCheck(!movementSawSlide,"crouch without sprint does not slide");
 if(t==205&&(movementCase==8||movementCase==9))movementCheck(movementSawSlide&&!slide.active,"B/death cancels slide");
 if(t==205&&movementCase==10)movementCheck(!movementSawSlide&&movementSawJump,"airborne crouch does not queue a landing slide");
 if(t==240){fprintf(logfile,"PASS movement case=%d slideSeen=%d jumped=%d\n",movementCase,movementSawSlide,movementSawJump);fflush(logfile);movementTicks=119;if(++movementCase==11){fprintf(logfile,"PASS movement: measured run/sprint/diagonal, coast/timeout, jump cancel, wheel cancel, braking and walking crouch\n");fflush(logfile);ExitProcess(0);}}
}
