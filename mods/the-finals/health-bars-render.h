/* World-anchored eight-cell gauge: native-era green health, blue armor,
 * olive gunmetal bezel and original pixel lettering. Hit/fade timing is unchanged. */
static void healthQuad(float x,float y,float width,float height){glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+width,y);glVertex2f(x+width,y+height);glVertex2f(x,y+height);glEnd();}
static void healthSegments(float x,float y,float width,float height,float value){for(int i=0;i<8;i++){float f=healthClamp(value*8-i);if(f>0)healthQuad(x+i*width/8,y,(width/8-1.25f)*f,height);}}
static void drawHealthSnapshot(void*w,HealthSnapshot s){
 if(!s.count)return;int width,height;getSize(w,&width,&height);if(width<1||height<1)return;
 int prog=0,active=0,mode=0;glGetIntegerv(0x8B8D,&prog);glGetIntegerv(0x84E0,&active);glGetIntegerv(GL_MATRIX_MODE,&mode);useProgram(0);activeTexture(0x84C0);
 glPushAttrib(GL_ALL_ATTRIB_BITS);glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_ALPHA_TEST);glDisable(GL_TEXTURE_2D);glDisable(GL_LIGHTING);glDisable(GL_STENCIL_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 uiScale=fminf(width/1280.f,height/720.f);float vw=width/uiScale,vh=height/uiScale;
 glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,vw,vh,0,-1,1);glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
 for(int i=0;i<s.count;i++){HealthBar*b=&s.bars[i];float bw=80,x=floorf(b->x*vw-bw*.5f),y=floorf(b->y*vh),a=b->alpha;if(a<=0)continue;x=fmaxf(3,fminf(x,vw-fmaxf(bw,retroWidth(b->name,1.f))-3));
  glDisable(GL_TEXTURE_2D);glColor4f(.015f,.018f,.012f,.78f*a);healthQuad(x-2,y-2,bw+3,10);glColor4f(.36f,.38f,.29f,a);healthQuad(x-1,y-1,bw+1,1);healthQuad(x-1,y-1,1,9);glColor4f(.10f,.12f,.08f,a);healthQuad(x,y+7,bw,1);
  glColor4f(.11f,.15f,.09f,.92f*a);healthSegments(x,y,bw,6,1);
  glColor4f(.78f,.67f,.39f,.85f*a);healthSegments(x,y,bw,6,b->trail);
  float red=b->hp>.5f?.28f:b->hp>.25f?.83f:.82f,green=b->hp>.5f?.66f:b->hp>.25f?.64f:.18f,blue=b->hp>.5f?.22f:.12f;
  glColor4f(red,green,blue,a);healthSegments(x,y,bw,6,b->hp);glColor4f(fminf(1,red+.18f),fminf(1,green+.17f),blue+.12f,a);healthSegments(x,y,bw,1,b->hp);
  if(b->armor>0){glColor4f(.04f,.08f,.12f,.85f*a);healthSegments(x,y-6,bw,3,1);glColor4f(.30f,.57f,.79f,a);healthSegments(x,y-6,bw,3,b->armor);}
  retroLabel(x,y+11,b->name,1.f,.86f,.86f,.73f,a);
 }

 glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glPopAttrib();glMatrixMode(mode);useProgram(prog);activeTexture(active);
}

static void drawHealth(void*w){if(!playing()||!snapshotRead().allowJump||FN(int(*)(void),0x1000+1453264)())return;drawHealthSnapshot(w,healthSnapshot());}
