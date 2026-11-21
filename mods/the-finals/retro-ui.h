/* Original 5x7 display lettering and hard-edged gauge primitives. No ROM data. */
static void retroQuad(float x,float y,float w,float h){glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);}
#include "../../port/controls/finals_font.inc"
static float retroWidth(const char*s,float pixel){size_t n=strlen(s);return n?(n*6-1)*pixel:0;}
static void retroText(float x,float y,const char*s,float pixel){
 glDisable(GL_TEXTURE_2D);glBegin(GL_QUADS);for(;*s;s++,x+=6*pixel){unsigned c=(unsigned char)*s;if(c>='a'&&c<='z')c-=32;int index=c>='A'&&c<='Z'?(int)c-'A':c>='0'&&c<='9'?(int)c-'0'+26:c=='-'?36:c=='.'?37:c==':'?38:c=='/'?39:c==' '?-1:40;if(index<0)continue;for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(retroGlyphs[index][row]&(1<<(4-col)))retroQuad(x+col*pixel,y+row*pixel,pixel,pixel);}glEnd();
}
static void retroLabel(float x,float y,const char*s,float pixel,float r,float g,float b,float a){glColor4f(.015f,.018f,.012f,.85f*a);retroText(x+pixel,y+pixel,s,pixel);glColor4f(r,g,b,a);retroText(x,y,s,pixel);}
