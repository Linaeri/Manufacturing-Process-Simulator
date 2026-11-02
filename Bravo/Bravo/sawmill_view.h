#ifndef SAWMILL_VIEW_H
#define SAWMILL_VIEW_H
#include "raylib.h"
#include "sawmill_state.h"
#include "view3d.h"

// 숲 현장 3D 화면: 구획, 하베스터, 포워더, 제재 라인, 울타리와 표지판.
// 3D forest site: plots, harvester, forwarder, mill line, perimeter fence and signs.
#define SAW_POPUPS 24
typedef struct { Vector3 at; float age, value; bool bad; } SawPopup;
// 포워더: 길가 더미에서 통나무를 싣고 야적장에 내립니다. pts의 y는 월드 z입니다.
// Forwarder: loads logs from a roadside pile and unloads them in the log yard; pts[].y holds world z.
typedef struct { Vector2 pts[24]; int act[24]; int n, k; float x, z, yaw, wait, crane; int load; bool active; } SawAgent;

typedef struct {
 RenderTexture2D target; int width, height;
 OrbitCam orbit;
 float time, wheel[MILL_STAGES];
 // 하베스터: 시뮬레이션의 현재 나무를 향해 도로를 따라 움직입니다.
 // Harvester: drives along the roads to the tree the simulation is working on.
 float hx, hz, hyaw; int hRow, jobPlot, jobTree; bool jobActive;
 Vector2 hpath[8]; int hn; float hlen;
 SawAgent forwarder;
 int pile[PLOTS], lastRoadside, workPlot;   // 구획별 길가 더미(합 = 시뮬레이션 roadside) / roadside pile per plot (sum = simulation roadside)
 int lastSold[MILL_STAGES+1], lastScrap; bool synced; float lastElapsed;
 SawPopup popups[SAW_POPUPS]; int popupCount;
 float patrol;                                // 순찰 작업자 위치 / patrolling worker's position on its loop
 Texture2D signTex[2]; Model sign[2]; bool signsReady;
} SawView;

void SawViewInit(SawView *v, int width, int height);
void SawViewUnload(SawView *v);
void SawViewFocus(SawView *v, int target);           // -1 전체 / overview
// 포워더가 길가 통나무를 싣고(SawTakeLogs) 야적장에 내립니다(SawUnloadLogs). 화면이 상태를 바꾸는 유일한 곳.
// The forwarder takes roadside logs (SawTakeLogs) and unloads them in the yard (SawUnloadLogs): the only place the view changes the state.
void SawViewUpdate(SawView *v, SawState *s, float dt);
void SawViewInput(SawView *v, Rectangle area, float dt);
void SawViewRender(SawView *v, const SawState *s);   // BeginTextureMode(canvas) 전에 / before BeginTextureMode(canvas)
void SawViewDraw(const SawView *v, const SawState *s, int x, int y);
#endif
