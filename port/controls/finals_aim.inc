#define AIM_ADS_GAIN .60
typedef struct {double velocity[2],remainder[2];void*owner;int stage,timer;double boost;int boostDirection,ads;} PrecisionAimState;
static double aimIntegrate(double target,double dt,double*velocity){
 *velocity=target;return target*dt;
}
static double aimFovScale(double fov){return tan(fov*3.141592653589793/360.)/tan(3.141592653589793/6.);}
static void aimResponse(int rawX,int rawY,int deadzone,int invert,int sensX,int sensY,double fov,double dt,double out[2],PrecisionAimState*state){
 double x=rawX,y=rawY,length=hypot(x,y);if(deadzone<0)deadzone=0;if(deadzone>30000)deadzone=30000;
 double strength=length>deadzone?fmin(1,(length-deadzone)/(32767.-deadzone)):0;
 if(length>0){x=x/length*strength;y=y/length*strength;}else x=y=0;
 /* SDL down is positive; preserve the installed profile's pitch convention. */
 y*=invert?-1:1;double input[2]={x,y},sensitivity[2]={sensX/100.,sensY/100.};
 for(int i=0;i<2;i++){double v=input[i];double target=v*fabs(v)*(i?170.:200.)*(sensitivity[i]/(i?.60:.65))*aimFovScale(fov)*AIM_ADS_GAIN;out[i]=aimIntegrate(target,dt,&state->velocity[i]);if(!target)state->remainder[i]=0;}
}
static void aimLookResponse(int x,int y,int dz,int invert,int sx,int sy,double fov,double dt,int ads,double delta[2],PrecisionAimState*state){
 aimResponse(x,y,dz,invert,sx,sy,fov,dt,delta,state);
 if(state->ads!=ads){state->boost=0;state->boostDirection=0;}state->ads=ads;
 if(!ads){delta[0]/=AIM_ADS_GAIN;delta[1]/=AIM_ADS_GAIN;
  /* An outer-stick turn boost, integrated exactly through the .5 s ramp. */
  int direction=x>0?1:x<0?-1:0;double strength=fabs((double)x)/32767.;
  if(strength>.95){if(direction!=state->boostDirection)state->boost=0;state->boostDirection=direction;double a=state->boost,b=fmin(.5,a+dt);double area=(b*b-a*a)/(2*.5)+fmax(0,dt-(b-a));delta[0]+=direction*220.*area*aimFovScale(fov);state->boost=b;}
  else {state->boost=0;state->boostDirection=0;}
 }else{state->boost=0;state->boostDirection=0;}
}
