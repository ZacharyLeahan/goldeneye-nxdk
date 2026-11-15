#include "weapon-wheel.c"
static void saveBitmap(const char*path,int width,int height){
 int stride=(width*3+3)&~3;unsigned char*rgb=calloc(stride,height),*tmp=malloc(width*height*3);
 glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,tmp);
 for(int y=0;y<height;y++)for(int x=0;x<width;x++){int a=(y*width+x)*3,b=y*stride+x*3;rgb[b]=tmp[a+2];rgb[b+1]=tmp[a+1];rgb[b+2]=tmp[a];}
 BITMAPFILEHEADER f={0};BITMAPINFOHEADER h={0};f.bfType=0x4d42;f.bfOffBits=54;f.bfSize=54+stride*height;h.biSize=40;h.biWidth=width;h.biHeight=height;h.biPlanes=1;h.biBitCount=24;
 FILE*out=fopen(path,"wb");fwrite(&f,14,1,out);fwrite(&h,40,1,out);fwrite(rgb,stride*height,1,out);fclose(out);free(rgb);free(tmp);
}
int main(int argc,char**argv){
 HMODULE sdl=LoadLibraryA("SDL2.dll");if(!sdl)return 10;
 int(*init)(unsigned)=(void*)GetProcAddress(sdl,"SDL_Init");
 int(*attr)(int,int)=(void*)GetProcAddress(sdl,"SDL_GL_SetAttribute");
 void*(*create)(const char*,int,int,int,int,unsigned)=(void*)GetProcAddress(sdl,"SDL_CreateWindow");
 void*(*context)(void*)=(void*)GetProcAddress(sdl,"SDL_GL_CreateContext");
 if(init(0x20))return 11;attr(17,3);attr(18,0);attr(21,2);
 void*w=create("Offscreen wheel visual test",0,0,1280,720,0x2|0x8);if(!w||!context(w))return 12;
 getSize=(void*)GetProcAddress(sdl,"SDL_GL_GetDrawableSize");useProgram=(void*)wglGetProcAddress("glUseProgram");activeTexture=(void*)wglGetProcAddress("glActiveTexture");
 for(int guns=0;guns<=21;guns++){
  count=0;for(int id=4;count<guns&&id<=25;id++)if(id!=23)ids[count++]=id;ids[count++]=GRENADE_ITEM;ids[count++]=PAD_ITEM;
  if(wheelTarget(0)!=count-2||wheelTarget(1.57079632679f)!=count-1)return 15;
  for(int i=0;i<count;i++)if(wheelTarget(wheelAngle(i))!=i)return 16;
 }
 printf("PASS fixed cardinal slots and icon/selection agreement with 0..21 guns\n");
 for(int shot=0;shot<9;shot++){
  count=shot<2?4:shot==2?2:shot>=5?3:shot==3?5:8;published.grenades=shot==8?0:3;
  int examples[]={6,8,14,15,17,24,25,5};examples[count-2]=GRENADE_ITEM;examples[count-1]=PAD_ITEM;for(int j=0;j<count;j++)ids[j]=examples[j];
  choice=(shot==5||shot==8)?-1:shot==6?count-1:shot==1?1:0;oldCurrent=6;if(shot>=7)glClearColor(.85f,.83f,.77f,1);else glClearColor(.11f,.14f,.19f,1);glClear(GL_COLOR_BUFFER_BIT);draw(w);glFinish();
  char filename[2048];snprintf(filename,sizeof(filename),"%s/wheel-%d.bmp",argc>1?argv[1]:".",shot);saveBitmap(filename,1280,720);
  unsigned err=glGetError();printf("preview %d: owned=%d selected=%d GL=%u\n",shot,count,choice,err);if(err)return 13;
 }
 for(int shot=0;shot<6;shot++){
  HealthSnapshot s={0};s.count=1;HealthBar*b=&s.bars[0];b->x=.5f;b->y=.40f;b->hp=shot==0?1:shot==1?.65f:shot==2?.4f:shot==3?.15f:.75f;b->armor=shot==4?.6f:0;b->trail=b->hp+.1f;b->alpha=shot==5?.25f:1;snprintf(b->name,sizeof(b->name),"GUARD %d",shot+1);if(shot&1)glClearColor(.80f,.77f,.65f,1);else glClearColor(.075f,.09f,.065f,1);glClear(GL_COLOR_BUFFER_BIT);drawHealthSnapshot(w,s);glFinish();
  char filename[2048];snprintf(filename,sizeof(filename),"%s/health-style-%d.bmp",argc>1?argv[1]:".",shot);saveBitmap(filename,1280,720);unsigned err=glGetError();printf("health preview %d: HP=%.2f armor=%.2f alpha=%.2f GL=%u\n",shot,b->hp,b->armor,b->alpha,err);if(err)return 14;
 }
 return 0;
}
