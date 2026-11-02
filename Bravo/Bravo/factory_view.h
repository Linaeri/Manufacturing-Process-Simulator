#ifndef FACTORY_VIEW_H
#define FACTORY_VIEW_H
#include "raylib.h"
#include "panel_state.h"
#include "view3d.h"

// 3D 공장 화면: 방, 기계, 궤도 카메라. 상태를 읽기만 하고 변경하지 않습니다.
// 3D factory view: room, machines and orbit camera. Reads the state, never changes it.
// 판매/폐기 시 떠오르는 글자. / Floating text when a part is sold or scrapped.
typedef struct { int machine; float age, value; bool bad; } FactoryPopup;
#define FACTORY_POPUPS 24
// 경로를 따라 움직이는 작업자·지게차. pts의 y는 월드 z입니다.
// A worker or forklift following a waypoint route; pts[].y holds world z.
#define AGENT_POINTS 24
typedef struct {
 Vector2 pts[AGENT_POINTS]; int act[AGENT_POINTS]; int n, k;
 float x, z, yaw, wait, phase, lift; int doing, load;
 bool active, carrying;
} FactoryAgent;

typedef struct {
 RenderTexture2D target; int width, height;   // 화면 크기(레이아웃 픽셀) / on-screen size in layout pixels
 OrbitCam orbit;                              // 공유 궤도 카메라 / shared orbit camera
 // 애니메이션 상태(표시 전용). / Animation state, display only.
 float time, belt[MACHINES], roll[MACHINES];
 int lastGood[MACHINES], lastBad[MACHINES];
 FactoryPopup popups[FACTORY_POPUPS]; int popupCount;
 // 작업자는 시트를 채우고, 지게차는 통을 비웁니다. 문은 이들이 지나갈 때 열립니다.
 // The worker refills sheets, the forklift empties bins, and the door opens to let them through.
 FactoryAgent worker, forklift;
 float door;
 bool synced; int lastSupplied;
 int collectedGood[MACHINES], collectedBad[MACHINES];
} FactoryView;

void FactoryInit(FactoryView *v, int width, int height);
void FactoryUnload(FactoryView *v);
// -1 = 전체 보기, 0..MACHINES-1 = 기계에 초점. / -1 = overview, 0..MACHINES-1 = focus a machine.
void FactoryFocus(FactoryView *v, int machine);
// area: 레이아웃 좌표의 공장 영역. 마우스가 그 안에 있을 때만 입력을 받습니다.
// area: the factory rectangle in layout coordinates; input only counts inside it.
void FactoryInput(FactoryView *v, Rectangle area, float dt);
// 컨베이어·롤러 움직임과 판매 글자를 갱신합니다. PanelUpdate 다음에 매 프레임 호출합니다.
// Advances belts, rollers and sale popups. Call every frame after PanelUpdate.
// 작업자가 팔레트에 도착하면 PanelDeliver로 창고의 시트를 옮깁니다(화면이 상태를 바꾸는 유일한 곳).
// When the worker reaches a pallet it moves sheets from the store with PanelDeliver (the only place the view changes the state).
void FactoryUpdate(FactoryView *v, PanelState *s, float dt);
// BeginTextureMode(canvas) 전에 호출해야 합니다. / Must be called before BeginTextureMode(canvas).
void FactoryRender(FactoryView *v, const PanelState *s, int selected);
// 캔버스에 3D 화면과 기계 라벨을 그립니다. / Draws the 3D view and machine labels onto the canvas.
void FactoryDraw(const FactoryView *v, const PanelState *s, int x, int y);
#endif
