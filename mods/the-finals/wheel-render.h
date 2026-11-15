/* Crisp vector silhouettes, composed per weapon rather than generic badges. */
static float uiScale=1,advances[128];
static int fontPixels;
static unsigned fontTexture;static int atlasSize,cellSize;
static void text(float x,float y,const char*s){
 glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,fontTexture);glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
 glBegin(GL_QUADS);
 for(;*s;s++){unsigned c=(unsigned char)*s;if(c>=128)continue;float u=(c%16)*cellSize/(float)atlasSize,v=(c/16)*cellSize/(float)atlasSize,du=(advances[c]+1)/(float)atlasSize,dv=(fontPixels+3)/(float)atlasSize,d=(advances[c]+1)/uiScale,dh=(fontPixels+3)/uiScale,top=y-fontPixels/uiScale;
  glTexCoord2f(u,v);glVertex2f(x,top);glTexCoord2f(u+du,v);glVertex2f(x+d,top);glTexCoord2f(u+du,v+dv);glVertex2f(x+d,top+dh);glTexCoord2f(u,v+dv);glVertex2f(x,top+dh);x+=advances[c]/uiScale;
 }
 glEnd();glDisable(GL_TEXTURE_2D);
}
static float textWidth(const char*s){float w=0;for(;*s;s++){unsigned c=(unsigned char)*s;if(c<128)w+=advances[c];}return w/uiScale;}
static void centered(float x,float y,const char*s){text(x-textWidth(s)*.5f,y,s);}
static void initFont(void){
 int pixels=(int)(17*uiScale+.5f);if(fontTexture&&pixels==fontPixels)return;
 if(fontTexture)glDeleteTextures(1,&fontTexture);fontPixels=pixels;cellSize=pixels*2;atlasSize=256;while(atlasSize<cellSize*16)atlasSize*=2;
 HDC dc=CreateCompatibleDC(NULL);BITMAPINFO bi={0};bi.bmiHeader.biSize=40;bi.bmiHeader.biWidth=atlasSize;bi.bmiHeader.biHeight=-atlasSize;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
 unsigned char*bits;HBITMAP bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,(void**)&bits,NULL,0);HGDIOBJ oldBitmap=SelectObject(dc,bitmap);
 HFONT f=CreateFontA(-pixels,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FF_DONTCARE,"Segoe UI");HGDIOBJ oldFont=SelectObject(dc,f);
 SetBkColor(dc,RGB(0,0,0));SetTextColor(dc,RGB(255,255,255));SetBkMode(dc,TRANSPARENT);
 memset(bits,0,atlasSize*atlasSize*4);
 for(int c=32;c<127;c++){char ch=(char)c;SIZE sz={0};GetTextExtentPoint32A(dc,&ch,1,&sz);advances[c]=(float)sz.cx;TextOutA(dc,(c%16)*cellSize,(c/16)*cellSize,&ch,1);}
 GdiFlush();
 for(int i=0;i<atlasSize*atlasSize;i++){unsigned char alpha=bits[i*4];if(bits[i*4+1]>alpha)alpha=bits[i*4+1];if(bits[i*4+2]>alpha)alpha=bits[i*4+2];bits[i*4]=bits[i*4+1]=bits[i*4+2]=255;bits[i*4+3]=alpha;}
 glGenTextures(1,&fontTexture);glBindTexture(GL_TEXTURE_2D,fontTexture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,atlasSize,atlasSize,0,GL_RGBA,GL_UNSIGNED_BYTE,bits);
 SelectObject(dc,oldFont);SelectObject(dc,oldBitmap);DeleteObject(f);DeleteObject(bitmap);DeleteDC(dc);
}
static void polygon(const float*p,int n){
 glBegin(GL_POLYGON);for(int i=0;i<n;i++)glVertex2f(p[i*2],p[i*2+1]);glEnd();
 glBegin(GL_LINE_LOOP);for(int i=0;i<n;i++)glVertex2f(p[i*2],p[i*2+1]);glEnd();
}
static void rect(float x,float y,float w,float h){float p[]={x,y,x+w,y,x+w,y+h,x,y+h};polygon(p,4);}
static void shape(const float*p,int n){polygon(p,n);}
static void silhouette(int id,float x,float y,float size){
 glPushMatrix();glTranslatef(x-size*.5f,y-size*.22f,0);glScalef(size/100,size/100,1);glLineWidth(fmaxf(1.f,uiScale));
 if(id==PAD_ITEM){float p[]={25,3,75,3,97,17,97,39,75,53,25,53,3,39,3,17};shape(p,8);glColor4f(.10f,.12f,.08f,1);float r[]={29,10,71,10,86,20,86,36,71,46,29,46,14,36,14,20};shape(r,8);glColor4f(1,1,1,1);float arrow[]={50,14,66,30,34,30};shape(arrow,3);rect(43,28,14,12);}
 else if(id==GRENADE_ITEM){float body[]={40,9,61,9,72,22,72,42,62,53,38,53,28,42,28,22};shape(body,8);rect(43,1,17,10);float lever[]={54,0,70,0,81,25,77,28,66,6,54,6};shape(lever,6);glBegin(GL_LINE_LOOP);for(int k=0;k<16;k++){float a=k*6.2831853f/16;glVertex2f(36+cosf(a)*7,5+sinf(a)*7);}glEnd();}
 else if(id==4||id==5||id==6||id==18||id==19||id==20||id==21){
  int revolver=id==18;float muzzle=id==5?91:78;
  rect(24,12,54,10);rect(24,9,3,4);rect(70,9,3,4);
  float grip[]={35,21,52,21,46,43,31,43};shape(grip,4);
  if(id==5)rect(77,11,19,12);
  if(id==6){rect(26,21,32,4);rect(78,15,5,6);}
  if(revolver){rect(37,10,17,15);rect(69,13,22,6);rect(51,25,14,3);rect(62,22,3,6);}
  if(id==19){rect(20,12,65,6);rect(43,5,12,9);}
  if(id==4||id==5||id==20||id==21){rect(50,24,13,3);rect(60,21,3,6);}
  (void)muzzle;
 }else if(id==7||id==9||id==10||id==11||id==12){
  float bodyW=id==7?36:id==9?37:48;rect(25,11,bodyW,13);rect(22,8,4,6);rect(25+bodyW-5,8,3,5);
  float grip[]={35,23,48,23,45,42,32,42};shape(grip,4);
  if(id==7||id==9)rect(36,32,9,17);else {float mag[]={54,24,64,24,68,42,60,43};shape(mag,4);}
  rect(25+bodyW,15,id==11?23:14,id==11?9:5);
  if(id==10||id==11||id==12){float stock[]={24,13,6,13,3,30,10,30,14,21,25,21};shape(stock,6);}
  else{rect(20,17,5,4);}
 }else if(id==14){
  float p[]={4,13,69,13,74,17,94,17,94,22,72,22,65,32,54,32,49,25,31,25,28,35,7,35};shape(p,12);
  rect(24,7,45,5);rect(46,4,13,4);rect(12,20,9,10);
 }else if(id==8||id==13||id==17){
  float stock[]={3,18,25,12,31,16,29,28,3,35};shape(stock,5);
  rect(28,13,40,13);rect(67,16,28,4);rect(86,11,3,6);
  float grip[]={36,25,47,25,43,40,34,39};shape(grip,4);
  if(id==8){float mag[]={53,25,63,25,65,35,72,45,63,48,56,37};shape(mag,6);rect(63,11,13,12);}
  if(id==13){float mag[]={52,25,62,25,60,44,50,44};shape(mag,4);rect(36,6,22,5);rect(36,10,3,4);rect(55,10,3,4);}
  if(id==17){rect(43,5,25,7);rect(48,11,3,4);rect(65,11,3,4);rect(52,26,10,8);}
 }else if(id==15||id==16){
  float stock[]={2,20,27,13,36,17,29,29,2,36};shape(stock,5);rect(28,13,32,11);rect(59,13,37,4);rect(50,20,36,5);
  if(id==15)rect(62,22,16,7);else {float mag[]={43,24,54,24,52,41,42,40};shape(mag,4);}
 }else if(id==22){
  float p[]={3,18,20,18,25,10,72,10,76,15,94,15,94,22,73,22,65,28,45,28,38,43,27,43,31,27,17,27,3,34};shape(p,15);rect(39,6,23,4);rect(76,13,7,11);
 }else if(id==24||id==25){
  if(id==25){float p[]={3,10,16,10,21,13,85,13,88,9,96,9,96,30,88,30,85,26,22,26,16,30,3,30};shape(p,12);rect(44,26,9,16);rect(60,26,8,12);rect(55,6,8,7);}
  else{float stock[]={3,16,26,13,30,18,26,28,3,34};shape(stock,5);rect(26,13,32,12);rect(57,12,38,16);rect(34,25,9,15);rect(69,26,12,5);}
 }else rect(12,14,76,16);
 glPopMatrix();
}
static void arc(float cx,float cy,float r,float start,float end){glBegin(GL_LINE_STRIP);for(int k=0;k<=64;k++){float a=start+(end-start)*k/64;glVertex2f(cx+cosf(a)*r,cy+sinf(a)*r);}glEnd();}
static void draw(void*w){
 int width,height;getSize(w,&width,&height);if(width<1||height<1)return;
 int prog=0,active=0;glGetIntegerv(0x8B8D,&prog);glGetIntegerv(0x84E0,&active);useProgram(0);activeTexture(0x84C0);
 glPushAttrib(GL_ALL_ATTRIB_BITS);glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_ALPHA_TEST);glDisable(GL_TEXTURE_2D);glDisable(GL_LIGHTING);glDisable(GL_STENCIL_TEST);
 glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 uiScale=fminf(width/1280.f,height/720.f);float vw=width/uiScale,vh=height/uiScale;
 glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,vw,vh,0,-1,1);glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();initFont();
 float cx=vw*.5f,cy=vh*.5f-8,r=230,inner=118;const float tau=6.28318530718f;
 glColor4f(.015f,.02f,.03f,.025f);glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(vw,0);glVertex2f(vw,vh);glVertex2f(0,vh);glEnd();
 int slices=count?count:1;
 for(int j=0;j<slices;j++){
  float start=0,end=tau,center=0;if(count){wheelBounds(j,&start,&end);center=wheelAngle(j);start+=.018f;end-=.018f;}int sel=j==choice&&count;
  if(sel)glColor4f(.48f,.51f,.57f,.28f);else glColor4f(.035f,.045f,.06f,.18f);
  glBegin(GL_TRIANGLE_STRIP);for(int k=0;k<=64;k++){float a=start+(end-start)*k/64;glVertex2f(cx+cosf(a)*inner,cy+sinf(a)*inner);glVertex2f(cx+cosf(a)*r,cy+sinf(a)*r);}glEnd();
  glLineWidth((sel?3.f:1.f)*uiScale);if(sel)glColor4f(1,1,1,.92f);else glColor4f(.85f,.9f,1,.16f);arc(cx,cy,r-1,start,end);
  if(count){float tx=cx+cosf(center)*174,ty=cy+sinf(center)*174;float iconSize=count>8?56:count>5?69:91;
   if(ids[j]!=PAD_ITEM){glColor4f(0,0,0,.55f);silhouette(ids[j],tx+1.2f,ty-9.8f,iconSize);}
   glColor4f(sel?1:.86f,sel?1:.89f,sel?1:.94f,ids[j]==GRENADE_ITEM&&snapshotRead().grenades<=0?.3f:1);silhouette(ids[j],tx,ty-11,iconSize);glColor4f(sel?1:.86f,sel?1:.89f,sel?1:.94f,1);
   if(ids[j]==GRENADE_ITEM){char ammo[16];snprintf(ammo,sizeof(ammo),"%d",snapshotRead().grenades);glColor4f(1,1,1,.85f);centered(tx,ty+44,ammo);}
  }
 }
 /* The open center deliberately leaves the world/reticle unobscured. */
 glColor4f(.9f,.94f,1,.18f);glLineWidth(uiScale);arc(cx,cy,inner-2,0,tau);
 if(choice>=0&&choice<count){
  /* Keep the single label beside its selected icon, outside neighboring wedges. */
  float angle=wheelAngle(choice),dx=cosf(angle),dy=sinf(angle);const char*name=itemName(ids[choice]);float width=textWidth(name);
  float x=cx+dx*(r+16),y=cy+dy*(r+16)+6;
  if(dx>.55f)x+=8;else if(dx<-.55f)x-=width+8;else x-=width*.5f;
  x=fmaxf(12,fminf(vw-width-12,x));y=fmaxf(24,fminf(vh-12,y));
  glColor4f(0,0,0,.7f);text(x+1,y+1,name);glColor4f(1,1,1,1);text(x,y,name);
 }
 glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glPopAttrib();useProgram(prog);activeTexture(active);
}
