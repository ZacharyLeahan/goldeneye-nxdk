/* GE32 2026.11.07 co-op setup. The relay and client own the match, roster,
 * character selection, stage transition and disconnect lifecycle. */
typedef struct { char label[64]; int enabled, style; } CoopMenuRow;
static int (*coopOriginalRows)(CoopMenuRow *);
static void (*coopOriginalActivate)(int);
enum { SOLO_COOP_ROW = 75, COOP_SETUP_PAGE = 16 };
#define COOP_INT(rva) (*(int *)(base + (rva)))
static HANDLE soloRelay;
static int soloLaunchCount,soloWaiting;
static DWORD soloConnectAt;
static char soloMessage[64];

static int soloSetup(void){
 return COOP_INT(0x4c616c)==COOP_SETUP_PAGE && !FN(int(*)(void),0x138760)();
}
static void soloStart(void){
 int mission=COOP_INT(0x4c6168),difficulty=COOP_INT(0x4c6160);
 if(mission<0||mission>=20||difficulty<0||difficulty>2)return;
 /* A per-process port also allows two independent local games on one PC. */
 int port=40000+(int)(GetCurrentProcessId()%20000);
 char arguments[160],address[40];
 snprintf(arguments,sizeof(arguments),"--local --port %d --mode coop --stage %d --scenario %d --max 2 --fill 2 --countdown 0",port,mission,difficulty);
 snprintf(address,sizeof(address),"127.0.0.1:%d",port);
 /* The stock relay permits one ready human with fill enabled. Co-op's
  * roster builder explicitly excludes bot fill for this mode. */
 soloRelay=NULL;soloWaiting=0;
 if(!FN(int(*)(const char*),0x13b7c0)(arguments)){
  strcpy(soloMessage,"Relay unavailable - retry Start alone");return;
 }
 HANDLE process=*(HANDLE*)(base+0x4c5b40);
 if(!process||WaitForSingleObject(process,180)!=WAIT_TIMEOUT){
  FN(void(*)(void),0x13b600)();strcpy(soloMessage,"Relay failed - retry Start alone");return;
 }
 soloMessage[0]=0;
 FN(void(*)(void),0x121eb0)();
 FN(void(*)(int,const char*,int,int,int,int),0x1367f0)(0,NULL,0,0,0,0);
 FN(void(*)(int),0x136590)(difficulty);
 FN(void(*)(int),0x13a270)(0);
 if(!FN(int(*)(const char*,int),0x136790)(address,0)){
  FN(void(*)(void),0x13b600)();FN(void(*)(int),0x13a270)(COOP_SETUP_PAGE);
  strcpy(soloMessage,"Connection failed - retry Start alone");return;
 }
 soloRelay=process;soloWaiting=1;
 soloConnectAt=GetTickCount();soloLaunchCount=COOP_INT(0x4c5b28);
}
static int soloRows(CoopMenuRow *rows){
 if(soloRelay){
  if(soloRelay!=*(HANDLE*)(base+0x4c5b40))soloRelay=NULL;
  else if(COOP_INT(0x4c5b28)!=soloLaunchCount&&!COOP_INT(0x427f2c)&&
          !FN(int(*)(void),0x102400)()){
   /* Native co-op has completed its debrief and returned to the lobby. */
   soloRelay=NULL;FN(void(*)(void),0x13c1b0)();
   FN(void(*)(int),0x13a270)(COOP_SETUP_PAGE);
  }else if(soloWaiting){
   if(FN(int(*)(void),0x1387e0)()){
    HANDLE input=*(HANDLE*)(base+0x4c5b38);DWORD written=0;
    if(input&&WriteFile(input,"start\n",6,&written,NULL)&&written==6)soloWaiting=0;
   }
   if(soloWaiting&&GetTickCount()-soloConnectAt>8000){
    soloRelay=NULL;FN(void(*)(void),0x13c1b0)();FN(void(*)(int),0x13a270)(COOP_SETUP_PAGE);
    strcpy(soloMessage,"Connection failed - retry Start alone");
   }
  }
 }
 int count=coopOriginalRows(rows);
 if(!soloSetup()||count<1||count>5)return count;
 /* Insert before Film with bots and preserve native action IDs and navigation. */
 int *ids=(int*)(base+0x4c5fc0);
 int insert=count-1;
 rows[count]=rows[insert];ids[count]=ids[insert];
 rows[insert]=(CoopMenuRow){.enabled=1};
 snprintf(rows[insert].label,sizeof(rows[insert].label),"%s",soloMessage[0]?soloMessage:"Start alone");
 ids[insert]=SOLO_COOP_ROW;
 COOP_INT(0x4c5f60)=count+1;
 return count+1;
}
static void soloActivate(int row){
 if(soloSetup()&&row>=0&&row<COOP_INT(0x4c5f60)&&((int*)(base+0x4c5fc0))[row]==SOLO_COOP_ROW){soloStart();return;}
 coopOriginalActivate(row);
}
/* Both GE32 prologues consist entirely of position-independent pushes and
 * stack setup. Copy whole instructions before installing the entry jump. */
static void *coopOriginal(uintptr_t rva,const unsigned char *signature,size_t size){
 unsigned char *entry=(unsigned char*)(base+rva);
 if(memcmp(entry,signature,size))return NULL;
 unsigned char *copy=VirtualAlloc(NULL,size+14,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 if(!copy)return NULL;
 memcpy(copy,entry,size);
 const unsigned char jump[6]={0xff,0x25,0,0,0,0};
 memcpy(copy+size,jump,6);
 uintptr_t resume=(uintptr_t)(entry+size);
 memcpy(copy+size+6,&resume,sizeof(resume));
 DWORD old;
 if(!VirtualProtect(copy,size+14,PAGE_EXECUTE_READ,&old)){VirtualFree(copy,0,MEM_RELEASE);return NULL;}
 FlushInstructionCache(GetCurrentProcess(),copy,size+14);
 return copy;
}

/* A one-person roster is valid only for mode 8 (campaign co-op). Resume at
 * the native slot, mission, difficulty and payload checks in both cases. */
static uintptr_t coopRosterOK __attribute__((used)),coopRosterBad __attribute__((used));
__attribute__((naked)) static void coopRosterRange(void){
 __asm__ volatile(
  "leal -2(%rbx), %edx\n\t"
  "cmpb $30, %dl\n\t"
  "jbe 1f\n\t"
  "cmpb $1, %bl\n\t"
  "jne 2f\n\t"
  "cmpb $8, 0x1f2(%rbp)\n\t"
  "jne 2f\n"
  "1: movzbl 0x18d(%rbp), %r10d\n\t"
  "jmp *coopRosterOK(%rip)\n"
  "2: jmp *coopRosterBad(%rip)\n");
}

static int coopEntryJump(uintptr_t rva,const unsigned char *signature,size_t size,void *fn){
 unsigned char *entry=(void*)(base+rva);
 if(size<14||memcmp(entry,signature,size))return 0;
 unsigned char jump[14]={0xff,0x25,0,0,0,0};
 uintptr_t destination=(uintptr_t)fn;
 memcpy(jump+6,&destination,sizeof(destination));
 DWORD old;
 if(!VirtualProtect(entry,sizeof(jump),PAGE_EXECUTE_READWRITE,&old))return 0;
 memcpy(entry,jump,sizeof(jump));
 VirtualProtect(entry,sizeof(jump),old,&old);
 FlushInstructionCache(GetCurrentProcess(),entry,sizeof(jump));return 1;
}

static int coopInstallRoster(void){
 static const unsigned char sig[]={0x8d,0x53,0xfe,0x80,0xfa,0x1e,0x0f,0x87,0x2f,0xe9,0xff,0xff,0x44,0x0f,0xb6,0x95,0x8d,0x01,0x00,0x00};
 coopRosterOK=base+0x13351c;coopRosterBad=base+0x131e43;
 return coopEntryJump(0x133508,sig,sizeof(sig),coopRosterRange);
}


/* Zero is the native inactive sentinel. Allow the legitimately negotiated
 * single slot to retain its roster and report active during a mission. */
static int coopAllowSinglePlayerCount(void){
 static const struct {uintptr_t rva;unsigned char expected[7];int size;} tests[]={
  {0xfe806,{0x83,0xf9,1},3},                 /* ge32CoopSet: count > 0 */
  {0xfe918,{0x83,0x3d,0xa1,0xca,0x29,0,1},7},/* ge32CoopActive: roster > 0 */
  {0xfe945,{0x83,0xf8,1},3}                  /* runtime player count > 0 */
 };
 for(int i=0;i<3;i++)if(memcmp((void*)(base+tests[i].rva),tests[i].expected,tests[i].size))return 0;
 /* These three adjacent functions are on the same executable page. */
 unsigned char *page=(unsigned char*)(base+0xfe800);DWORD old;
 if(!VirtualProtect(page,0x150,PAGE_EXECUTE_READWRITE,&old))return 0;
 for(int i=0;i<3;i++)*((unsigned char*)base+tests[i].rva+tests[i].size-1)=0;
 VirtualProtect(page,0x150,old,&old);FlushInstructionCache(GetCurrentProcess(),page,0x150);
 return 1;
}

static int installSoloCoop(void){
 static const unsigned char rows[]={0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x57,0x56,0x53,0x48,0x81,0xec,0x78,0x01,0,0};
 static const unsigned char activate[]={0x55,0x57,0x56,0x48,0x81,0xec,0xb0,0x01,0,0,0x48,0x8d,0xac,0x24,0x80,0,0,0};
 coopOriginalRows=coopOriginal(0x13cc60,rows,sizeof(rows));
 coopOriginalActivate=coopOriginal(0x140910,activate,sizeof(activate));
 if(!coopOriginalRows||!coopOriginalActivate||!coopAllowSinglePlayerCount()||!coopInstallRoster())return 0;
 return coopEntryJump(0x140910,activate,sizeof(activate),soloActivate)&&
        coopEntryJump(0x13cc60,rows,sizeof(rows),soloRows);
}
