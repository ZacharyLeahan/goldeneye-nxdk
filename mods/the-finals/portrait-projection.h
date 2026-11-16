/* The portrait's shallow ortho depth range clips its slightly tilted image
 * plane. Widen depth only for this supported-release call; XY, UVs, menu
 * scissors, animation and every gameplay/world projection remain native. */
static void portraitOrtho(void*m,float left,float right,float bottom,float top,float nearZ,float farZ,float scale){
 (void)nearZ;(void)farZ;FN(void(*)(void*,float,float,float,float,float,float,float),0x199570)(m,left,right,bottom,top,-64.f,64.f,scale);
}
static int installPortraitProjection(void){
 if(getenv("GE32_PORTRAIT_BASELINE"))return 0;
 unsigned char*site=(void*)(base+0x5bbb3);if(site[0]!=0xe8||(uintptr_t)(site+5+*(int*)(site+1))!=base+0x199570)return 0;
 unsigned char*thunk=NULL;for(uintptr_t offset=0x1000000;offset<0x70000000&&!thunk;offset+=0x1000000)thunk=VirtualAlloc((void*)(base+offset),64,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!thunk)return 0;
 unsigned char jump[]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};*(uintptr_t*)(jump+2)=(uintptr_t)portraitOrtho;memcpy(thunk,jump,12);DWORD prior;if(!VirtualProtect(thunk,64,PAGE_EXECUTE_READ,&prior))return 0;FlushInstructionCache(GetCurrentProcess(),thunk,12);
 intptr_t distance=(intptr_t)thunk-(intptr_t)(site+5);if(distance<INT32_MIN||distance>INT32_MAX)return 0;int displacement=(int)distance;
 if(!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&prior))return 0;memcpy(site+1,&displacement,4);VirtualProtect(site,5,prior,&prior);FlushInstructionCache(GetCurrentProcess(),site,5);return 1;
}
