/* Stable gadget quadrants: right = grenades, bottom = jump pad.
 * Owned guns share the upper/left half without moving the gadget directions. */
WHEEL_METHOD void wheelBounds(int index,float*start,float*end){
 const float pi=3.14159265359f;int id=ids[index];
 if(id==PAD_ITEM){*start=pi*.25f;*end=pi*.75f;return;}
 if(id==GRENADE_ITEM){*start=-pi*.25f;*end=pi*.25f;return;}
 int guns=0,rank=0;for(int i=0;i<count;i++)if(ids[i]!=PAD_ITEM&&ids[i]!=GRENADE_ITEM){if(i<index)rank++;guns++;}
 *start=pi*.75f+rank*pi/guns;*end=*start+pi/guns;
}
WHEEL_METHOD float wheelAngle(int index){float start,end;wheelBounds(index,&start,&end);if(ids[index]!=PAD_ITEM&&ids[index]!=GRENADE_ITEM){int guns=0;for(int i=0;i<count;i++)if(ids[i]!=PAD_ITEM&&ids[i]!=GRENADE_ITEM)guns++;if(guns==1)return 4.71238898038f;}return(start+end)*.5f;}
WHEEL_METHOD int wheelTarget(float angle){for(int i=0;i<count;i++){float start,end,a=angle;wheelBounds(i,&start,&end);while(a<start)a+=6.28318530718f;if(a<=end)return i;}return -1;}
