#include "raylib.h"
#include "panel_state.h"
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <string.h>
// 공통 색상과 UI 도우미: 모든 페이지에서 같은 스타일을 사용합니다.
// Shared colors and UI helpers keep all pages consistent.
static Color bg={13,21,30,255},panel={23,34,46,255},muted={145,163,181,255},accent={65,222,177,255};
static bool button(int x,int y,int w,const char *t,bool active){Rectangle r={(float)x,(float)y,(float)w,34};bool hover=CheckCollisionPointRec(GetMousePosition(),r);DrawRectangleRounded(r,.18,6,active?accent:hover?(Color){62,81,98,255}:(Color){39,54,69,255});DrawText(t,x+10,y+10,14,active?bg:RAYWHITE);return hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT);}
static void label(int x,int y,const char *t){DrawText(t,x,y,16,muted);}
static void metric(int x,int y,const char *name,const char *value){DrawRectangleRounded((Rectangle){x,y,245,80},.1,6,panel);label(x+16,y+12,name);DrawText(value,x+16,y+37,28,RAYWHITE);}
static void bar(int x,int y,int w,double value){DrawRectangle(x,y,w,8,panel);DrawRectangle(x,y,(int)(w*fmax(0,fmin(1,value))),8,accent);}
int main(int argc, char **argv){
 bool capture=argc>1&&strcmp(argv[1],"--screenshot")==0; int frames=0;
 // 창을 원래 크기의 85%로 줄이고 입력 좌표도 같은 비율로 맞춥니다.
 // Reduce the window to 85% and map mouse input to the original layout.
 const float scale=.85f;
 InitWindow(1224,765,"Metalworks | Operations panel");SetTargetFPS(60);
 RenderTexture2D canvas=LoadRenderTexture(1440,900);
 SetMouseScale(1.0f/scale,1.0f/scale);
 // 생산 상태와 현재 페이지를 초기화합니다.
 // Initialize production state and the active page.
 PanelState s;PanelInit(&s);int tab=0,selected=0;const char *notice="CSV reports save to the working folder";
 while(!WindowShouldClose()){
 // Play 상태에서 생산·재고·전력 데이터를 갱신합니다. 3D 화면과는 독립적입니다.
 // Update production, inventory and energy while playing, independently of 3D rendering.
 PanelUpdate(&s,GetFrameTime());
 BeginTextureMode(canvas);ClearBackground(bg);
 // 공장 영역은 안내 화면으로 유지하며 3D 렌더링과 카메라 입력을 제거했습니다.
 // Keep a placeholder in the factory area; 3D rendering and camera input are removed.
 DrawText("METALWORKS",28,24,30,RAYWHITE);
 label(29,61,"OPERATIONS PANEL / STANDALONE");
 DrawRectangleRounded((Rectangle){28,180,808,470},.03f,6,panel);
 DrawText("OPERATIONS PANEL",65,235,30,RAYWHITE);
 label(65,300,"Use the four page buttons on the right.");
 label(65,335,"3D factory animation and camera controls are removed.");
 label(65,370,"Play starts production. Pause holds it. Reset clears the session.");
 label(65,405,TextFormat("Selected machine: %02d",selected+1));
 label(65,440,TextFormat("%s  |  TIME %.1f s",s.running?"RUNNING":"PAUSED",s.elapsed));
 if(button(28,817,112,s.running?"Pause":"Play",s.running))s.running=!s.running;
 if(button(150,817,98,"Reset",false)){PanelInit(&s);selected=0;notice="Panel reset";}
 if(button(260,817,45,"-",false))s.speed=fmax(.25,s.speed-.25);
 label(320,827,TextFormat("RATE x%.2f",s.speed));
 if(button(435,817,45,"+",false))s.speed=fmin(4,s.speed+.25);
 if(button(510,817,155,"Factory view",false))selected=0;
 label(28,872,"Play / Pause: production  |  Reset: restore initial values");
 DrawRectangle(864,0,576,900,(Color){18,28,39,255});DrawText("OPERATIONS",890,26,28,RAYWHITE);label(890,62,"Production, resources & machine control");
 // 페이지 버튼은 표시할 패널만 변경하며 현재 설정은 유지합니다.
 // Page buttons switch the visible panel while preserving local settings.
 const char *tabs[]={"Production","Controls","Inventory","Costs"};for(int i=0;i<4;i++)if(button(890+i*132,104,123,tabs[i],tab==i))tab=i;
 // 생산 요약과 목표: Play 중 갱신되는 생산 결과를 표시합니다.
 // Production summary and target: display production results updated during Play.
 if(tab==0){
 metric(890,166,"TOTAL PRODUCED",TextFormat("%d",s.total));metric(1150,166,"SUCCESS RATE",TextFormat("%.1f%%",s.total?100.0*s.good/s.total:0));
 metric(890,262,"GOOD PRODUCTS",TextFormat("%d",s.good));metric(1150,262,"DEFECTIVE",TextFormat("%d",s.defects));
 label(890,372,"PRODUCTION TARGET (good products)");
 if(button(890,404,45,"-",false))s.target=s.target>100?s.target-100:100;
 DrawText(TextFormat("%d / %d",s.good,s.target),955,410,24,RAYWHITE);
 if(button(1345,404,45,"+",false))s.target=s.target<=INT_MAX-100?s.target+100:s.target;
 bar(890,462,500,(double)s.good/s.target);label(890,487,TextFormat("%.1f%% complete  |  %s",100.0*s.good/s.target,s.good>=s.target?"TARGET ACHIEVED":"IN PROGRESS"));
 label(890,534,TextFormat("Configured capacity: %.1f units / minute",PanelRate(&s)));
 label(890,569,"RECENT INSPECTION RESULTS");
 for(int j=0;j<4&&j<s.recordCount;j++){Product p=s.records[(s.nextRecord-1-j+RECORDS)%RECORDS];label(890,605+j*27,TextFormat("#%04d   Machine %d    %7.1fs    %s",p.id,p.machine+1,p.time,p.defective?"DEFECT":"GOOD"));}
 }
 // 기계 설정: View는 선택만 하며 카메라를 움직이지 않습니다.
 // Machine settings: View selects a machine without moving a camera.
 if(tab==1){
 for(int i=0;i<MACHINES;i++){int y=166+i*152;Machine *m=&s.machines[i];DrawRectangleRounded((Rectangle){890,y,505,140},.1,6,panel);DrawText(TextFormat("%s %02d",s.process?"FORMING CELL":"STAMPING PRESS",i+1),905,y+14,18,RAYWHITE);
 if(button(1210,y+8,72,"View",selected==i)){selected=i;}
 if(button(1290,y+8,90,m->enabled?"Stop":"Start",m->enabled))m->enabled=!m->enabled;
 if(button(905,y+49,36,"-",false))m->speed=fmax(.5,m->speed-.25);
 label(956,y+59,TextFormat("SPEED x%.2f",m->speed));if(button(1085,y+49,36,"+",false))m->speed=fmin(2,m->speed+.25);
 if(button(1230,y+49,150,"Service",false))PanelMaintain(&s,i);
 label(905,y+99,TextFormat("%d cycles | Wear %d/200 | %s",m->cycles,m->wear,m->wear>=200?"FAULT / SERVICE":m->busy?"PROCESSING":"IDLE"));bar(905,y+126,465,m->progress);
 }
 label(890,641,"PROCESS VARIATION (available after Reset)");
 if(button(890,673,240,"Metal stamping",!s.process)&&s.total==0&&s.stock==s.supplied&&!s.running)s.process=0;
 if(button(1150,673,240,"Sheet forming",s.process)&&s.total==0&&s.stock==s.supplied&&!s.running)s.process=1;
 }
 // 재고 패널: 재입고 버튼은 로컬 재고를 100장 추가합니다.
 // Inventory panel: restocking adds 100 sheets to local stock.
 if(tab==2){metric(890,166,"RAW MATERIAL / SHEETS",TextFormat("%d",s.stock));metric(1150,166,"CONSUMED / SHEETS",TextFormat("%d",s.supplied-s.stock));
 label(890,283,TextFormat("Total supplied: %d sheets",s.supplied));if(button(890,327,240,"Restock +100 sheets",false))PanelRestock(&s);
 label(890,398,"ESTIMATED STOCK REMAINING");double rate=PanelRate(&s);DrawText(rate>0?TextFormat("%.1f minutes (estimate)",s.stock/rate):"No active machines",890,435,26,RAYWHITE);
 bar(890,491,500,(double)s.stock/s.supplied);label(890,527,"One sheet is consumed when a machine starts a job.");label(890,557,"In-process jobs finish after stock reaches zero.");label(890,587,"Restocking resumes enabled machines automatically.");}
 // 비용 패널: 현재 데이터로 비용을 계산하고 CSV로 내보냅니다.
 // Costs panel: calculate costs from current data and export CSV.
 if(tab==3){metric(890,166,"ENERGY / kWh",TextFormat("%.4f",s.energy));metric(1150,166,"MATERIAL COST / EUR",TextFormat("%.2f",(s.supplied-s.stock)*2.5));metric(890,262,"TOTAL COST / EUR",TextFormat("%.2f",PanelCost(&s)));metric(1150,262,"REVENUE / EUR",TextFormat("%.2f",PanelRevenue(&s)));metric(890,358,"ESTIMATED PROFIT / EUR",TextFormat("%.2f",PanelRevenue(&s)-PanelCost(&s)));
 label(890,476,"Sheet EUR 2.50  |  Electricity EUR 0.22/kWh");label(890,507,s.process?"Good formed part: EUR 9.00":"Good stamped part: EUR 7.50");label(890,538,"Cost includes consumed sheets and active energy.");
 if(button(890,583,250,"Export CSV report",false))notice=PanelExport(&s,"production-report.csv")?"Saved: production-report.csv":"Export failed: folder is not writable";label(890,636,notice);label(890,669,"Report contains totals and latest 1,000 product records.");}
 const char *alerts[6];int n=PanelAlerts(&s,alerts,6);DrawLine(890,738,1395,738,(Color){52,69,84,255});DrawText(TextFormat("ALERTS  /  %d",n),890,754,17,n?ORANGE:accent);if(!n)label(890,787,"All systems normal");for(int i=0;i<n&&i<5;i++)DrawText(alerts[i],890,784+i*21,14,ORANGE);
 EndTextureMode();
 BeginDrawing();ClearBackground(bg);
 DrawTexturePro(canvas.texture,(Rectangle){0,0,1440,-900},(Rectangle){0,0,1224,765},(Vector2){0,0},0,WHITE);
 EndDrawing();
 if(capture&&++frames==3){TakeScreenshot("preview.png");break;}
 }
 UnloadRenderTexture(canvas);CloseWindow();return 0;
}
