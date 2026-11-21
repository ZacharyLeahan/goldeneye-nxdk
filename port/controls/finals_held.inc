static void padHeldFace(float out[24][3],float vw,float vh,float blend){
 float ease=1-(1-blend)*(1-blend),cx=vw*.75f,cy=vh-55+(1-ease)*180;
 for(int i=0;i<8;i++){float a=6.28318530718f*(i+.5f)/8,x=cosf(a),y=sinf(a);out[i][0]=(cx+128*x+36*y)/vw;out[i][1]=(cy+62*y-18*x)/vh;out[i][2]=1+.13f*y;out[i+8][0]=(cx+124*x+35*y)/vw;out[i+8][1]=(cy+62*y-18*x+19)/vh;out[i+8][2]=out[i][2];out[i+16][0]=(cx+92*x+26*y)/vw;out[i+16][1]=(cy+45*y-13*x-6)/vh;out[i+16][2]=out[i][2];}
}
