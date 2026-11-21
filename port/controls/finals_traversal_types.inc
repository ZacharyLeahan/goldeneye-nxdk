#define MANTLE_BUFFER 18.f
#define MANTLE_EARLIEST_GRAB 8.f
#define MANTLE_REACH 62.f
/* Measured from airborne feet. The standing eye is about 167 units high. */
#define MANTLE_HEIGHT 235.f
#define MANTLE_DURATION 32.f
#define MANTLE_LOW_STEP 65.f
#define MANTLE_LOW_DURATION 18.f
#define MANTLE_RADIUS 30.f
#define MANTLE_LIP_ADJUST 24.f
#define MANTLE_BARRIER_SPAN 128.f
#define MANTLE_BARRIER_DROP 175.f

typedef struct {
 void *owner;
 int stage,clock,press,armed,active,found,clear,chose,forward,airPress;
 float age,elapsed,entrySpeed;
 JumpVec start,liftStart,land;
 float adjustTime,barrierTop,duration;
 int crossingBarrier;
} XboxMantle;

typedef struct {void*owner;int stage,timer,active,starts,blocked,ready;float elapsed,cooldown,entry,dx,dz,lastX,lastZ,lastForward,lastSide;} XboxSlide;
