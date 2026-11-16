/* The native post-move STAN edge correction treats the window wall as a
 * 2D floor-plan barrier. Suppress that one correction only for the exact
 * destination already accepted by a full 3D pad-flight body sweep. */
__attribute__((noinline)) static int portalLocusCount(void*locus){
 uintptr_t caller=(uintptr_t)__builtin_return_address(0);

 if(caller==base+0x1d717&&NUMBER==localIndex()&&CURRENT==gadget.owner&&gadget.flight&&gadget.airPortal&&jumpAllowed()&&portalStep.owner==CURRENT&&portalStep.timer==*(int*)(base+0x303000+211432)&&fabsf(PF(1864)-portalStep.position.x)<.01f&&fabsf(PF(1872)-portalStep.position.z)<.01f){portalClampSkips++;return 0;}
 return *(int*)((unsigned char*)locus+8);
}
static int portalPointWithin(void*tile,float x,float z){
 if(NUMBER==localIndex()&&CURRENT==gadget.owner&&gadget.flight&&gadget.portalBridge&&jumpAllowed()&&portalStep.owner==CURRENT&&fabsf(x-portalStep.position.x)<.01f&&fabsf(z-portalStep.position.z)<.01f)return 1;
 return FN(int(*)(void*,float,float),0xd5e10)(tile,x,z);
}
static int installPortalHook(void){
 unsigned char*site=(void*)(base+0x12b73);if(site[0]!=0xe8||(uintptr_t)(site+5+*(int*)(site+1))!=base+0xd5e10)return 0;
 unsigned char*thunk=NULL;for(uintptr_t offset=0x1000000;offset<0x70000000&&!thunk;offset+=0x1000000)thunk=VirtualAlloc((void*)(base+offset),64,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!thunk)return 0;
 unsigned char expected[]={0x8b,0x41,0x08,0xc3,0x90,0x66,0x66,0x2e,0x0f,0x1f,0x84,0x00};unsigned char*target=(void*)(base+0xd7520);if(memcmp(target,expected,12))return 0;
 unsigned char jump[]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};*(uintptr_t*)(jump+2)=(uintptr_t)portalPointWithin;memcpy(thunk,jump,12);DWORD prior;VirtualProtect(thunk,64,PAGE_EXECUTE_READ,&prior);FlushInstructionCache(GetCurrentProcess(),thunk,12);int displacement=(int)((intptr_t)thunk-(intptr_t)(site+5));if(!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&prior))return 0;memcpy(site+1,&displacement,4);VirtualProtect(site,5,prior,&prior);FlushInstructionCache(GetCurrentProcess(),site,5);
 unsigned char j[]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};*(uintptr_t*)(j+2)=(uintptr_t)portalLocusCount;DWORD protection;if(!VirtualProtect(target,12,PAGE_EXECUTE_READWRITE,&protection))return 0;memcpy(target,j,12);VirtualProtect(target,12,protection,&protection);FlushInstructionCache(GetCurrentProcess(),target,12);portalClampInstalled=1;return 1;
}
