#include "raylib.h"
#include "panel_state.h"
#include "factory_view.h"
#include "sawmill_state.h"
#include "sawmill_panel.h"
#include "sawmill_view.h"
#include "ui.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
// 공장 하나의 시뮬레이션과 화면 상태. 보이지 않는 공장은 멈춰 있습니다(데모용, 방치형 아님).
// One factory's simulation and its UI state. The factory you're not looking at stays paused (demo, not idle play).
typedef struct { int tab, selected; bool endOpen, endDone; float endTime; const char *notice; } Site;
static Site sites[2];
static PanelState metal;   // 금속 공장 / metalworks
static SawState saw;       // 제재소 / sawmill
// 지금 보이는 공장만 새로 시작합니다. / New game restarts only the factory you're looking at.
static void newGame(int site){if(site)SawInit(&saw);else PanelInit(&metal,&METAL_CONFIG);}
int main(int argc, char **argv){
 bool capture=argc>1&&strcmp(argv[1],"--screenshot")==0; int frames=0;
 // 창은 레이아웃과 같은 1440x900(1:1)으로 엽니다. 화면이 작으면 맞게 줄이고, 크기 조절도 가능합니다.
 // Open the window at the layout's native 1440x900 (1:1). Shrink only if the screen is too small; resizable.
 const int W=1440,H=900;
 SetConfigFlags(FLAG_WINDOW_RESIZABLE);
 InitWindow(W,H,"METALWORKS | Operations panel");SetTargetFPS(60);SetWindowMinSize(W/2,H/2);
 int mon=GetCurrentMonitor(),mw=GetMonitorWidth(mon),mh=GetMonitorHeight(mon);
 float fit=fminf(1.0f,fminf((mw-40)/(float)W,(mh-120)/(float)H));
 if(fit<1.0f){int w=(int)(W*fit),h=(int)(H*fit);Vector2 mp=GetMonitorPosition(mon);SetWindowSize(w,h);SetWindowPosition((int)mp.x+(mw-w)/2,(int)mp.y+(mh-h)/2);}
 RenderTexture2D canvas=LoadRenderTexture(W,H);
 // 3D 공장 화면(레이아웃 좌표 28,180 / 808x470). / 3D factory view at layout 28,180, 808x470.
 const Rectangle factoryArea={28,180,808,470};
 FactoryView fv;FactoryInit(&fv,(int)factoryArea.width,(int)factoryArea.height);
 static SawView sv;SawViewInit(&sv,(int)factoryArea.width,(int)factoryArea.height); // 숲 현장 3D / forest site 3D
 int filter=-1;
 // 생산 상태와 현재 페이지를 초기화합니다.
 // Initialize production state and the active page.
 for(int k=0;k<2;k++){newGame(k);sites[k].notice="CSV reports save to the game folder";}
 int cur=0; // 0 = 금속 공장, 1 = 제재소 / 0 = metalworks, 1 = sawmill
 // 지금 보이는 공장의 상태(전환할 때 sites에 저장하고 다른 공장을 불러옵니다).
 // The visible factory's state; switching stores it in sites and loads the other one.
 int tab=0,selected=0;const char *notice=sites[0].notice;
 // 계약 완료 화면 상태 / contract-complete screen state
 bool endOpen=false,endDone=false;float endTime=0;
 while(!WindowShouldClose()){
 const SimConfig *C=metal.cfg; // 금속 공장의 이름과 값 / metalworks names and values
 bool isSaw=cur==1;
 // 지금 공장의 공통 값(머리글, 버튼, 종료 화면) / the current factory's shared values (header, buttons, end screen)
 bool *running=isSaw?&saw.running:&metal.running;double *simSpeed=isSaw?&saw.speed:&metal.speed;
 // 창 크기에 맞게 캔버스 배율과 여백을 계산하고 마우스 좌표를 레이아웃 좌표로 변환합니다.
 // Fit the canvas to the window (letterboxed) and map mouse input back to layout coordinates.
 float scale=fminf(GetScreenWidth()/(float)W,GetScreenHeight()/(float)H);
 float ox=floorf((GetScreenWidth()-W*scale)/2),oy=floorf((GetScreenHeight()-H*scale)/2);
 SetMouseOffset((int)-ox,(int)-oy);SetMouseScale(1.0f/scale,1.0f/scale);
 // 1:1 이상에서는 픽셀 글꼴을 선명하게, 축소할 때는 부드럽게 표시합니다.
 // Keep crisp square pixels at 1:1 or larger; smooth only when shrinking.
 int want=scale<1.0f?TEXTURE_FILTER_BILINEAR:TEXTURE_FILTER_POINT;
 if(want!=filter){SetTextureFilter(canvas.texture,want);filter=want;}
 // F 키: 시연과 테스트용 빨리 감기(x1, x2, x4, x8). / F key: fast-forward for demos and testing.
 if(IsKeyPressed(KEY_F))*simSpeed=*simSpeed>=8?1:*simSpeed*2;
 // 생산·경제를 갱신합니다. 판매와 비용은 즉시 돈에 반영됩니다.
 // Update production and the economy; sales and costs change money immediately.
 if(isSaw)SawUpdate(&saw,GetFrameTime());else PanelUpdate(&metal,GetFrameTime());
 // 3D 화면은 캔버스보다 먼저 그려야 합니다(Raylib은 렌더 텍스처를 중첩할 수 없음).
 // The 3D view must be rendered before the canvas, because Raylib can't nest render textures.
 if(isSaw)SawViewUpdate(&sv,&saw,GetFrameTime());else FactoryUpdate(&fv,&metal,GetFrameTime());
 // 계약을 끝내면 한 번 종료 화면을 열고 생산을 멈춥니다. / When the contract completes, open the end screen once and pause.
 bool done=isSaw?saw.contractDone:metal.contractDone;
 if(done&&!endDone){endDone=true;endOpen=true;endTime=0;*running=false;}
 if(endOpen)endTime+=GetFrameTime();
 Rectangle inputArea=endOpen?(Rectangle){0,0,0,0}:factoryArea;
 if(isSaw){SawViewInput(&sv,inputArea,GetFrameTime());SawViewRender(&sv,&saw);}
 else{FactoryInput(&fv,inputArea,GetFrameTime());
  FactoryRender(&fv,&metal,selected);}
 BeginTextureMode(canvas);ClearBackground(bg);uiBlocked=endOpen;
 UiText(isSaw?"SAWMILL":C->title,28,24,30,RAYWHITE);
 label(29,61,isSaw?"FOREST HARVESTING & TIMBER MILL":C->subtitle);
 // 돈은 모든 화면에서 항상 표시합니다. / Money is shown on every page at all times.
 const char *cash=eur(isSaw?saw.money:metal.money);UiText(cash,836-UiMeasure(cash,30),24,30,accent);
 const char *inc=TextFormat("+%s / min",eur(isSaw?SawIncomePerMinute(&saw):PanelIncomePerMinute(&metal)));label(836-UiMeasure(inc,16),61,inc);
 // 목표와 상태, 그 아래 3D 공장 화면. / Goal and status, then the 3D factory view below.
 label(28,112,TextFormat("Goal: earn %s in sales to complete the contract.",eur(isSaw?SAW_CONTRACT_GOAL:C->contractGoal)));
 if(isSaw)label(28,140,TextFormat("%s  |  TIME %s  |  SELLING %s",saw.running?"RUNNING":"PAUSED",mmss(saw.elapsed),SAW_TIER_PRODUCT[saw.stagesOwned]));
 else label(28,140,TextFormat("%s  |  TIME %s  |  MACHINE %02d SELECTED",metal.running?"RUNNING":"PAUSED",mmss(metal.elapsed),selected+1));
 if(isSaw)SawViewDraw(&sv,&saw,(int)factoryArea.x,(int)factoryArea.y);
 else FactoryDraw(&fv,&metal,(int)factoryArea.x,(int)factoryArea.y);
 if(!isSaw&&metal.contractDone){DrawRectangle(28,180,808,44,(Color){13,21,30,220});UiText(TextFormat("CONTRACT COMPLETE IN %s",mmss(metal.contractTime)),44,190,24,accent);}
 if(isSaw){label(28,668,"Trees regrow by themselves. Products sell automatically.");
  label(28,698,"Buy machines on Machines and land on Forest.");}
 else{label(28,668,"Good parts sell automatically. Defective parts are scrapped.");
  label(28,698,TextFormat("Buy %s on Inventory and upgrades on Controls.",C->materials));}
 if(!*running)label(28,738,"Press Play to start production.");
 if(button(28,817,90,*running?"Pause":"Play",*running))*running=!*running;
 if(button(128,817,110,"New game",false)){newGame(cur);selected=0;if(isSaw)SawViewFocus(&sv,-1);else FactoryFocus(&fv,-1);endOpen=endDone=false;notice="New game started";}
 label(252,827,TextFormat("SIM SPEED x%.0f",*simSpeed));
 if(button(470,817,150,isSaw?"Site view":"Factory view",false)){if(isSaw)SawViewFocus(&sv,-1);else FactoryFocus(&fv,-1);}
 // 시뮬레이션 전환: 지금 공장을 저장하고(멈춤) 다른 공장을 불러옵니다. / Switch: store this factory (paused) and load the other.
 if(button(650,817,186,"Switch Simulation",false)){
  sites[cur]=(Site){tab,selected,endOpen,endDone,endTime,notice};cur^=1;isSaw=cur==1;
  running=isSaw?&saw.running:&metal.running;simSpeed=isSaw?&saw.speed:&metal.speed;
  tab=sites[cur].tab;selected=sites[cur].selected;endOpen=sites[cur].endOpen;endDone=sites[cur].endDone;endTime=sites[cur].endTime;notice=sites[cur].notice;
  SetWindowTitle(TextFormat("%s | Operations panel",isSaw?"SAWMILL":C->title));}
 label(28,872,"F: fast-forward  |  New game restarts only this factory");
 DrawRectangle(864,0,576,900,(Color){18,28,39,255});UiText("OPERATIONS",890,26,28,RAYWHITE);label(890,62,"Production, upgrades & finance");
 // 페이지 버튼은 표시할 패널만 변경합니다. / Page buttons switch the visible panel.
 if(isSaw)SawDrawPanel(&saw,&tab,&notice);
 else{
 const char *tabs[]={"Production","Controls","Inventory","Finance"};for(int i=0;i<4;i++)if(button(890+i*132,104,123,tabs[i],tab==i))tab=i;
 // 생산 요약과 계약 진행. / Production summary and contract progress.
 if(tab==0){
 metric(890,166,"TOTAL PRODUCED",TextFormat("%d",metal.total));metric(1150,166,"SUCCESS RATE",TextFormat("%.1f%%",metal.total?100.0*metal.good/metal.total:0));
 metric(890,262,"GOOD PRODUCTS",TextFormat("%d",metal.good));metric(1150,262,"DEFECTIVE",TextFormat("%d",metal.defects));
 label(890,372,"CONTRACT (lifetime sales)");
 UiText(TextFormat("%s / %s",eur(metal.earned),eur(C->contractGoal)),890,404,24,RAYWHITE);
 bar(890,446,500,metal.earned/C->contractGoal);
 label(890,470,metal.contractDone?TextFormat("COMPLETE in %s",mmss(metal.contractTime)):TextFormat("%.1f%% complete  |  IN PROGRESS",100*metal.earned/C->contractGoal));
 label(890,534,TextFormat("Capacity %.1f parts/min  |  Part %s",PanelRate(&metal),eur2(PanelPartValue(&metal))));
 label(890,569,"RECENT INSPECTION RESULTS");
 for(int j=0;j<4&&j<metal.recordCount;j++){Product p=metal.records[(metal.nextRecord-1-j+RECORDS)%RECORDS];label(890,605+j*27,TextFormat("#%04d   Machine %d    %7.1fs    %s",p.id,p.machine+1,p.time,p.defective?"DEFECT":"GOOD"));}
 }
 // 기계와 공장 업그레이드. View는 기계를 선택합니다(3D 카메라는 다음 단계).
 // Machine and factory upgrades. View moves the 3D camera to that machine.
 if(tab==1){
 for(int i=0;i<MACHINES;i++){int y=166+i*152;Machine *m=&metal.machines[i];DrawRectangleRounded((Rectangle){890,y,505,140},.1,6,panel);
 if(i>=metal.owned){UiText(TextFormat("MACHINE %02d  -  LOCKED",i+1),905,y+14,18,muted);
  if(i==metal.owned){if(shop(905,y+49,240,"Unlock machine",PanelUpgradeCost(&metal,UP_MACHINE),metal.money,NULL))PanelBuyUpgrade(&metal,UP_MACHINE);}
  else label(905,y+59,"Unlock the previous machine first.");
  continue;}
 UiText(TextFormat("%s %02d",C->machineName[metal.process?1:0],i+1),905,y+14,18,RAYWHITE);
 if(button(1210,y+8,72,"View",selected==i)){selected=i;FactoryFocus(&fv,i);}
 if(button(1290,y+8,90,m->enabled?"Stop":"Start",m->enabled))m->enabled=!m->enabled;
 label(905,y+59,TextFormat("x%.2f  %.1f s/part",m->speed,PanelCycleTime(&metal,i)));
 Upgrade su=(Upgrade)(UP_SPEED_1+i);if(shop(1080,y+49,140,"Faster",PanelUpgradeCost(&metal,su),metal.money,"Max speed"))PanelBuyUpgrade(&metal,su);
 if(shop(1230,y+49,150,C->serviceWord,m->wear?C->serviceCost:-1,metal.money,"Serviced"))PanelMaintain(&metal,i);
 label(905,y+99,TextFormat("%d cycles | Wear %d/%d | Risk %.0f%% | %s",m->cycles,m->wear,C->wearLimit,100*PanelDefectRisk(&metal,i),m->wear>=C->wearLimit?"FAULT":!m->enabled?"STOPPED":m->busy?"WORKING":"IDLE"));bar(905,y+126,465,m->progress);
 }
 label(890,641,"FACTORY UPGRADES (prices in EUR)");
 if(shop(890,673,155,TextFormat("Quality %d",metal.level[UP_QUALITY]+1),PanelUpgradeCost(&metal,UP_QUALITY),metal.money,"Quality MAX"))PanelBuyUpgrade(&metal,UP_QUALITY);
 if(shop(1050,673,188,"Auto-service",PanelUpgradeCost(&metal,UP_AUTOSERVICE),metal.money,"Auto-service ON"))PanelBuyUpgrade(&metal,UP_AUTOSERVICE);
 if(shop(1243,673,152,C->processUpgrade,PanelUpgradeCost(&metal,UP_PROCESS),metal.money,TextFormat("%s ON",C->processUpgrade)))PanelBuyUpgrade(&metal,UP_PROCESS);
 if(!metal.process)label(890,714,TextFormat("%s: slower cycles, EUR %.0f per part instead of %.0f",C->processUpgrade,C->value[1],C->value[0]));
 }
 // 재고: 시트 구매와 자동 구매기. / Inventory: buying sheets and the auto-buyer.
 if(tab==2){int onPallets=PanelPalletTotal(&metal);metric(890,166,TextFormat("%s IN STORE",C->materialsUpper),TextFormat("%d",metal.stock));metric(1150,166,"ON MACHINE PALLETS",TextFormat("%d",onPallets));
 label(890,283,TextFormat("Price per %s %s  |  One %s per part",C->material,eur2(C->materialPrice),C->material));
 if(shop(890,317,240,TextFormat("Buy 10 %s",C->materials),10*C->materialPrice,metal.money,NULL))PanelBuyMaterial(&metal,10);
 if(shop(1150,317,240,TextFormat("Buy 50 %s",C->materials),50*C->materialPrice,metal.money,NULL))PanelBuyMaterial(&metal,50);
 label(890,385,TextFormat("AUTO-BUYER (buys 10 %s at a time)",C->materials));
 if(!metal.level[UP_AUTOBUY]){if(shop(890,415,240,"Unlock auto-buyer",PanelUpgradeCost(&metal,UP_AUTOBUY),metal.money,NULL))PanelBuyUpgrade(&metal,UP_AUTOBUY);}
 else{if(button(890,415,240,metal.autoBuy?"Auto-buyer ON":"Auto-buyer OFF",metal.autoBuy))metal.autoBuy=!metal.autoBuy;
  if(button(1150,415,45,"-",false))metal.autoBuyLevel=metal.autoBuyLevel>10?metal.autoBuyLevel-10:10;
  label(1210,425,TextFormat("Keep %d",metal.autoBuyLevel));
  if(button(1345,415,45,"+",false))metal.autoBuyLevel=metal.autoBuyLevel<200?metal.autoBuyLevel+10:200;}
 label(890,480,"ESTIMATED STOCK REMAINING");double rate=PanelRate(&metal);UiText(rate>0?TextFormat("%.1f minutes (estimate)",(metal.stock+onPallets)/rate):"No active machines",890,510,26,RAYWHITE);
 bar(890,556,500,(metal.stock+onPallets)/100.0);label(890,580,TextFormat("Bought %s go to the store. The worker",C->materials));label(890,606,"carries them to each machine's own pallet.");
 label(890,640,TextFormat("Pallets: %s",metal.owned>=3?TextFormat("M1 %d   M2 %d   M3 %d",metal.machines[0].input,metal.machines[1].input,metal.machines[2].input):metal.owned==2?TextFormat("M1 %d   M2 %d",metal.machines[0].input,metal.machines[1].input):TextFormat("M1 %d",metal.machines[0].input)));
 label(890,672,"If you run out of money, the supplier sends 5 free.");}
 // 재무: 수입과 지출 내역, CSV 내보내기. / Finance: income and spending, CSV export.
 if(tab==3){metric(890,166,"MONEY / EUR",TextFormat("%.2f",metal.money));metric(1150,166,"SALES / EUR",TextFormat("%.2f",metal.earned));
 metric(890,262,TextFormat("%s / EUR",C->materialsUpper),TextFormat("%.2f",metal.spentMaterial));metric(1150,262,"ENERGY / EUR",TextFormat("%.2f",metal.spentEnergy));
 metric(890,358,"SERVICE / EUR",TextFormat("%.2f",metal.spentService));metric(1150,358,"UPGRADES / EUR",TextFormat("%.2f",metal.spentUpgrades));
 label(890,456,TextFormat("Operating profit: %s  (before upgrades)",eur2(PanelRevenue(&metal)-PanelCost(&metal))));
 label(890,486,TextFormat("Price per %s %s  |  Electricity %s/kWh",C->material,eur2(C->materialPrice),eur2(C->energyPrice)));
 label(890,516,TextFormat("Service %s  |  Good part %s",eur2(C->serviceCost),eur2(PanelPartValue(&metal))));
 label(890,546,"Defective parts are scrapped and earn nothing.");
 if(button(890,583,250,"Export CSV report",false))notice=PanelExport(&metal,C->reportFile)?"Saved to the game folder":"Export failed: folder is not writable";
 label(890,636,notice);label(890,669,TextFormat("%s: totals + latest 1,000 records",C->reportFile));}
 }
 char alerts[6][ALERT_LEN];int n=isSaw?SawAlerts(&saw,alerts,6):PanelAlerts(&metal,alerts,6);DrawLine(890,738,1395,738,(Color){52,69,84,255});UiText(TextFormat("ALERTS  /  %d",n),890,754,17,n?ORANGE:accent);if(!n)label(890,787,"All systems normal");for(int i=0;i<n&&i<5;i++)UiText(alerts[i],890,784+i*21,14,ORANGE);
 // ---- 계약 완료 화면 / Contract-complete screen ----
 if(endOpen){
  UiRow rows[9];int nr=0;
  #define ROW(nm,...) do{rows[nr].name=(nm);snprintf(rows[nr].value,sizeof rows[nr].value,__VA_ARGS__);nr++;}while(0)
  double ct;
  if(isSaw){ct=saw.contractTime;
   ROW("TIME","%s",mmss(ct));ROW("TREES FELLED","%d",saw.treesFelled);ROW("LOGS CUT","%d",saw.logsCut);
   ROW("TOTAL SALES","%s",eur(saw.earned));ROW("MONEY IN BANK","%s",eur(saw.money));ROW("SPENT ON UPGRADES","%s",eur(saw.spentUpgrades));
   ROW("LAND PLOTS","%d of %d",saw.plotsOwned,PLOTS);ROW("SELLING","%s",SAW_TIER_PRODUCT[saw.stagesOwned]);
   ROW("WAITED FOR TREES","%.0f%% of the time",saw.elapsed>0?100*saw.harvIdle/saw.elapsed:0);}
  else{ct=metal.contractTime;
   ROW("TIME","%s",mmss(ct));ROW("PARTS PRODUCED","%d (%d good)",metal.total,metal.good);ROW("DEFECT RATE","%.1f%%",metal.total?100.0*metal.defects/metal.total:0);
   ROW("TOTAL SALES","%s",eur(metal.earned));ROW("MONEY IN BANK","%s",eur(metal.money));ROW("SPENT ON UPGRADES","%s",eur(metal.spentUpgrades));
   ROW("MACHINES","%d of %d",metal.owned,MACHINES);ROW("PROCESS","%s",C->processName[metal.process?1:0]);ROW("ENERGY USED","%.2f kWh",metal.energy);}
  #undef ROW
  int act=UiEndScreen(W,H,endTime,isSaw?SAW_CONTRACT_GOAL:C->contractGoal,ct,rows,nr,notice&&strncmp(notice,"Saved",5)==0);
  if(act==END_RESUME){endOpen=false;*running=true;}
  if(act==END_NEW_GAME){newGame(cur);selected=0;if(isSaw)SawViewFocus(&sv,-1);else FactoryFocus(&fv,-1);endOpen=endDone=false;notice="New game started";}
  if(act==END_EXPORT)notice=(isSaw?SawExport(&saw,"sawmill-report.csv"):PanelExport(&metal,C->reportFile))?"Saved to the game folder":"Export failed: folder is not writable";
 }
 EndTextureMode();
 BeginDrawing();ClearBackground(bg);
 DrawTexturePro(canvas.texture,(Rectangle){0,0,(float)W,(float)-H},(Rectangle){ox,oy,floorf(W*scale),floorf(H*scale)},(Vector2){0,0},0,WHITE);
 EndDrawing();
 if(capture&&++frames==3){TakeScreenshot("preview.png");break;}
 }
 FactoryUnload(&fv);SawViewUnload(&sv);UnloadRenderTexture(canvas);CloseWindow();return 0;
}
