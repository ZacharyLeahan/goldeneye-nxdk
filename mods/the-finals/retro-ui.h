/* Original 5x7 display lettering and hard-edged gauge primitives. No ROM data. */
static void retroQuad(float x,float y,float w,float h){glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);}
static const unsigned char retroGlyphs[][7]={
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,27,17},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
 {2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14},
 {0,0,0,31,0,0,0},{0,0,0,0,0,6,6},{0,6,6,0,6,6,0},{1,2,2,4,8,8,16},{14,17,1,2,4,0,4}
};
static float retroWidth(const char*s,float pixel){size_t n=strlen(s);return n?(n*6-1)*pixel:0;}
static void retroText(float x,float y,const char*s,float pixel){
 glDisable(GL_TEXTURE_2D);glBegin(GL_QUADS);for(;*s;s++,x+=6*pixel){unsigned c=(unsigned char)*s;if(c>='a'&&c<='z')c-=32;int index=c>='A'&&c<='Z'?(int)c-'A':c>='0'&&c<='9'?(int)c-'0'+26:c=='-'?36:c=='.'?37:c==':'?38:c=='/'?39:c==' '?-1:40;if(index<0)continue;for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(retroGlyphs[index][row]&(1<<(4-col)))retroQuad(x+col*pixel,y+row*pixel,pixel,pixel);}glEnd();
}
static void retroLabel(float x,float y,const char*s,float pixel,float r,float g,float b,float a){glColor4f(.015f,.018f,.012f,.85f*a);retroText(x+pixel,y+pixel,s,pixel);glColor4f(r,g,b,a);retroText(x,y,s,pixel);}
