/* Opt-in native-engine acceptance test; ordinary play never enters fixtures. */
static int grenadeTestTicks,grenadeTestPhase,grenadeTestWait,grenadeTestFire,grenadeTestThrows,grenadeTestAmmo;
static volatile LONG grenadeTestSelect;
static void grenadeCheck(int ok,const char*why){if(!ok){if(logfile){fprintf(logfile,"FAIL grenades: %s phase=%d wait=%d total=%d weapon=%d state=%d\n",why,grenadeTestPhase,grenadeTestWait,grenadeAmmo(),HI(0,0),HI(0,36));fflush(logfile);}ExitProcess(2);}}
static void grenadeTestBefore(signed char*x,signed char*y,unsigned short*buttons){
 if(!getenv("GE32_GRENADE_TEST")||!jumpAllowed())return;*x=*y=0;*buttons=0;grenadeTestFire=0;
 grenadeCheck(++grenadeTestTicks<2200,"integration deadline");grenadeTestWait++;
 if(grenadeTestTicks==90){grenadeCheck(grenadeAmmo()==3,"fresh life receives exactly three grenades");FN(int(*)(int),0x1000+44224)(5);swapNow(0,5);grenadeTestAmmo=testTotalAmmo(1);InterlockedExchange(&grenadeTestSelect,PAD_ITEM);grenadeTestPhase=1;grenadeTestWait=0;}
 if(grenadeTestPhase==1&&padCarrying){InterlockedExchange(&grenadeTestSelect,GRENADE_ITEM);grenadeTestPhase=2;grenadeTestWait=0;}
 if(grenadeTestPhase==2&&HI(0,0)==GRENADE_ITEM&&HI(0,4)<0&&HI(0,36)==0&&grenadeTestWait>100){grenadeCheck(!padCarrying,"grenade selection stows pad");grenadeTestPhase=3;grenadeTestWait=0;}
 if(grenadeTestPhase==3){
  if(grenadeTestWait<10){*buttons=0x2000;grenadeTestFire=1;}
  if(grenadeTestWait>180){grenadeTestThrows++;grenadeCheck(grenadeAmmo()==3-grenadeTestThrows,"one native throw consumes exactly one grenade");grenadeCheck(testTotalAmmo(1)==grenadeTestAmmo,"throw preserves gun ammo");grenadeTestWait=0;if(grenadeTestThrows==3)grenadeTestPhase=4;}
 }
 if(grenadeTestPhase==4&&grenadeTestWait>120){grenadeCheck(grenadeAmmo()==0,"empty grenades are not silently refilled");InterlockedExchange(&grenadeTestSelect,GRENADE_ITEM);grenadeTestPhase=5;grenadeTestWait=0;}
 if(grenadeTestPhase==5&&grenadeTestWait>30){grenadeCheck(!available(GRENADE_ITEM),"empty wheel slot cannot equip grenades");InterlockedExchange(&grenadeTestSelect,PAD_ITEM);grenadeTestPhase=6;grenadeTestWait=0;}
 if(grenadeTestPhase==6&&padCarrying){grenadeCheck(grenadeAmmo()==0,"pad selection does not refill grenades");InterlockedExchange(&grenadeTestSelect,5);grenadeTestPhase=7;grenadeTestWait=0;}
 if(grenadeTestPhase==7&&HI(0,0)==5&&!padCarrying&&grenadeTestWait>100){grenadeCheck(testTotalAmmo(1)==grenadeTestAmmo,"gun ammo survives grenade and pad round trip");*reserveAmmo(ammoType(GRENADE_ITEM))=2;FN(int(*)(int),0x1000+44224)(GRENADE_ITEM);grenadeTestPhase=8;grenadeTestWait=0;}
 if(grenadeTestPhase==8&&grenadeTestWait>30){grenadeCheck(grenadeAmmo()==2&&available(GRENADE_ITEM),"native ammo pickup re-enables right slot without top-up");if(logfile){fprintf(logfile,"PASS grenades: fresh three, bottom pad/right grenade wheel selection, native equip, three actual throws to zero, no refill, empty rejection, gun ammo preserved, pickup re-enabled\n");fflush(logfile);}ExitProcess(0);}
}
