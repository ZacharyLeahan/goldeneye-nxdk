/* Offline grounded-control regression after a rejected pad passage. */
static int wallTicks,wallState,wallWait,wallFlightTicks,wallCase;static float wallLandingX,wallStartHealth;
static void wallEscapeBefore(signed char*x,signed char*y,unsigned short*buttons){
 if(!getenv("GE32_WALL_ESCAPE_TEST"))return;*x=*y=0;*buttons=0;if(!jumpAllowed())return;wallTicks++;wallWait++;
 if(wallTicks==180){float launchX=wallCase>=2?518:450,z=(wallCase&1)?-1707.9f:-1650.f;padTestPlacePlayer(launchX,z,470,270);PF(396)=0;PF(416)=PF(420)=PF(424)=0;PF(432)=PF(436)=PF(440)=0;FN(void(*)(void),0x1000+87712)();gadget.owner=CURRENT;gadget.stage=*(int*)(base+0x303000+211452);gadget.position=(JumpVec){launchX,PF(140),z};gadget.placed=1;gadget.cooldown=0;wallState=1;wallWait=0;wallFlightTicks=0;wallStartHealth=PF(260);}
 if(wallState){PF(380)=270;FN(void(*)(void),0x1000+87712)();}
 if(wallState==1){if(gadget.flight){wallFlightTicks++;if(wallFlightTicks<=110)*buttons=8;}if(wallFlightTicks>30&&!gadget.flight){wallLandingX=PF(1864);if(wallCase<2)gadget.placed=0;wallState=2;wallWait=0;fprintf(logfile,"wall landed case=%d x=%.2f drop=%d ground=%.2f\n",wallCase,wallLandingX,drop.active,PF(144));fflush(logfile);}}
 if(wallState==2){*buttons=4;if(wallWait>=40){openingCheck(wallLandingX-PF(1864)>100,"backward movement must escape the wall after landing");openingCheck((wallCase>=2||fabsf(PF(140)-PF(144))<1.5f)&&PF(260)>=wallStartHealth-.001f,"escaping stays grounded without damage");fprintf(logfile,"PASS wall escape case=%d: backed away %.2f, drop=%d\n",wallCase,wallLandingX-PF(1864),drop.active);fflush(logfile);if(++wallCase==4){ExitProcess(0);}wallTicks=179;wallState=0;}}
 openingCheck(wallTicks<650,"wall escape deadline");
}
