WHEEL_METHOD void wheelUpdate(int lb,int back,float x,float y){
 if(!playing()){openWheel=0;previousLB=lb;cancelled=0;return;}
 if(lb&&!previousLB){
  inventory();oldCurrent=current();choice=-1;
  for(int i=0;i<count;i++)if(ids[i]==oldCurrent)choice=i;
  openWheel=1;cancelled=0;wheelStickEngaged=0;
  wheelOpened();
 }
 if(openWheel){
  if(x*x+y*y>.16f*.16f)wheelStickEngaged=1;
  if(back){cancelled=1;openWheel=0;}
  else if(x*x+y*y<.22f*.22f&&wheelStickEngaged)choice=-1;
  else if(x*x+y*y>.28f*.28f&&count){
   wheelStickEngaged=1;
   float angle=atan2f(y,x);int target=wheelTarget(angle);
   if(target<0)choice=-1;
   else if(target!=choice){float start,end;wheelBounds(target,&start,&end);float wrapped=angle;while(wrapped<start)wrapped+=6.28318530718f;while(wrapped>start+6.28318530718f)wrapped-=6.28318530718f;
    if(choice<0||(wrapped>start+.06f&&wrapped<end-.06f))choice=target;
   }
  }
 }
 if(!lb&&previousLB){
  if(openWheel&&!cancelled&&choice>=0&&choice<count&&available(ids[choice])&&wheelStickEngaged&&ids[choice]!=current())equip(ids[choice]);
  openWheel=0;cancelled=0;
 }
 previousLB=lb;
}
