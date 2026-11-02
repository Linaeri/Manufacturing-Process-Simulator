#include "factory_view.h"
#include <math.h>
#include "ui.h"

// ---- 방과 카메라 치수(미터, 라디안) / Room and camera dimensions (metres, radians) ----
#define ROOM_HW 17.0f      // 방 너비의 절반 / half room width  (x)
#define ROOM_HD 14.0f      // 방 깊이의 절반 / half room depth  (z)
#define ROOM_H 10.0f       // 벽 높이 / wall height
#define CAM_MARGIN 0.6f    // 카메라와 벽·천장·기계 사이 최소 간격 / minimum gap to walls, ceiling and presses
#define MIN_DIST 3.5f
#define MAX_DIST 24.0f
#define PITCH_MIN 0.14f    // 약 8°: 바닥 아래로 내려가지 않음 / ~8°: never below the focus height
#define PITCH_MAX 1.35f    // 약 77°
#define FOVY 60.0f
#define SUPERSAMPLE 2      // 2배 해상도로 그려 가장자리를 부드럽게 / render at 2x for smoother edges

static const float STATION_X[MACHINES]={-7,0,7};
#define PRESS_HX 1.4f      // 프레스 충돌 상자 / press obstacle box: half x, half z, height
#define PRESS_HZ 1.0f
#define PRESS_TOP 5.4f

static Color bgColor={9,14,20,255},accentColor={65,222,177,255};

void FactoryFocus(FactoryView *v,int machine){
 OrbitCam *c=&v->orbit;
 if(machine<0||machine>=MACHINES){c->focusGoal=v3(0,1.5f,-.5f);c->yaw=0;c->pitch=.6f;c->dist=MAX_DIST;}
 else{c->focusGoal=v3(STATION_X[machine],2.0f,-.5f);c->dist=9;if(c->pitch<.3f)c->pitch=.45f;}
}
void FactoryInit(FactoryView *v,int width,int height){
 *v=(FactoryView){.width=width,.height=height};
 v->target=LoadRenderTexture(width*SUPERSAMPLE,height*SUPERSAMPLE);
 SetTextureFilter(v->target.texture,TEXTURE_FILTER_BILINEAR);
 // 카메라: 방 안에 머물고 프레스 세 대를 피합니다. / Camera stays inside the hall and clear of the three presses.
 OrbitCam *c=&v->orbit;
 c->cam=(Camera3D){.up={0,1,0},.fovy=FOVY,.projection=CAMERA_PERSPECTIVE};
 c->lo=v3(-ROOM_HW,0,-ROOM_HD);c->hi=v3(ROOM_HW,ROOM_H,ROOM_HD);
 c->margin=CAM_MARGIN;c->minDist=MIN_DIST;c->maxDist=MAX_DIST;c->pitchMin=PITCH_MIN;c->pitchMax=PITCH_MAX;
 for(int i=0;i<MACHINES;i++)c->obstacles[c->obstacleCount++]=(Box3){v3(STATION_X[i]-PRESS_HX,0,-PRESS_HZ),v3(STATION_X[i]+PRESS_HX,PRESS_TOP,PRESS_HZ)};
 FactoryFocus(v,-1);OrbitSnap(c);
}
void FactoryUnload(FactoryView *v){UnloadRenderTexture(v->target);}
void FactoryInput(FactoryView *v,Rectangle area,float dt){OrbitUpdate(&v->orbit,area,dt);}

// ---- 방 / Room ----
#define DOOR_HW 3.0f   // 문 너비의 절반 / half door width
#define DOOR_H 5.2f
// 문 밖의 배경: 마당, 울타리, 나무, 언덕, 하늘. 문이 열릴 때 보입니다.
// Outdoor backdrop seen through the open door: yard, fence, trees, hills and sky.
static void drawOutside(void){
 float z0=-ROOM_HD-.4f;
 box(v3(0,-.06f,z0-25),v3(80,.1f,50),(Color){74,77,82,255});                          // 마당 / yard
 box(v3(0,-.05f,-ROOM_HD-.2f),v3(2*DOOR_HW,.1f,.4f),(Color){60,64,70,255});           // 문턱 / threshold
 for(float x=-12;x<=12;x+=4)box(v3(x,.012f,z0-9),v3(.12f,.012f,5),(Color){220,200,90,255}); // 주차선 / parking lines
 box(v3(-9,1.6f,z0-10),v3(2.5f,3.0f,7.5f),(Color){176,182,190,255});                 // 트레일러 / lorry trailer
 box(v3(-9,1.4f,z0-5.4f),v3(2.4f,2.2f,1.8f),(Color){52,98,150,255});                 // 트럭 머리 / lorry cab
 for(float x=-38;x<=38;x+=2.5f)box(v3(x,.8f,z0-30),v3(.08f,1.6f,.08f),(Color){120,126,132,255}); // 울타리 / fence
 box(v3(0,1.5f,z0-30),v3(78,.06f,.06f),(Color){120,126,132,255});
 for(int i=0;i<14;i++){float x=-36+i*5.6f+((i*37)%5)*.6f,zz=z0-34-((i*53)%7),h=4+((i*29)%4);
  DrawCylinderEx(v3(x,0,zz),v3(x,1.4f,zz),.25f,.25f,8,(Color){92,68,48,255});
  DrawCylinderEx(v3(x,1.2f,zz),v3(x,1.2f+h,zz),1.6f,0,10,(Color){(unsigned char)(48+(i*13)%30),(unsigned char)(104+(i*7)%30),62,255});}
 for(int i=0;i<6;i++){float x=-50+i*20,w=26+(i%3)*6,h=7+(i%2)*5;box(v3(x,h/2,z0-52),v3(w,h,4),(Color){92,118,112,255});} // 언덕 / hills
 Color sky[6]={{198,214,224,255},{180,204,222,255},{160,192,220,255},{140,180,216,255},{122,166,210,255},{106,152,204,255}};
 for(int i=0;i<6;i++)box(v3(0,i*7+3.5f,z0-58),v3(160,7,1),sky[i]);                    // 하늘 / sky bands
}
static void drawRoom(const FactoryView *v){
 Color floorC={50,56,62,255},wallC={88,98,108,255},bandC={60,68,76,255},windowC={128,160,180,255},trussC={40,46,52,255},lineC={208,170,52,255};
 drawOutside();
 box(v3(0,-.05f,0),v3(2*ROOM_HW,.1f,2*ROOM_HD),floorC);
 for(float x=-ROOM_HW+2;x<ROOM_HW;x+=2)DrawLine3D(v3(x,.004f,-ROOM_HD),v3(x,.004f,ROOM_HD),shade(floorC,1.18f));
 for(float z=-ROOM_HD+2;z<ROOM_HD;z+=2)DrawLine3D(v3(-ROOM_HW,.004f,z),v3(ROOM_HW,.004f,z),shade(floorC,1.18f));
 // 통로 표시선 / walkway lines
 box(v3(0,.012f,5.0f),v3(2*ROOM_HW-1,.012f,.14f),lineC);box(v3(0,.012f,-7.0f),v3(2*ROOM_HW-1,.012f,.14f),lineC);
 // 벽: 안쪽 면이 정확히 방 경계에 오도록 바깥에 둡니다. 뒷벽에는 문 구멍이 있습니다.
 // Walls sit outside so their inner face is exactly on the room boundary; the back wall has a door opening.
 float t=.4f,side=ROOM_HW-DOOR_HW;
 box(v3(ROOM_HW+t/2,ROOM_H/2,0),v3(t,ROOM_H,2*ROOM_HD+2*t),wallC);box(v3(-ROOM_HW-t/2,ROOM_H/2,0),v3(t,ROOM_H,2*ROOM_HD+2*t),wallC);
 box(v3(0,ROOM_H/2,ROOM_HD+t/2),v3(2*ROOM_HW,ROOM_H,t),wallC);
 box(v3(-(DOOR_HW+side/2),ROOM_H/2,-ROOM_HD-t/2),v3(side,ROOM_H,t),wallC);box(v3(DOOR_HW+side/2,ROOM_H/2,-ROOM_HD-t/2),v3(side,ROOM_H,t),wallC);
 box(v3(0,(DOOR_H+ROOM_H)/2,-ROOM_HD-t/2),v3(2*DOOR_HW,ROOM_H-DOOR_H,t),wallC);
 // 아래 띠와 높은 창 / lower band and high windows
 float b=.05f;
 box(v3(ROOM_HW-b/2,.6f,0),v3(b,1.2f,2*ROOM_HD),bandC);box(v3(-ROOM_HW+b/2,.6f,0),v3(b,1.2f,2*ROOM_HD),bandC);
 box(v3(0,.6f,ROOM_HD-b/2),v3(2*ROOM_HW,1.2f,b),bandC);
 box(v3(-(DOOR_HW+side/2),.6f,-ROOM_HD+b/2),v3(side,1.2f,b),bandC);box(v3(DOOR_HW+side/2,.6f,-ROOM_HD+b/2),v3(side,1.2f,b),bandC);
 for(float x=-13.5f;x<=13.5f;x+=4.5f){box(v3(x,ROOM_H-2.1f,-ROOM_HD+b/2),v3(3,1.2f,b),windowC);box(v3(x,ROOM_H-2.1f,ROOM_HD-b/2),v3(3,1.2f,b),windowC);}
 for(float z=-10;z<=10;z+=4){box(v3(-ROOM_HW+b/2,ROOM_H-2.1f,z),v3(b,1.2f,2.6f),windowC);box(v3(ROOM_HW-b/2,ROOM_H-2.1f,z),v3(b,1.2f,2.6f),windowC);}
 // 롤러 셔터 문: 열리면 위의 드럼으로 말려 올라갑니다. / Roller shutter: rolls up into the drum above when open.
 float hgt=DOOR_H*(1-v->door);
 if(hgt>.02f){box(v3(0,DOOR_H-hgt/2,-ROOM_HD+.04f),v3(2*DOOR_HW,hgt,.08f),(Color){72,80,88,255});
  for(float y=DOOR_H-.35f;y>DOOR_H-hgt;y-=.65f)box(v3(0,y,-ROOM_HD+.09f),v3(2*DOOR_HW-.2f,.06f,.04f),(Color){58,64,70,255});}
 DrawCylinderEx(v3(-DOOR_HW-.1f,DOOR_H+.35f,-ROOM_HD+.35f),v3(DOOR_HW+.1f,DOOR_H+.35f,-ROOM_HD+.35f),.32f,.32f,14,(Color){96,104,112,255});
 box(v3(-DOOR_HW-.2f,2.6f,-ROOM_HD+.1f),v3(.25f,5.4f,.2f),lineC);box(v3(DOOR_HW+.2f,2.6f,-ROOM_HD+.1f),v3(.25f,5.4f,.2f),lineC);
 // 문 옆 신호등: 열려 있으면 초록 / door signal light, green while open
 DrawSphere(v3(DOOR_HW+.2f,5.6f,-ROOM_HD+.25f),.14f,v->door>.95f?(Color){60,220,110,255}:v->door>.02f?(Color){240,170,40,255}:(Color){220,60,50,255});
 // 모서리 기둥(카메라 간격보다 얇음) / corner columns, thinner than the camera margin
 float p=.4f;
 for(int sx=-1;sx<=1;sx+=2)for(int sz=-1;sz<=1;sz+=2)box(v3(sx*(ROOM_HW-p/2),ROOM_H/2,sz*(ROOM_HD-p/2)),v3(p,ROOM_H,p),shade(wallC,.8f));
 // 천장 트러스와 조명(카메라 최대 높이보다 위) / roof trusses and lights, above the camera's highest point
 for(float z=-12;z<=12;z+=4){box(v3(0,ROOM_H-.2f,z),v3(2*ROOM_HW,.3f,.3f),trussC);
  for(float x=-12;x<=12;x+=6)box(v3(x,ROOM_H-.38f,z),v3(1.8f,.06f,.35f),(Color){235,228,200,255});}
}

// ---- 기계 / Machines ----
static Color statusColor(const PanelState *s,int i){
 const Machine *m=&s->machines[i];
 if(i>=s->owned)return (Color){90,100,110,255};
 if(m->wear>=s->cfg->wearLimit)return (Color){230,70,60,255};
 if(!m->enabled)return (Color){120,130,140,255};
 if(!s->running)return (Color){150,170,190,255};
 if(m->busy)return accentColor;
 if(m->input>0)return (Color){150,170,190,255};
 return (Color){240,170,40,255};
}
// 고장·자재 없음일 때 상태등이 깜박입니다. / The beacon blinks on a fault or when out of sheets.
static Color beaconColor(const PanelState *s,int i,float time){
 const Machine *m=&s->machines[i];Color c=statusColor(s,i);
 bool blink=i<s->owned&&s->running&&(m->wear>=s->cfg->wearLimit||(m->enabled&&!m->busy&&m->input==0));
 return (blink&&fmodf(time,.8f)<.4f)?shade(c,.3f):c;
}

// ---- 애니메이션: 한 사이클의 단계(progress 0..1) ----
// ---- Animation: phases of one cycle (progress 0..1) ----
// 모든 움직임은 시뮬레이션의 progress로 계산되므로 통계와 항상 일치합니다.
// Every movement is computed from the simulation's progress, so it always matches the statistics.
#define P_LIFT .08f   // 팔레트에서 컨베이어로 / pallet onto the conveyor
#define P_BELT .20f   // 컨베이어 끝까지 / to the end of the conveyor
#define P_FEED .25f   // 기계 안으로 / into the machine
#define P_DOWN .45f   // 램 하강(성형: 롤러 통과 시작) / ram comes down (forming: sheet enters the rollers)
#define P_HOLD .52f   // 가압 유지 / press holds
#define P_WORK .70f   // 램 상승 또는 롤 성형 완료 / ram back up, or roll forming finished
#define P_EXIT .85f   // 출구 테이블로, 그 뒤 통으로 / onto the exit table, then into a bin
#define RAM_TOP 3.7f
#define RAM_BOTTOM 1.87f  // 상부 금형이 시트에 닿는 높이 / height where the upper die meets the sheet
static float ramHeight(const Machine *m){
 float p=m->busy?(float)m->progress:0;
 if(p<P_FEED)return RAM_TOP;
 if(p<P_DOWN)return lerpf(RAM_TOP,RAM_BOTTOM,seg(p,P_FEED,P_DOWN));
 if(p<P_HOLD)return RAM_BOTTOM;
 return lerpf(RAM_BOTTOM,RAM_TOP,seg(p,P_HOLD,P_WORK));
}
// 통에는 지게차가 아직 가져가지 않은 부품만 쌓입니다. / Bins show only parts the forklift hasn't collected yet.
static float goodFill(const FactoryView *v,const Machine *m,int i){return fminf(m->good-v->collectedGood[i],24)/24.0f*.72f;}
static float badFill(const FactoryView *v,const Machine *m,int i){return fminf(m->defects-v->collectedBad[i],10)/10.0f*.72f;}
// 팔레트 높이 = 시뮬레이션의 그 기계 팔레트 시트 수. / Pallet height = that machine's pallet count in the simulation.
static float stackTop(const PanelState *s,int i){int n=s->machines[i].input;if(n>40)n=40;return .14f+.025f*(n>0?n:0);}

static void drawPress(float cx,Color body,Color beacon,float ramY){
 Color steel={120,130,140,255},dark={45,52,60,255};
 box(v3(cx,.55f,0),v3(2.6f,1.1f,1.8f),body);                       // 베드 / bed
 box(v3(cx,1.18f,0),v3(2.0f,.16f,1.4f),steel);                     // 볼스터 / bolster plate
 box(v3(cx-1.05f,2.85f,0),v3(.42f,3.5f,1.3f),body);box(v3(cx+1.05f,2.85f,0),v3(.42f,3.5f,1.3f),body); // 기둥 / columns
 box(v3(cx,4.95f,0),v3(2.8f,.9f,1.7f),body);                       // 크라운 / crown
 DrawCylinderEx(v3(cx,4.5f,0),v3(cx,4.15f,0),.34f,.34f,14,shade(steel,.8f));        // 실린더 / hydraulic cylinder
 DrawCylinderEx(v3(cx,4.2f,0),v3(cx,ramY+.3f,0),.18f,.18f,12,(Color){200,205,210,255}); // 피스톤 로드 / piston rod
 box(v3(cx,ramY,0),v3(1.7f,.6f,1.2f),shade(body,.75f));             // 램 / ram
 box(v3(cx,ramY-.38f,0),v3(1.5f,.16f,1.0f),dark);                  // 상부 금형 / upper die
 box(v3(cx,1.32f,0),v3(1.5f,.12f,1.0f),dark);                      // 하부 금형 / lower die
 DrawCylinderEx(v3(cx+1.15f,5.4f,.55f),v3(cx+1.15f,5.75f,.55f),.13f,.13f,10,beacon); // 상태등 / status beacon
}
// 회전하는 롤러: 표면의 줄무늬가 각도를 보여 줍니다. / A turning roller; the stripe on its surface shows the angle.
static void roller(float cx,float y,float z,float r,float angle,Color col){
 DrawCylinderEx(v3(cx-1.0f,y,z),v3(cx+1.0f,y,z),r,r,16,col);
 for(int k=0;k<2;k++){float a=angle+k*PI;box(v3(cx,y+r*cosf(a),z+r*sinf(a)),v3(2.02f,.07f,.07f),shade(col,.55f));}
}
static void drawFormer(float cx,Color body,Color beacon,float angle){
 Color roll={190,196,202,255},guard={214,120,40,255};
 box(v3(cx,.45f,0),v3(2.6f,.9f,1.8f),body);                        // 받침 / base
 box(v3(cx-1.15f,1.8f,0),v3(.3f,1.8f,1.5f),body);box(v3(cx+1.15f,1.8f,0),v3(.3f,1.8f,1.5f),body); // 측판 / side plates
 roller(cx,1.3f,0,.32f,angle,roll);                                // 아래 롤러 / lower roller
 roller(cx,2.0f,0,.32f,-angle,roll);                               // 위 롤러(반대 방향) / upper roller, turning the other way
 roller(cx,1.32f,.72f,.2f,angle*1.6f,shade(roll,.85f));            // 굽힘 롤러 / bending roller
 box(v3(cx-1.6f,1.4f,.3f),v3(.5f,.8f,.8f),shade(body,.8f));        // 모터 / motor
 box(v3(cx,2.85f,0),v3(2.6f,.3f,1.6f),guard);                      // 보호 덮개 / safety guard
 DrawCylinderEx(v3(cx+1.15f,3.0f,.55f),v3(cx+1.15f,3.35f,.55f),.13f,.13f,10,beacon);
}
// 작업 중인 시트/부품: 팔레트 → 컨베이어 → 기계 → 출구 테이블 → 통 위로 들어 올린 뒤 똑바로 떨어뜨림.
// The sheet or part in progress: pallet -> conveyor -> machine -> exit table -> lifted over the bin, then dropped straight down.
#define P_OVER .93f
static void drawWorkpiece(const FactoryView *v,const PanelState *s,int i){
 const Machine *m=&s->machines[i];if(!m->busy)return;
 float cx=STATION_X[i],p=(float)m->progress;bool form=s->process,bad=m->pendingDefect;
 Vector3 pallet=v3(cx,stackTop(s,i)+.03f,-5.2f),belt0=v3(cx,.88f,-4.1f),belt1=v3(cx,.88f,form?-1.5f:-1.2f);
 Vector3 work=form?v3(cx,1.65f,-.9f):v3(cx,1.395f,0),workEnd=form?v3(cx,1.65f,.95f):work;
 float bx=cx+(bad?.85f:-.85f),fill=bad?badFill(v,m,i):goodFill(v,m,i);
 // 통 벽(0.9 m) 위로 충분히 띄운 지점과 쌓인 더미 위 착지점 / a point safely above the 0.9 m bin walls, and the landing spot on the pile
 Vector3 table=v3(cx,.82f,1.6f),over=v3(bx,1.25f,3.2f),land=v3(bx,.1f+fill,3.2f);
 Vector3 pos;
 if(p<P_LIFT){float t=seg(p,0,P_LIFT);pos=lerp3(pallet,belt0,t);pos.y+=sinf(t*PI)*.35f;}
 else if(p<P_BELT)pos=lerp3(belt0,belt1,(p-P_LIFT)/(P_BELT-P_LIFT));
 else if(p<P_FEED){float t=seg(p,P_BELT,P_FEED);pos=lerp3(belt1,work,t);pos.y+=sinf(t*PI)*.4f;}
 else if(p<P_WORK)pos=form?lerp3(work,workEnd,(p-P_FEED)/(P_WORK-P_FEED)):work;
 else if(p<P_EXIT){float t=seg(p,P_WORK,P_EXIT);pos=lerp3(workEnd,table,t);pos.y+=sinf(t*PI)*.3f;}
 else if(p<P_OVER){float t=seg(p,P_EXIT,P_OVER);pos=lerp3(table,over,t);pos.y=lerpf(table.y,over.y,sqrtf(t));} // 먼저 올라감 / rises first
 else{float t=(p-P_OVER)/(1-P_OVER);pos=lerp3(over,land,t*t);}                          // 낙하 / falls, accelerating
 // 모양: 프레스는 가압 순간 각인, 성형은 롤러를 지나며 옆면이 올라옴.
 // Shape: the press stamps it at the bottom of the stroke; forming raises the sides after the rollers.
 float shaped=form?seg(p,.55f,P_WORK):(p>=P_DOWN?1:0);
 Color plain={176,186,196,255},done=form?(Color){200,170,120,255}:(Color){190,198,206,255},col=plain;
 if(shaped>.5f)col=bad?(Color){168,96,72,255}:done; // 불량은 가공 후 녹슨 색 / defects show a rusty colour once made
 if(!form){box(pos,v3(1.0f,.03f,.7f),col);if(shaped>0)box(v3(pos.x,pos.y+.04f,pos.z),v3(.5f,.05f,.35f),shade(col,.9f));}
 else{float w=.7f-.2f*shaped,h=.25f*shaped;box(pos,v3(1.0f,.03f,w),col);
  if(h>.01f){box(v3(pos.x,pos.y+h/2,pos.z-w/2+.015f),v3(1.0f,h,.03f),col);box(v3(pos.x,pos.y+h/2,pos.z+w/2-.015f),v3(1.0f,h,.03f),col);}}
}
static void drawStation(const FactoryView *v,const PanelState *s,int i,bool selected){
 float cx=STATION_X[i];bool owned=i<s->owned;Color st=statusColor(s,i);
 box(v3(cx,.006f,-1.1f),v3(3.9f,.012f,10.4f),owned?(Color){58,65,72,255}:(Color){46,51,57,255});
 floorFrame(cx,-1.1f,3.9f,10.4f,selected?.16f:.08f,selected?accentColor:owned?(Color){190,150,50,255}:(Color){80,86,92,255});
 if(!owned){ // 잠긴 자리: 윤곽만 표시 / locked slot: outline only
  Color g={95,105,115,255};
  DrawCubeWiresV(v3(cx,2.7f,0),v3(2.8f,5.4f,1.8f),g);DrawCubeWiresV(v3(cx,.5f,-2.65f),v3(1.0f,1.0f,3.1f),g);
  DrawCubeWiresV(v3(cx,.5f,3.2f),v3(2.9f,1.0f,1.0f),g);return;}
 const Machine *m=&s->machines[i];
 // 원자재 팔레트 / material pallet
 box(v3(cx,.07f,-5.2f),v3(1.6f,.14f,1.4f),(Color){112,82,52,255});
 float top=stackTop(s,i);if(top>.15f)box(v3(cx,(.14f+top)/2,-5.2f),v3(1.4f,top-.14f,1.2f),(Color){176,186,196,255});
 // 컨베이어: 기계가 일할 때 벨트 줄무늬가 움직입니다 / conveyor; belt stripes move while the machine works
 box(v3(cx,.82f,-2.65f),v3(1.0f,.08f,3.1f),(Color){34,37,41,255});
 for(int k=0;k<8;k++){float z=-4.2f+fmodf(v->belt[i]+k*3.1f/8,3.1f);box(v3(cx,.866f,z),v3(.95f,.012f,.06f),(Color){62,67,73,255});}
 box(v3(cx-.55f,.72f,-2.65f),v3(.1f,.26f,3.1f),(Color){92,102,112,255});box(v3(cx+.55f,.72f,-2.65f),v3(.1f,.26f,3.1f),(Color){92,102,112,255});
 for(int k=0;k<2;k++){float z=k?-1.4f:-3.9f;box(v3(cx-.5f,.3f,z),v3(.1f,.6f,.1f),(Color){70,78,86,255});box(v3(cx+.5f,.3f,z),v3(.1f,.6f,.1f),(Color){70,78,86,255});}
 Color beacon=beaconColor(s,i,v->time);
 if(s->process)drawFormer(cx,(Color){74,92,104,255},beacon,v->roll[i]);else drawPress(cx,(Color){58,96,132,255},beacon,ramHeight(m));
 drawWorkpiece(v,s,i);
 // 출구 테이블과 통(둘 다 안쪽 1.08 m, 부품 1.0 m): 양품(녹색), 불량(빨강)
 // Exit table and bins, both 1.08 m inside for 1.0 m parts: good (green), reject (red)
 box(v3(cx,.75f,1.6f),v3(1.4f,.1f,1.0f),(Color){92,102,112,255});
 bin(v3(cx-.85f,0,3.2f),v3(1.2f,.9f,1.0f),(Color){62,138,92,255});
 bin(v3(cx+.85f,0,3.2f),v3(1.2f,.9f,1.0f),(Color){150,62,56,255});
 float g=goodFill(v,m,i),d=badFill(v,m,i);
 Color part=s->process?(Color){200,170,120,255}:(Color){190,198,206,255};
 if(g>0)box(v3(cx-.85f,.06f+g/2,3.2f),v3(1.05f,g,.85f),part);
 if(d>0)box(v3(cx+.85f,.06f+d/2,3.2f),v3(1.05f,d,.85f),(Color){168,96,72,255});
 // 제어반 / control pedestal
 box(v3(cx+1.75f,.7f,1.3f),v3(.4f,1.4f,.3f),(Color){70,78,86,255});box(v3(cx+1.75f,1.25f,1.46f),v3(.3f,.25f,.03f),st);
}

// ---- 작업자와 지게차 / Worker and forklift ----
#define ACT_DROP 100   // +기계 번호: 팔레트에 시트 내려놓기 / + machine: put sheets on that pallet
#define ACT_PICK 200   // +기계 번호: 통 비우기 / + machine: empty that station's bins
#define LANE_WORKER (-1.5f)  // 문을 지나는 각자의 차선 / separate lanes through the door
#define LANE_FORK 1.5f
#define PARK_X 7.0f    // 지게차가 쉬는 마당 위치 / where the forklift waits in the yard
#define PARK_Z (-ROOM_HD-8)
static void route(FactoryAgent *a,float x,float z,int act){if(a->n<AGENT_POINTS){a->pts[a->n]=(Vector2){x,z};a->act[a->n++]=act;}}
static void startWorker(FactoryView *v,const int *need,int n){
 FactoryAgent *a=&v->worker;*a=(FactoryAgent){.x=-8,.z=-ROOM_HD-5,.yaw=PI/2,.active=true,.carrying=true};
 route(a,LANE_WORKER,-ROOM_HD-5,0);route(a,LANE_WORKER,-ROOM_HD+1,0);route(a,LANE_WORKER,-7,0);
 for(int k=0;k<n;k++){float x=STATION_X[need[k]];route(a,x,-7,0);route(a,x,-6.25f,ACT_DROP+need[k]);route(a,x,-7,0);}
 route(a,LANE_WORKER,-7,0);route(a,LANE_WORKER,-ROOM_HD+1,0);route(a,LANE_WORKER,-ROOM_HD-5,0);route(a,-8,-ROOM_HD-5,0);
}
static void startForklift(FactoryView *v,const int *stop,int n){
 FactoryAgent *a=&v->forklift;float x=a->x,z=a->z,yaw=a->yaw;
 *a=(FactoryAgent){.x=x,.z=z,.yaw=yaw,.active=true};
 route(a,LANE_FORK,PARK_Z+2,0);route(a,LANE_FORK,-ROOM_HD+2,0);route(a,LANE_FORK,-9.5f,0);route(a,3.5f,-9.5f,0);route(a,3.5f,6.8f,0);
 for(int k=0;k<n;k++){float cx=STATION_X[stop[k]];route(a,cx,6.8f,0);route(a,cx,6.1f,ACT_PICK+stop[k]);route(a,cx,6.8f,0);}
 route(a,3.5f,6.8f,0);route(a,3.5f,-9.5f,0);route(a,LANE_FORK,-9.5f,0);route(a,LANE_FORK,-ROOM_HD+2,0);route(a,LANE_FORK,PARK_Z+2,0);route(a,PARK_X,PARK_Z,0);
}
// 한 경로점씩 이동. 문 앞에서는 문이 열릴 때까지 기다립니다.
// Move one waypoint at a time; wait at the doorway until the door is open.
static void stepAgent(FactoryView *v,PanelState *s,FactoryAgent *a,float dt,float speed,float turn,bool fork){
 if(!a->active)return;
 if(a->wait>0){a->wait-=dt;if(a->wait<=0)a->doing=0;return;}
 if(a->k>=a->n){a->active=false;return;}
 Vector2 t=a->pts[a->k];float dx=t.x-a->x,dz=t.y-a->z,d=sqrtf(dx*dx+dz*dz);
 if(fabsf(a->z+ROOM_HD)<2.5f&&v->door<.92f)return;
 if(d<.03f){
  a->x=t.x;a->z=t.y;int act=a->act[a->k++];
  if(act>=ACT_PICK){int i=act-ACT_PICK;const Machine *m=&s->machines[i];
   a->load+=m->good-v->collectedGood[i]+m->defects-v->collectedBad[i];
   v->collectedGood[i]=m->good;v->collectedBad[i]=m->defects;a->carrying=true;a->wait=1.8f;a->doing=act;}
  else if(act>=ACT_DROP){int i=act-ACT_DROP,left=1;
   for(int k=a->k;k<a->n;k++)left+=a->act[k]>=ACT_DROP&&a->act[k]<ACT_PICK;
   PanelDeliver(s,i,(s->stock+left-1)/left);   // 창고에서 팔레트로, 남은 정류장과 공평하게 / store to pallet, shared fairly with the remaining stops
   a->wait=.8f;a->doing=act;a->carrying=left>1;}
  return;}
 // 지게차는 작업자가 앞에 있으면 멈춥니다. / The forklift stops if the worker is in front of it.
 if(fork&&v->worker.active){float fx=a->x+sinf(a->yaw)*2.2f,fz=a->z+cosf(a->yaw)*2.2f,wx=v->worker.x-fx,wz=v->worker.z-fz;if(wx*wx+wz*wz<2.6f*2.6f)return;}
 // 방향을 먼저 맞춘 뒤 이동(지게차는 제자리 회전). / Turn towards the target first; the forklift turns on the spot.
 float diff=wrapAngle(atan2f(dx,dz)-a->yaw),r=turn*dt;
 a->yaw=wrapAngle(a->yaw+(fabsf(diff)<r?diff:(diff>0?r:-r)));
 if(fabsf(diff)>(fork?.12f:.6f))return;
 float step=fminf(d,speed*dt);a->x+=dx/d*step;a->z+=dz/d*step;a->phase+=step*4.5f;
}
static void updateAgents(FactoryView *v,PanelState *s,float dt){
 // 새 게임(또는 첫 프레임): 표시 상태를 실제 상태에 맞춥니다. / New game or first frame: sync the display with the real state.
 if(!v->synced||s->supplied<v->lastSupplied){
  for(int i=0;i<MACHINES;i++){v->collectedGood[i]=s->machines[i].good;v->collectedBad[i]=s->machines[i].defects;}
  v->worker.active=false;v->forklift=(FactoryAgent){.x=PARK_X,.z=PARK_Z,.yaw=0};v->door=0;v->synced=true;}
 v->lastSupplied=s->supplied;
 for(int i=0;i<MACHINES;i++){
  if(v->collectedGood[i]>s->machines[i].good)v->collectedGood[i]=s->machines[i].good;
  if(v->collectedBad[i]>s->machines[i].defects)v->collectedBad[i]=s->machines[i].defects;}
 float sdt=s->running?dt*(float)s->speed:0;
 // 창고에 시트가 있고 팔레트가 부족하면 작업자가 채우러 옵니다(새 기계 포함).
 // When the store has sheets and a pallet is running low (including a new machine), the worker comes to fill it.
 if(!v->worker.active&&s->running){int need[MACHINES],n=0;for(int i=0;i<s->owned;i++)if(PanelNeedsDelivery(s,i))need[n++]=i;if(n)startWorker(v,need,n);}
 // 양품 8개 또는 불량 4개가 쌓이면 지게차가 옵니다. / The forklift comes once a station has 8 good or 4 rejected parts waiting.
 if(!v->forklift.active&&s->running){bool go=false;int stop[MACHINES],n=0;
  for(int i=0;i<s->owned;i++){int g=s->machines[i].good-v->collectedGood[i],b=s->machines[i].defects-v->collectedBad[i];go|=g>=8||b>=4;if(g+b>0)stop[n++]=i;}
  if(go)startForklift(v,stop,n);}
 stepAgent(v,s,&v->worker,sdt,2.2f,8,false);
 stepAgent(v,s,&v->forklift,sdt,3.0f,2.2f,true);
 // 지게차 포크 높이: 집을 때 올리고, 짐을 싣고 다닐 때는 낮게. / Fork height: raised while picking, low while carrying.
 FactoryAgent *f=&v->forklift;float want=f->doing>=ACT_PICK?.75f:f->carrying?.3f:.06f;f->lift+=(want-f->lift)*(1-expf(-4*sdt));
 if(!f->active&&f->carrying&&f->k>=f->n){f->carrying=false;f->load=0;} // 마당에서 짐을 내림 / unloaded in the yard
 // 누군가 문 근처에 있으면 문을 엽니다(약 1.5초). / The door opens (about 1.5 s) while anyone is near it.
 bool open=false;FactoryAgent *ag[2]={&v->worker,&v->forklift};
 for(int k=0;k<2;k++)open|=ag[k]->active&&fabsf(ag[k]->z+ROOM_HD)<4.5f&&fabsf(ag[k]->x)<8;  // 문 앞 4.5 m 안 / within 4.5 m of the doorway
 v->door=clampf(v->door+(open?1:-1)*sdt/1.5f,0,1);
}
static void drawForklift(const FactoryAgent *a){
 Vector3 o=v3(a->x,0,a->z);float y=a->yaw,L=a->lift;
 Color body={232,180,40,255},dark={40,42,46,255},steel={130,136,142,255},tyre={28,28,30,255};
 obox(o,y,v3(0,.62f,-.1f),v3(1.2f,.7f,2.0f),body);                       // 차체 / body
 obox(o,y,v3(0,.75f,-1.05f),v3(1.22f,.95f,.5f),shade(body,.8f));          // 균형추 / counterweight
 for(int sx=-1;sx<=1;sx+=2){obox(o,y,v3(sx*.52f,1.68f,-.65f),v3(.08f,1.8f,.08f),dark);obox(o,y,v3(sx*.52f,1.68f,.45f),v3(.08f,1.8f,.08f),dark);}
 obox(o,y,v3(0,2.61f,-.1f),v3(1.15f,.06f,1.3f),dark);                    // 보호 지붕(운전자 안전모 위) / overhead guard, above the driver's hat
 obox(o,y,v3(0,1.1f,-.35f),v3(.5f,.25f,.5f),dark);                       // 좌석 / seat
 for(int sx=-1;sx<=1;sx+=2)for(int sz=-1;sz<=1;sz+=2){Vector3 c=v3(sx*.6f,.3f,sz*.68f);
  DrawCylinderEx(xform(o,y,v3(c.x-.13f,c.y,c.z)),xform(o,y,v3(c.x+.13f,c.y,c.z)),.3f,.3f,12,tyre);}
 obox(o,y,v3(-.4f,1.35f,1.0f),v3(.1f,2.6f,.1f),steel);obox(o,y,v3(.4f,1.35f,1.0f),v3(.1f,2.6f,.1f),steel); // 마스트 / mast
 obox(o,y,v3(0,.4f+L,1.1f),v3(1.0f,.32f,.08f),dark);                      // 캐리지 / carriage
 obox(o,y,v3(-.3f,.12f+L,1.7f),v3(.12f,.05f,1.1f),steel);obox(o,y,v3(.3f,.12f+L,1.7f),v3(.12f,.05f,1.1f),steel); // 포크 / forks
 if(a->carrying){float h=.25f+.04f*fminf(a->load,20);                     // 짐: 부품 상자 / load: a crate of parts
  obox(o,y,v3(0,.2f+L,1.7f),v3(1.0f,.1f,1.0f),(Color){112,82,52,255});
  obox(o,y,v3(0,.25f+L+h/2,1.7f),v3(.9f,h,.9f),(Color){150,160,170,255});}
 drawPerson(xform(o,y,v3(0,0,-.35f)),y,0,false,false,true,(Color){236,238,240,255}); // 운전자(흰 안전모) / driver, white hard hat
}
static void drawAgents(const FactoryView *v){
 drawForklift(&v->forklift);
 const FactoryAgent *w=&v->worker;
 if(w->active)drawPerson(v3(w->x,0,w->z),w->yaw,w->phase,w->wait<=0,w->carrying,false,(Color){244,184,32,255});
}
void FactoryUpdate(FactoryView *v,PanelState *s,float dt){
 dt=(dt>0)?fminf(dt,.1f):0;v->time+=dt;
 // 떠오르는 글자 나이 증가, 오래된 것 제거 / age popups and drop old ones
 int n=0;for(int k=0;k<v->popupCount;k++){v->popups[k].age+=dt;if(v->popups[k].age<1.4f)v->popups[n++]=v->popups[k];}v->popupCount=n;
 for(int i=0;i<MACHINES;i++){const Machine *m=&s->machines[i];
  // 새 게임이면 카운터가 줄어듭니다 / counters drop when a new game starts
  if(m->good<v->lastGood[i]||m->defects<v->lastBad[i]){v->lastGood[i]=m->good;v->lastBad[i]=m->defects;}
  // 빨리 감기에서 한 번에 여러 개가 팔려도 글자는 최대 2개 / at most 2 popups per frame under fast-forward
  int newGood=m->good-v->lastGood[i],newBad=m->defects-v->lastBad[i];
  for(int k=0;k<newGood&&k<2&&v->popupCount<FACTORY_POPUPS;k++)v->popups[v->popupCount++]=(FactoryPopup){i,0,(float)PanelPartValue(s),false};
  for(int k=0;k<newBad&&k<2&&v->popupCount<FACTORY_POPUPS;k++)v->popups[v->popupCount++]=(FactoryPopup){i,0,0,true};
  v->lastGood[i]=m->good;v->lastBad[i]=m->defects;
  // 벨트와 롤러는 기계가 일할 때만, 기계 속도에 맞춰 움직입니다 / belts and rollers move only while working, at the machine's speed
  if(s->running&&i<s->owned&&m->busy){float k=dt*(float)(s->speed*m->speed);v->belt[i]=fmodf(v->belt[i]+k*.7f,3.1f);v->roll[i]=fmodf(v->roll[i]+k*3.0f,2*PI);}
 }
 updateAgents(v,s,dt);
}
void FactoryRender(FactoryView *v,const PanelState *s,int selected){
 BeginTextureMode(v->target);ClearBackground(bgColor);
 BeginMode3D(v->orbit.cam);
 drawRoom(v);
 for(int i=0;i<MACHINES;i++)drawStation(v,s,i,i==selected);
 drawAgents(v);
 EndMode3D();EndTextureMode();
}
// 캔버스에 3D 화면을 붙이고, 각 기계 위에 상태 라벨을 그립니다.
// Paste the 3D view onto the canvas and draw a status label above each machine.
void FactoryDraw(const FactoryView *v,const PanelState *s,int x,int y){
 int w=v->width,h=v->height,W=w*SUPERSAMPLE,H=h*SUPERSAMPLE;
 DrawTexturePro(v->target.texture,(Rectangle){0,0,(float)W,(float)-H},(Rectangle){(float)x,(float)y,(float)w,(float)h},(Vector2){0,0},0,WHITE);
 Vector3 fw=v3(v->orbit.cam.target.x-v->orbit.cam.position.x,v->orbit.cam.target.y-v->orbit.cam.position.y,v->orbit.cam.target.z-v->orbit.cam.position.z);
 for(int i=0;i<MACHINES;i++){
  Vector3 p=v3(STATION_X[i],6.3f,0),r=v3(p.x-v->orbit.cam.position.x,p.y-v->orbit.cam.position.y,p.z-v->orbit.cam.position.z);
  if(r.x*fw.x+r.y*fw.y+r.z*fw.z<=0)continue; // 카메라 뒤 / behind the camera
  Vector2 sp=GetWorldToScreenEx(p,v->orbit.cam,W,H);sp.x/=SUPERSAMPLE;sp.y/=SUPERSAMPLE;
  const Machine *m=&s->machines[i];
  const char *t=i>=s->owned?TextFormat("M%d  LOCKED",i+1):m->wear>=s->cfg->wearLimit?TextFormat("M%d  FAULT",i+1):!m->enabled?TextFormat("M%d  STOPPED",i+1):!s->running?TextFormat("M%d  PAUSED",i+1):m->busy?TextFormat("M%d  WORKING",i+1):m->input>0?TextFormat("M%d  READY",i+1):s->stock>0?TextFormat("M%d  AWAITING SHEETS",i+1):TextFormat("M%d  NO SHEETS",i+1);
  int tw=UiMeasure(t,14),lx=x+(int)sp.x-tw/2-6,ly=y+(int)sp.y-12;
  if(lx<x||ly<y||lx+tw+12>x+w||ly+24>y+h)continue;
  DrawRectangle(lx,ly,tw+12,24,(Color){13,21,30,210});UiText(t,lx+6,ly+5,14,statusColor(s,i));
 }
 // 판매 금액/폐기 글자가 통 위로 떠오르며 사라집니다. / Sale amounts and SCRAP float up from the bins and fade.
 for(int k=0;k<v->popupCount;k++){const FactoryPopup *pp=&v->popups[k];
  Vector3 p=v3(STATION_X[pp->machine]+(pp->bad?.85f:-.85f),1.3f+pp->age*.9f,3.2f),r=v3(p.x-v->orbit.cam.position.x,p.y-v->orbit.cam.position.y,p.z-v->orbit.cam.position.z);
  if(r.x*fw.x+r.y*fw.y+r.z*fw.z<=0)continue;
  Vector2 sp=GetWorldToScreenEx(p,v->orbit.cam,W,H);int px=x+(int)(sp.x/SUPERSAMPLE),py=y+(int)(sp.y/SUPERSAMPLE);
  const char *t=pp->bad?"SCRAP":TextFormat("+EUR %.0f",pp->value);int tw=UiMeasure(t,18);
  if(px-tw/2<x||py<y||px+tw/2>x+w||py+18>y+h)continue;
  unsigned char a=(unsigned char)(255*clampf(1-pp->age/1.4f,0,1));
  Color c=pp->bad?(Color){235,90,70,a}:(Color){65,222,177,a};
  UiText(t,px-tw/2+1,py+1,18,(Color){0,0,0,(unsigned char)(a*.6f)});UiText(t,px-tw/2,py,18,c);
 }
 DrawRectangle(x,y+h-26,268,26,(Color){13,21,30,200});UiText("Drag: orbit   Wheel: zoom",x+8,y+h-20,14,(Color){145,163,181,255});
 DrawRectangleLines(x,y,w,h,(Color){39,54,69,255});
}
