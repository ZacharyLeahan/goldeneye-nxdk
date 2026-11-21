/* Original low-poly field equipment. Textures are generated from a tiny,
 * deliberately coarse metal/rubber palette; no game or Embark assets are used. */
static unsigned padRubberTexture;
static void padMaterial(void){
 if(padRubberTexture)return;unsigned char pixels[32*32*3];
#include "../../port/controls/finals_material.inc"
 glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);glGenTextures(1,&padRubberTexture);glBindTexture(GL_TEXTURE_2D,padRubberTexture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812F);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812F);glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,32,32,0,GL_RGB,GL_UNSIGNED_BYTE,pixels);glPopClientAttrib();
}
static void padVertex(const float p[3],float vw,float vh){float d=p[2];glVertex4f(p[0]*vw*d,p[1]*vh*d,0,d);}
static void padMix(const float a[3],const float b[3],float f,float out[3]){float d=a[2]*(1-f)+b[2]*f;out[2]=d;out[0]=(a[0]*a[2]*(1-f)+b[0]*b[2]*f)/d;out[1]=(a[1]*a[2]*(1-f)+b[1]*b[2]*f)/d;}
static void padPatch(const float p[24][3],int side,float u0,float u1,float v0,float v1,float vw,float vh){
 int j=(side+1)%8;float corners[4][3];float us[4]={u0,u1,u1,u0},vs[4]={v0,v0,v1,v1};for(int k=0;k<4;k++){float a[3],b[3];padMix(p[side],p[j],us[k],a);padMix(p[side+16],p[j+16],us[k],b);padMix(a,b,vs[k],corners[k]);}glBegin(GL_QUADS);for(int k=0;k<4;k++)padVertex(corners[k],vw,vh);glEnd();
}
static void padFace(const float p[24][3],float vw,float vh,int ghost,int valid,float pulse){
 glDisable(GL_TEXTURE_2D);
 for(int i=0;i<8;i++){int j=(i+1)%8;float light=.19f+.045f*cosf(i*.7853981634f);if(ghost)glColor4f(valid?.52f:.82f,valid?.72f:.18f,valid?.30f:.12f,.16f);else glColor4f(light*.86f,light,light*.72f,1);glBegin(GL_QUADS);padVertex(p[i],vw,vh);padVertex(p[j],vw,vh);padVertex(p[j+8],vw,vh);padVertex(p[i+8],vw,vh);glEnd();}
 for(int i=0;i<8;i++){int j=(i+1)%8;float light=.33f+.085f*cosf((i+2)*.7853981634f);if(ghost)glColor4f(valid?.52f:.82f,valid?.72f:.18f,valid?.30f:.12f,.22f);else glColor4f(light*.91f,light,light*.72f,1);glBegin(GL_QUADS);padVertex(p[i],vw,vh);padVertex(p[j],vw,vh);padVertex(p[j+16],vw,vh);padVertex(p[i+16],vw,vh);glEnd();}
 if(!ghost){padMaterial();glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,padRubberTexture);glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);glColor4f(.94f+fminf(.06f,pulse*.06f),.95f,.87f,1);}else glColor4f(valid?.52f:.82f,valid?.72f:.18f,valid?.30f:.12f,.12f);
 glBegin(GL_POLYGON);for(int i=0;i<8;i++){float a=6.28318530718f*(i+.5f)/8;glTexCoord2f(.5f+cosf(a)*.49f,.5f+sinf(a)*.49f);padVertex(p[i+16],vw,vh);}glEnd();glDisable(GL_TEXTURE_2D);
 if(!ghost){
  /* Recessed fasteners and short ochre identification tabs on the bevel. */
  for(int i=0;i<8;i++){glColor4f(.08f,.10f,.07f,1);padPatch(p,i,.43f,.58f,.28f,.73f,vw,vh);glColor4f(.59f,.60f,.46f,1);padPatch(p,i,.46f,.55f,.35f,.57f,vw,vh);glColor4f(.18f,.20f,.14f,1);padPatch(p,i,.495f,.515f,.35f,.57f,vw,vh);}
  glColor4f(.64f,.56f,.29f,1);padPatch(p,1,.16f,.34f,.10f,.35f,vw,vh);padPatch(p,5,.16f,.34f,.10f,.35f,vw,vh);
 }else{
  glColor4f(valid?.65f:.92f,valid?.82f:.27f,valid?.40f:.17f,.75f);glLineWidth(fmaxf(1,uiScale));glBegin(GL_LINE_LOOP);for(int i=0;i<8;i++)padVertex(p[i],vw,vh);glEnd();
 }
}
#include "../../port/controls/finals_held.inc"
static void drawPad(void*w){
 static int debug,wasDown;int down=(GetAsyncKeyState(VK_F8)&0x8000)!=0;if(down&&!wasDown){debug=!debug;InterlockedExchange(&mantleDebugEnabled,debug);}wasDown=down;
 if(!playing())return;PadSnapshot s=padSnapshot();if(!debug&&!getenv("GE32_MANTLE_TEST")&&!s.carrying&&!s.visible&&s.carryBlend<=0)return;
 int width,height;getSize(w,&width,&height);if(width<1||height<1)return;
 int prog=0,active=0,mode=0;glGetIntegerv(0x8B8D,&prog);glGetIntegerv(0x84E0,&active);glGetIntegerv(GL_MATRIX_MODE,&mode);useProgram(0);activeTexture(0x84C0);
 glPushAttrib(GL_ALL_ATTRIB_BITS);glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_ALPHA_TEST);glDisable(GL_TEXTURE_2D);glDisable(GL_LIGHTING);glDisable(GL_STENCIL_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 uiScale=fminf(width/1280.f,height/720.f);float vw=width/uiScale,vh=height/uiScale;
 glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,vw,vh,0,-1,1);glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
 if(debug||getenv("GE32_MANTLE_TEST")){char line[160];AcquireSRWLockShared(&mantleDebugLock);memcpy(line,mantleDebugLine,sizeof(line));ReleaseSRWLockShared(&mantleDebugLock);
  glColor4f(.02f,.02f,.02f,.8f);retroQuad(12,12,retroWidth(line,1.8f)+12,24);retroLabel(18,18,line,1.8f,.8f,.95f,.75f,1);}
 if(s.visible)padFace(s.face,vw,vh,0,1,s.pulse);
 if(s.carryBlend>0&&!openWheel){float held[24][3];padHeldFace(held,vw,vh,s.carryBlend);padFace(held,vw,vh,0,1,0);}
 if(s.carrying&&!openWheel){
  if(s.previewVisible)padFace(s.ghost,vw,vh,1,s.valid,0);
  retroLabel(vw*.75f-38,vh-34+(1-s.carryBlend)*180,"Q-07",1.3f,.72f,.72f,.54f,1);
  const char*hint=s.valid?"RT: PLACE   B: CANCEL":"CLEAR LEVEL FLOOR REQUIRED";retroLabel((vw-retroWidth("JUMP PAD",1.4f))*.5f,vh-94,"JUMP PAD",1.4f,.86f,.86f,.73f,1);retroLabel((vw-retroWidth(hint,1.f))*.5f,vh-77,hint,1.f,s.valid?.66f:.88f,s.valid?.73f:.32f,s.valid?.44f:.22f,1);
 }
 glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glPopAttrib();glMatrixMode(mode);useProgram(prog);activeTexture(active);
}
