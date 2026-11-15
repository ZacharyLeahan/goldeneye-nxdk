/* Procedural, asset-free pad visual and equipment preview. */
static void padFace(const float p[24][2],float vw,float vh,int ghost,int valid,float pulse){
 float a=ghost?.36f:1.f;glDisable(GL_TEXTURE_2D);
 if(!ghost){glColor4f(.035f,.045f,.06f,.98f);glBegin(GL_QUADS);for(int i=0;i<8;i++){int j=(i+1)%8;glVertex2f(p[i][0]*vw,p[i][1]*vh);glVertex2f(p[j][0]*vw,p[j][1]*vh);glVertex2f(p[j+8][0]*vw,p[j+8][1]*vh);glVertex2f(p[i+8][0]*vw,p[i+8][1]*vh);}glEnd();}
 if(ghost)glColor4f(valid?.1f:1.f,valid?.8f:.2f,valid?1.f:.22f,a);else glColor4f(.12f,.14f,.17f,.98f);
 glBegin(GL_POLYGON);for(int i=0;i<8;i++)glVertex2f(p[i][0]*vw,p[i][1]*vh);glEnd();
 if(ghost)glColor4f(valid?.2f:1.f,valid?.85f:.2f,valid?1.f:.22f,.95f);else glColor4f(1,.78f,.15f,1);
 glLineWidth(2.f*uiScale);glBegin(GL_LINE_LOOP);for(int i=0;i<8;i++)glVertex2f(p[i][0]*vw,p[i][1]*vh);glEnd();
 if(!ghost){glColor4f(.87f,.66f,.14f,.95f);glBegin(GL_POLYGON);for(int i=16;i<24;i++)glVertex2f(p[i][0]*vw,p[i][1]*vh);glEnd();}
 float cx=0,cy=0;for(int i=16;i<24;i++){cx+=p[i][0]*vw/8;cy+=p[i][1]*vh/8;}
 float ux=(p[16][0]-p[20][0])*vw*.32f,uy=(p[16][1]-p[20][1])*vh*.32f,vx=(p[18][0]-p[22][0])*vw*.32f,vy=(p[18][1]-p[22][1])*vh*.32f;
 if(ghost)glColor4f(.7f,.95f,1,1);else glColor4f(.035f,.05f,.07f,1);
 static const float arrow[6][2]={{-1,.15f},{0,-.7f},{1,.15f},{.68f,.45f},{0,-.12f},{-.68f,.45f}};
 static const int corners[8]={0,1,4,5,1,2,3,4};
 for(int row=0;row<2;row++){glBegin(GL_QUADS);for(int j=0;j<8;j++){int k=corners[j];float u=arrow[k][0]*.78f,v=arrow[k][1]*.72f+(row? .50f:-.28f);glVertex2f(cx+ux*u+vx*v,cy+uy*u+vy*v);}glEnd();}
 if(pulse>0){glColor4f(.2f,.85f,1,pulse);glLineWidth((2+3*pulse)*uiScale);glBegin(GL_LINE_LOOP);for(int i=0;i<8;i++)glVertex2f(p[i][0]*vw,p[i][1]*vh);glEnd();}
}
static void drawPad(void*w){
 if(!playing())return;PadSnapshot s=padSnapshot();if(!s.carrying&&!s.visible)return;
 int width,height;getSize(w,&width,&height);if(width<1||height<1)return;
 int prog=0,active=0,mode=0;glGetIntegerv(0x8B8D,&prog);glGetIntegerv(0x84E0,&active);glGetIntegerv(GL_MATRIX_MODE,&mode);useProgram(0);activeTexture(0x84C0);
 glPushAttrib(GL_ALL_ATTRIB_BITS);glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_ALPHA_TEST);glDisable(GL_TEXTURE_2D);glDisable(GL_LIGHTING);glDisable(GL_STENCIL_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 uiScale=fminf(width/1280.f,height/720.f);float vw=width/uiScale,vh=height/uiScale;
 glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,vw,vh,0,-1,1);glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();initFont();
 if(s.visible)padFace(s.face,vw,vh,0,1,s.pulse);
 if(s.carrying&&!openWheel){
  if(s.previewVisible)padFace(s.ghost,vw,vh,1,s.valid,0);
  float cx=vw*.70f,cy=vh-75;float body[]={cx-122,cy-38,cx+85,cy-62,cx+139,cy-25,cx+126,cy+44,cx-102,cy+65,cx-140,cy+22};
  glColor4f(.035f,.045f,.06f,.98f);polygon(body,6);glLineWidth(2*uiScale);glColor4f(.95f,.75f,.2f,1);glBegin(GL_LINE_LOOP);for(int i=0;i<6;i++)glVertex2f(body[i*2],body[i*2+1]);glEnd();
  glPushMatrix();glTranslatef(cx-60,cy-15,0);glScalef(1.15f,1.15f,1);silhouette(26,50,15,115);glPopMatrix();
  glColor4f(.035f,.045f,.06f,.78f);rect(vw*.5f-182,vh-148,364,66);
  glColor4f(1,1,1,1);centered(vw*.5f,vh-124,"JUMP PAD");
  glColor4f(s.valid?.25f:1.f,s.valid?.9f:.4f,s.valid?1.f:.35f,1);centered(vw*.5f,vh-100,s.valid?"RT  PLACE     B  CANCEL":"NEEDS CLEAR, LEVEL FLOOR");
 }
 glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glPopAttrib();glMatrixMode(mode);useProgram(prog);activeTexture(active);
}
