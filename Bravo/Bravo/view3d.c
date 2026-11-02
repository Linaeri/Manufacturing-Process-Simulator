#include "view3d.h"
#include <math.h>

// ---- 도형 / Shapes ----
Vector3 v3(float x,float y,float z){return (Vector3){x,y,z};}
float clampf(float v,float lo,float hi){return v<lo?lo:v>hi?hi:v;}
Color shade(Color c,float f){return (Color){(unsigned char)clampf(c.r*f,0,255),(unsigned char)clampf(c.g*f,0,255),(unsigned char)clampf(c.b*f,0,255),c.a};}

// ---- 도형: 면마다 밝기를 달리해 조명 없이도 입체감을 줍니다 ----
// ---- Shapes: each face gets its own brightness, giving depth without a lighting shader ----
void quad(Vector3 a,Vector3 b,Vector3 c,Vector3 d,Color col){DrawTriangle3D(a,b,c,col);DrawTriangle3D(a,c,d,col);}
void box(Vector3 c,Vector3 s,Color col){
 float x0=c.x-s.x/2,x1=c.x+s.x/2,y0=c.y-s.y/2,y1=c.y+s.y/2,z0=c.z-s.z/2,z1=c.z+s.z/2;
 // 반시계 방향 = 앞면(Raylib 기본 컬링). / Counter-clockwise from outside = front face (Raylib's default culling).
 quad(v3(x0,y1,z0),v3(x0,y1,z1),v3(x1,y1,z1),v3(x1,y1,z0),col);              // +y top
 quad(v3(x0,y0,z0),v3(x1,y0,z0),v3(x1,y0,z1),v3(x0,y0,z1),shade(col,.45f));  // -y bottom
 quad(v3(x1,y0,z0),v3(x1,y1,z0),v3(x1,y1,z1),v3(x1,y0,z1),shade(col,.82f));  // +x
 quad(v3(x0,y0,z0),v3(x0,y0,z1),v3(x0,y1,z1),v3(x0,y1,z0),shade(col,.66f));  // -x
 quad(v3(x0,y0,z1),v3(x1,y0,z1),v3(x1,y1,z1),v3(x0,y1,z1),shade(col,.9f));   // +z
 quad(v3(x0,y0,z0),v3(x0,y1,z0),v3(x1,y1,z0),v3(x1,y0,z0),shade(col,.6f));   // -z
}
// 위가 열린 상자(통). / Open-topped bin.
void bin(Vector3 c,Vector3 s,Color col){float t=.06f;
 box(v3(c.x,c.y+t/2,c.z),v3(s.x,t,s.z),shade(col,.6f));
 box(v3(c.x-s.x/2+t/2,c.y+s.y/2,c.z),v3(t,s.y,s.z),col);box(v3(c.x+s.x/2-t/2,c.y+s.y/2,c.z),v3(t,s.y,s.z),col);
 box(v3(c.x,c.y+s.y/2,c.z-s.z/2+t/2),v3(s.x,s.y,t),col);box(v3(c.x,c.y+s.y/2,c.z+s.z/2-t/2),v3(s.x,s.y,t),col);}
// 바닥에 그리는 직사각형 테두리. / Rectangle outline painted on the floor.
void floorFrame(float cx,float cz,float w,float d,float t,Color col){float y=.012f,h=.012f;
 box(v3(cx,y,cz-d/2),v3(w,h,t),col);box(v3(cx,y,cz+d/2),v3(w,h,t),col);
 box(v3(cx-w/2,y,cz),v3(t,h,d),col);box(v3(cx+w/2,y,cz),v3(t,h,d),col);}

// 세로축으로 회전한 상자(작업자·지게차용). origin 기준 local 좌표, +z가 앞쪽입니다.
// Box rotated about the vertical axis (for the worker and forklift); local coordinates around origin, +z is forward.
Vector3 xform(Vector3 o,float yaw,Vector3 l){float c=cosf(yaw),s=sinf(yaw);return v3(o.x+l.x*c+l.z*s,o.y+l.y,o.z-l.x*s+l.z*c);}
void obox(Vector3 o,float yaw,Vector3 lc,Vector3 sz,Color col){
 Vector3 k[8];
 for(int i=0;i<8;i++)k[i]=xform(o,yaw,v3(lc.x+((i&1)?sz.x:-sz.x)/2,lc.y+((i&2)?sz.y:-sz.y)/2,lc.z+((i&4)?sz.z:-sz.z)/2));
 // box()와 같은 면 순서(바깥에서 반시계). 밝기는 회전된 면의 방향으로 정합니다.
 // Same face order as box() (counter-clockwise from outside); brightness follows the rotated face direction.
 static const int F[6][4]={{2,6,7,3},{0,1,5,4},{1,3,7,5},{0,4,6,2},{4,5,7,6},{0,2,3,1}};
 static const float N[6][3]={{0,1,0},{0,-1,0},{1,0,0},{-1,0,0},{0,0,1},{0,0,-1}};
 float c=cosf(yaw),sn=sinf(yaw);
 for(int f=0;f<6;f++){float nx=N[f][0]*c+N[f][2]*sn,nz=-N[f][0]*sn+N[f][2]*c,ny=N[f][1];
  float b=ny>.5f?1:ny<-.5f?.45f:.75f+.15f*nz+.07f*nx;
  quad(k[F[f][0]],k[F[f][1]],k[F[f][2]],k[F[f][3]],shade(col,b));}
}

float seg(float p,float a,float b){float t=clampf((p-a)/(b-a),0,1);return t*t*(3-2*t);}
float lerpf(float a,float b,float t){return a+(b-a)*t;}
Vector3 lerp3(Vector3 a,Vector3 b,float t){return v3(lerpf(a.x,b.x,t),lerpf(a.y,b.y,t),lerpf(a.z,b.z,t));}
float wrapAngle(float a){while(a>PI)a-=2*PI;while(a<-PI)a+=2*PI;return a;}

// 작업자: 형광 조끼(반사띠), 안전모, 걷는 다리. / Worker in a hi-vis vest with reflective bands, a hard hat and walking legs.
void drawPerson(Vector3 o,float yaw,float phase,bool walking,bool carrying,bool seated,Color hat){
 Color vest={206,236,44,255},band={225,228,230,255},trousers={42,52,82,255},skin={224,180,140,255},boots={30,30,32,255};
 float sw=walking?sinf(phase)*.18f:0;
 float ty=seated?1.63f:1.17f; // 몸통 중심 높이(앉으면 좌석 위) / torso centre height (on the seat when seated)
 if(!seated){
  obox(o,yaw,v3(-.12f,.44f,sw),v3(.16f,.82f,.18f),trousers);obox(o,yaw,v3(.12f,.44f,-sw),v3(.16f,.82f,.18f),trousers);
  obox(o,yaw,v3(-.12f,.05f,sw+.04f),v3(.18f,.1f,.28f),boots);obox(o,yaw,v3(.12f,.05f,-sw+.04f),v3(.18f,.1f,.28f),boots);}
 else{obox(o,yaw,v3(-.12f,ty-.33f,.2f),v3(.16f,.16f,.5f),trousers);obox(o,yaw,v3(.12f,ty-.33f,.2f),v3(.16f,.16f,.5f),trousers);}
 obox(o,yaw,v3(0,ty,0),v3(.5f,.62f,.3f),vest);                                             // 형광 조끼 / hi-vis vest
 obox(o,yaw,v3(0,ty-.15f,0),v3(.52f,.06f,.32f),band);obox(o,yaw,v3(0,ty+.1f,0),v3(.52f,.06f,.32f),band); // 반사띠 / reflective bands
 float ay=ty-.04f;
 if(carrying){obox(o,yaw,v3(-.3f,ay+.05f,.2f),v3(.13f,.15f,.45f),vest);obox(o,yaw,v3(.3f,ay+.05f,.2f),v3(.13f,.15f,.45f),vest);
  obox(o,yaw,v3(0,ay,.38f),v3(.75f,.18f,.55f),(Color){176,186,196,255});}                  // 시트 묶음 / bundle of sheets
 else if(seated){obox(o,yaw,v3(-.3f,ay+.05f,.22f),v3(.13f,.15f,.45f),vest);obox(o,yaw,v3(.3f,ay+.05f,.22f),v3(.13f,.15f,.45f),vest);} // 핸들 잡기 / hands on the controls
 else{obox(o,yaw,v3(-.32f,ay,-sw*.8f),v3(.13f,.55f,.15f),vest);obox(o,yaw,v3(.32f,ay,sw*.8f),v3(.13f,.55f,.15f),vest);
  obox(o,yaw,v3(-.32f,ay-.32f,-sw*.8f),v3(.11f,.1f,.11f),skin);obox(o,yaw,v3(.32f,ay-.32f,sw*.8f),v3(.11f,.1f,.11f),skin);}
 float hy=ty+.45f;
 obox(o,yaw,v3(0,hy,0),v3(.24f,.26f,.24f),skin);
 Vector3 h=xform(o,yaw,v3(0,hy+.12f,0));
 DrawCylinderEx(h,v3(h.x,h.y+.02f,h.z),.2f,.2f,12,hat);                 // 챙 / brim
 DrawCylinderEx(v3(h.x,h.y+.02f,h.z),v3(h.x,h.y+.15f,h.z),.16f,.11f,12,hat); // 안전모 / hard hat
}

// ---- 궤도 카메라 / Orbit camera ----
static Vector3 orbitDir(float yaw,float pitch){return v3(cosf(pitch)*sinf(yaw),sinf(pitch),cosf(pitch)*cosf(yaw));}
// 원점 o에서 방향 d로 쏜 광선이 상자에 들어가는 거리, 맞지 않으면 -1.
// Distance along ray (o, d) where it enters the box, or -1 if it misses.
static float rayBox(Vector3 o,Vector3 d,Vector3 mn,Vector3 mx){
 float oo[3]={o.x,o.y,o.z},dd[3]={d.x,d.y,d.z},lo[3]={mn.x,mn.y,mn.z},hi[3]={mx.x,mx.y,mx.z},t0=-1e30f,t1=1e30f;
 for(int a=0;a<3;a++){
  if(fabsf(dd[a])<1e-8f){if(oo[a]<lo[a]||oo[a]>hi[a])return -1;continue;}
  float ta=(lo[a]-oo[a])/dd[a],tb=(hi[a]-oo[a])/dd[a];if(ta>tb){float k=ta;ta=tb;tb=k;}
  t0=fmaxf(t0,ta);t1=fminf(t1,tb);if(t0>t1)return -1;}
 return t1<0?-1:t0;
}
// 원점이 상자 안에 있을 때 광선이 상자를 빠져나가는 거리. / Distance at which a ray starting inside the box leaves it.
static float rayBoxExit(Vector3 o,Vector3 d,Vector3 mn,Vector3 mx){
 float oo[3]={o.x,o.y,o.z},dd[3]={d.x,d.y,d.z},lo[3]={mn.x,mn.y,mn.z},hi[3]={mx.x,mx.y,mx.z},t=1e30f;
 for(int a=0;a<3;a++){if(dd[a]>1e-8f)t=fminf(t,(hi[a]-oo[a])/dd[a]);else if(dd[a]<-1e-8f)t=fminf(t,(lo[a]-oo[a])/dd[a]);}
 return t;
}
// 여백을 더한 장애물 상자. exitTest면 바닥 아래까지 늘립니다. / Obstacle box grown by the margin; reaches below ground for the exit test.
static void grown(const OrbitCam *c,int i,bool exitTest,Vector3 *mn,Vector3 *mx){
 Box3 b=c->obstacles[i];float m=c->margin;
 *mn=v3(b.mn.x-m,exitTest?b.mn.y-1:b.mn.y,b.mn.z-m);*mx=v3(b.mx.x+m,b.mx.y+m,b.mx.z+m);
}
// 초점이 장애물 안에 있으면(그 기계를 보는 중) 카메라는 최소한 그 밖에 있어야 합니다. 그 최소 거리, 없으면 0.
// If the focus is inside an obstacle (looking at that machine), the camera must at least be outside it: that distance, or 0.
static float obstacleExit(const OrbitCam *c,Vector3 f,Vector3 d){float need=0;
 for(int i=0;i<c->obstacleCount;i++){Vector3 mn,mx;grown(c,i,true,&mn,&mx);
  if(f.x>mn.x&&f.x<mx.x&&f.y>mn.y&&f.y<mx.y&&f.z>mn.z&&f.z<mx.z)need=fmaxf(need,rayBoxExit(f,d,mn,mx)+.05f);}
 return need;
}
// 스프링 암: 초점에서 카메라 쪽으로 선을 그어 경계·장애물에 닿기 전에 멈춥니다.
// Spring arm: follow the line from the focus towards the camera and stop before the bounds or any obstacle.
float OrbitSafeDistance(const OrbitCam *c,Vector3 f,float yaw,float pitch,float want){
 Vector3 d=orbitDir(yaw,pitch);float t=want,m=c->margin;
 t=fmaxf(t,obstacleExit(c,f,d));
 float o[3]={f.x,f.y,f.z},dv[3]={d.x,d.y,d.z};
 float lo[3]={c->lo.x+m,c->lo.y+m,c->lo.z+m},hi[3]={c->hi.x-m,c->hi.y-m,c->hi.z-m};
 for(int a=0;a<3;a++){
  if(dv[a]>1e-6f)t=fminf(t,(hi[a]-o[a])/dv[a]);
  else if(dv[a]<-1e-6f)t=fminf(t,(lo[a]-o[a])/dv[a]);}
 for(int i=0;i<c->obstacleCount;i++){Vector3 mn,mx;grown(c,i,false,&mn,&mx);
  bool inside=f.x>mn.x&&f.x<mx.x&&f.y>mn.y&&f.y<mx.y&&f.z>mn.z&&f.z<mx.z; // 위에서 처리함 / handled above
  if(inside)continue;
  float hit=rayBox(f,d,mn,mx);if(hit>=0&&hit<t)t=hit;}
 return t>0?t:0;
}
static void place(OrbitCam *c){
 Vector3 d=orbitDir(c->yaw,c->pitch);
 c->cam.target=c->focus;
 c->cam.position=v3(c->focus.x+d.x*c->shownDist,c->focus.y+d.y*c->shownDist,c->focus.z+d.z*c->shownDist);
}
void OrbitSnap(OrbitCam *c){c->focus=c->focusGoal;c->shownDist=OrbitSafeDistance(c,c->focus,c->yaw,c->pitch,c->dist);place(c);}
void OrbitUpdate(OrbitCam *c,Rectangle area,float dt){
 dt=(dt>0)?fminf(dt,.1f):0;
 bool over=CheckCollisionPointRec(GetMousePosition(),area);
 // 왼쪽 드래그: 궤도 회전. 화면 영역 안에서 시작한 드래그만 사용합니다.
 // Left-drag orbits; only drags that start inside the view count.
 if(over&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))c->dragging=true;
 if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT))c->dragging=false;
 if(c->dragging){Vector2 d=GetMouseDelta();c->yaw-=d.x*.006f;c->pitch+=d.y*.006f;}
 c->pitch=clampf(c->pitch,c->pitchMin,c->pitchMax);
 c->yaw=fmodf(c->yaw,2*PI);
 // 휠: 확대·축소. 이미 경계 때문에 가까워져 있으면 그 거리부터 시작합니다.
 // Wheel zooms; if the bounds already pulled the camera in, zoom from there so it responds immediately.
 float wheel=over?GetMouseWheelMove():0;
 if(wheel!=0){if(c->dist>c->shownDist)c->dist=c->shownDist;c->dist=clampf(c->dist*powf(.85f,wheel),c->minDist,c->maxDist);}
 float k=1-expf(-6*dt);
 c->focus=v3(c->focus.x+(c->focusGoal.x-c->focus.x)*k,c->focus.y+(c->focusGoal.y-c->focus.y)*k,c->focus.z+(c->focusGoal.z-c->focus.z)*k);
 // 가까워질 때는 즉시(통과 방지), 멀어질 때는 부드럽게. / Pull in instantly (no clipping), ease back out smoothly.
 float safe=OrbitSafeDistance(c,c->focus,c->yaw,c->pitch,c->dist);
 if(safe<c->shownDist)c->shownDist=safe;else c->shownDist+=(safe-c->shownDist)*(1-expf(-8*dt));
 // 장애물 밖으로 나가는 것은 즉시. / Getting out of an obstacle happens instantly, never eased.
 c->shownDist=fmaxf(c->shownDist,obstacleExit(c,c->focus,orbitDir(c->yaw,c->pitch)));
 place(c);
}
