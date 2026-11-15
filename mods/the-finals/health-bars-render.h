/* Small world-anchored segmented bars, with a short damage trail. */
static void healthQuad(float x,float y,float width,float height){glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+width,y);glVertex2f(x+width,y+height);glVertex2f(x,y+height);glEnd();}
static void healthSegments(float x,float y,float width,float height,float value){for(int i=0;i<8;i++){float f=healthClamp(value*8-i);if(f>0)healthQuad(x+i*width/8,y,(width/8-1.25f)*f,height);}}
static void drawHealth(void*w){
 HealthSnapshot s=healthSnapshot();if(!playing()||!snapshotRead().allowJump||FN(int(*)(void),0x1000+1453264)()||!s.count)return;int width,height;getSize(w,&width,&height);if(width<1||height<1)return;
 int prog=0,active=0,mode=0;glGetIntegerv(0x8B8D,&prog);glGetIntegerv(0x84E0,&active);glGetIntegerv(GL_MATRIX_MODE,&mode);useProgram(0);activeTexture(0x84C0);
 glPushAttrib(GL_ALL_ATTRIB_BITS);glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_ALPHA_TEST);glDisable(GL_TEXTURE_2D);glDisable(GL_LIGHTING);glDisable(GL_STENCIL_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 uiScale=fminf(width/1280.f,height/720.f);float vw=width/uiScale,vh=height/uiScale;
 glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,vw,vh,0,-1,1);glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();initFont();
 for(int i=0;i<s.count;i++){HealthBar*b=&s.bars[i];float bw=80,x=b->x*vw-bw*.5f,y=b->y*vh,a=b->alpha;if(a<=0)continue;
  glDisable(GL_TEXTURE_2D);glColor4f(.015f,.02f,.025f,.35f*a);healthQuad(x-2,y-1,bw+3,7);
  glColor4f(.30f,.065f,.10f,.68f*a);healthSegments(x,y,bw,5,1);
  glColor4f(1,.53f,.54f,.8f*a);healthSegments(x,y,bw,6,b->trail);
  glColor4f(.91f,.11f,.23f,a);healthSegments(x,y,bw,6,b->hp);
  if(b->armor>0){glColor4f(.91f,.95f,1,.9f*a);healthSegments(x,y-5,bw,2,b->armor);}
  glPushMatrix();glTranslatef(x,y+17,0);glScalef(.55f,.55f,1);glColor4f(0,0,0,.75f*a);text(1,1,b->name);glColor4f(1,.21f,.30f,a);text(0,0,b->name);glPopMatrix();
 }
 glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glPopAttrib();glMatrixMode(mode);useProgram(prog);activeTexture(active);
}
