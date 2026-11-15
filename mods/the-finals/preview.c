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
 for(int shot=0;shot<5;shot++){
  count=shot<2?2:shot==2?1:shot==3?3:8;
  int examples[]={6,8,5,14,15,17,24,25};for(int j=0;j<count;j++)ids[j]=examples[j];
  choice=shot==1?1:0;oldCurrent=6;glClearColor(.11f,.14f,.19f,1);glClear(GL_COLOR_BUFFER_BIT);draw(w);glFinish();
  char filename[2048];snprintf(filename,sizeof(filename),"%s/wheel-%d.bmp",argc>1?argv[1]:".",shot);saveBitmap(filename,1280,720);
  unsigned err=glGetError();printf("preview %d: owned=%d selected=%d GL=%u\n",shot,count,choice,err);if(err)return 13;
 }
 return 0;
}
