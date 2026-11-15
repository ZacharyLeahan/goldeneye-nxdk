/* Native hand grenades. Award once per playable life, on the simulation thread.
 * Synthetic gadgets must never reuse an engine weapon ID. */
static int grenadeAmmo(void){int type=ammoType(GRENADE_ITEM),n=0;if(type<=0||type>=33)return 0;n=*reserveAmmo(type);for(int h=0;h<2;h++)if(HI(h,0)==GRENADE_ITEM)n+=HI(h,44);return n>0?n:0;}
static void grenadeLoadout(void){
 static void*owner;static int stage,timer,awarded;int nowStage=*(int*)(base+0x303000+211452),now=*(int*)(base+0x303000+211432);
 if(owner!=CURRENT||stage!=nowStage||now<timer){owner=CURRENT;stage=nowStage;awarded=0;}timer=now;
 if(PI(256)){awarded=0;return;}if(awarded||!jumpAllowed())return;
 int type=ammoType(GRENADE_ITEM);if(type<=0||type>=33)return;
 FN(int(*)(int),0x1000+44224)(GRENADE_ITEM);
 /* Three at spawn; subsequent pickups/throws are entirely native. */
 *reserveAmmo(type)=3;for(int h=0;h<2;h++)if(HI(h,0)==GRENADE_ITEM)*reserveAmmo(type)-=HI(h,44);awarded=1;
 if(logfile){fprintf(logfile,"grenade loadout total=%d type=%d\n",grenadeAmmo(),type);fflush(logfile);}
}
