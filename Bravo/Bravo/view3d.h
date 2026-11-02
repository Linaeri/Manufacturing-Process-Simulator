#ifndef VIEW3D_H
#define VIEW3D_H
#include "raylib.h"
#include <stdbool.h>
// 두 공장 3D 화면이 함께 쓰는 도형, 사람, 궤도 카메라. / Shapes, people and the orbit camera shared by both 3D views.

// ---- 도형(조명 셰이더 없이 면마다 밝기를 달리함) / Shapes (per-face brightness instead of a lighting shader) ----
Vector3 v3(float x,float y,float z);
float clampf(float v,float lo,float hi);
Color shade(Color c,float f);
void quad(Vector3 a,Vector3 b,Vector3 c,Vector3 d,Color col);
void box(Vector3 c,Vector3 s,Color col);                       // 축 정렬 상자 / axis-aligned box
void bin(Vector3 c,Vector3 s,Color col);                       // 위가 열린 통 / open-topped bin
void floorFrame(float cx,float cz,float w,float d,float t,Color col);
Vector3 xform(Vector3 o,float yaw,Vector3 l);                  // 세로축 회전 좌표 / local -> world, rotated about the vertical
void obox(Vector3 o,float yaw,Vector3 lc,Vector3 sz,Color col);// 회전한 상자 / rotated box
float seg(float p,float a,float b);                            // 구간 [a,b]의 부드러운 0..1 / smooth 0..1 across [a,b]
float lerpf(float a,float b,float t);
Vector3 lerp3(Vector3 a,Vector3 b,float t);
float wrapAngle(float a);
// 형광 조끼와 안전모를 쓴 작업자. / A worker in a hi-vis vest and hard hat.
void drawPerson(Vector3 o,float yaw,float phase,bool walking,bool carrying,bool seated,Color hat);

// ---- 궤도 카메라: 드래그로 회전, 휠로 확대, 스프링 암으로 경계와 장애물을 피함 ----
// ---- Orbit camera: drag to orbit, wheel to zoom, a spring arm keeps it inside the bounds and out of obstacles ----
typedef struct { Vector3 mn, mx; } Box3;
#define ORBIT_MAX_OBSTACLES 32
typedef struct {
 float yaw, pitch, dist, shownDist; Vector3 focus, focusGoal; bool dragging; Camera3D cam;
 Vector3 lo, hi;                       // 카메라가 머무는 공간(여백은 안쪽으로 적용) / space the camera stays in (margin applied inwards)
 float margin, minDist, maxDist, pitchMin, pitchMax;
 Box3 obstacles[ORBIT_MAX_OBSTACLES]; int obstacleCount;  // 기계·건물(바닥에서 시작) / machines and buildings, standing on the ground
} OrbitCam;
float OrbitSafeDistance(const OrbitCam *c,Vector3 focus,float yaw,float pitch,float want);
void OrbitUpdate(OrbitCam *c,Rectangle area,float dt);   // 입력 + 이동 / input and movement
void OrbitSnap(OrbitCam *c);                             // 목표 위치로 즉시 / jump straight to the goal
#endif
