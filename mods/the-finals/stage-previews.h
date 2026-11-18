/* GE32 2026.11.07. Keep the menu's own texture selection, tint, labels
 * and hit testing. Only two filmstrip cells need new image descriptors. */
enum { STAGE_PREVIEW_WIDTH = 68, STAGE_PREVIEW_HEIGHT = 44,
       STAGE_PREVIEW_STRIDE = 72, STAGE_PREVIEW_BYTES = STAGE_PREVIEW_STRIDE * STAGE_PREVIEW_HEIGHT };
typedef struct {
 uint32_t pixels;
 uint8_t width,height,level,format,depth,wrapS,wrapT,pad;
} StagePreviewImage;
static StagePreviewImage libraryPreview __attribute__((used)),stackPreview __attribute__((used));
static uintptr_t stagePreviewResume __attribute__((used));

__attribute__((naked)) static void stagePreviewLookup(void){
 __asm__ volatile(
  "movq (%r12), %rax\n\t"
  "leaq (%rax,%rdx,4), %r13\n\t" /* original descriptor */
  "cmpl $4, %ebx\n\t"
  "jne 1f\n\t"
  "leaq libraryPreview(%rip), %r13\n"
  "1: cmpl $6, %ebx\n\t"
  "jne 2f\n\t"
  "leaq stackPreview(%rip), %r13\n"
  "2: movl $2, 0x20(%rsp)\n\t"
  "jmp *stagePreviewResume(%rip)\n");
}
static int installStagePreviews(void){
 static const unsigned char lookup[]={0x49,0x8b,0x04,0x24,0xc7,0x44,0x24,0x20,2,0,0,0,0x4c,0x8d,0x2c,0x90};
 const int *setups=(const int*)(base+0x235e80);
 if(memcmp((void*)(base+0x5e29f),lookup,sizeof(lookup))||
    setups[4*6+2]!=48||setups[5*6+2]!=45||setups[6*6+2]!=46)return 0;
 FILE *file=fopen("data/mp-stage-previews.i8","rb");
 if(!file)return 0;
 /* texSelect uses a 32-bit pixel address and reads a texture-pool ID
  * eight bytes before it. Keep both immutable images in its raw host
  * address range, outside GE's emulated and segmented address ranges. */
 unsigned char *bank=NULL;
 for(uintptr_t addr=0x20000000;addr<0x40000000&&!bank;addr+=0x10000)
  bank=VirtualAlloc((void*)addr,2*(8+STAGE_PREVIEW_BYTES),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 int ok=bank!=NULL;
 StagePreviewImage *images[]={&libraryPreview,&stackPreview};
 for(int i=0;ok&&i<2;i++){
  unsigned char *header=bank+i*(8+STAGE_PREVIEW_BYTES);
  memset(header,0xff,8);
  ok=fread(header+8,1,STAGE_PREVIEW_BYTES,file)==STAGE_PREVIEW_BYTES;
  *images[i]=(StagePreviewImage){(uint32_t)(uintptr_t)(header+8),STAGE_PREVIEW_WIDTH,STAGE_PREVIEW_HEIGHT,0,4,1,2,2,0};
 }
 if(ok)ok=fgetc(file)==EOF&&!ferror(file);
 fclose(file);
 stagePreviewResume=base+0x5e2af;
 if(ok&&coopEntryJump(0x5e29f,lookup,sizeof(lookup),stagePreviewLookup))return 1;
 if(bank)VirtualFree(bank,0,MEM_RELEASE);
 return 0;
}

/* Offline installer capture. Position the real local player after the map
 * loads, then allow the native camera and renderer to settle. */
static int previewCaptureStage;
static volatile LONG previewCaptureReady;
static void stagePreviewCamera(void){
 static int ticks;
 if(!jumpAllowed()||*(int*)(base+0x303000+211452)!=previewCaptureStage)return;
 if(++ticks==30){
  if(previewCaptureStage==48)padTestPlacePlayer(1100.f,1400.f,5.f,90.f);
  else padTestPlacePlayer(-1996.f,750.7f,290.f,0.f);
 }
 if(ticks>=30){
  PF(380)=previewCaptureStage==48?90.f:0.f;
  PF(396)=previewCaptureStage==48?-12.f:-7.f;
 }
 if(ticks==65)InterlockedExchange(&previewCaptureReady,1);
}
static void stagePreviewCapture(void *window){
 if(InterlockedExchange(&previewCaptureReady,0)){
  padTestCapture(window,"stage-preview.bmp");
  ExitProcess(0);
 }
}
