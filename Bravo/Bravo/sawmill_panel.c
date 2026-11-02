#include "sawmill_panel.h"
#include "ui.h"
#include <stdio.h>

static const char *stageStatus(const SawState *s,int k){const MillStage *m=&s->stage[k];
 return m->blocked?"BLOCKED - next stage full":m->busy?"WORKING":"WAITING FOR LOGS";}

// ---- 오른쪽 운영 패널 / Right-hand operations panel ----
void SawDrawPanel(SawState *s,int *tab,const char **notice){
 const char *tabs[]={"Production","Machines","Forest","Finance"};
 for(int i=0;i<4;i++)if(button(890+i*132,104,123,tabs[i],*tab==i))*tab=i;
 if(*tab==0){ // 생산 요약과 계약 / production summary and contract
  int soldProducts=s->sold[1]+s->sold[2]+s->sold[3];
  metric(890,166,"TREES FELLED",TextFormat("%d",s->treesFelled));metric(1150,166,"LOGS CUT",TextFormat("%d",s->logsCut));
  metric(890,262,"PROCESSED & SOLD",TextFormat("%d",soldProducts));metric(1150,262,"LOGS SOLD RAW",TextFormat("%d",s->sold[0]));
  label(890,372,"CONTRACT (lifetime sales)");
  UiText(TextFormat("%s / %s",eur(s->earned),eur(SAW_CONTRACT_GOAL)),890,404,24,RAYWHITE);
  bar(890,446,500,s->earned/SAW_CONTRACT_GOAL);
  label(890,470,s->contractDone?TextFormat("COMPLETE in %s",mmss(s->contractTime)):TextFormat("%.1f%% complete  |  IN PROGRESS",100*s->earned/SAW_CONTRACT_GOAL));
  label(890,534,TextFormat("Now selling: %s at %s per log",SAW_TIER_PRODUCT[s->stagesOwned],eur(SawCurrentValue(s))));
  label(890,569,"SOLD BY PRODUCT");
  for(int k=0;k<=MILL_STAGES;k++)label(890,605+k*27,TextFormat("%-20s %5d   %s each",SAW_TIER_PRODUCT[k],s->sold[k],eur(SAW_TIER_VALUE[k])));
 }
 if(*tab==1){ // 하베스터와 제재 기계 / harvester and mill machines
  int y=166;DrawRectangleRounded((Rectangle){890,y,505,104},.1f,6,panel);
  UiText("HARVESTER",905,y+12,18,RAYWHITE);label(1040,y+14,"fell, strip, cut");
  if(shop(1240,y+8,140,"Faster",SawUpgradeCost(s,SU_HARVESTER),s->money,"Max speed"))SawBuyUpgrade(s,SU_HARVESTER);
  label(905,y+46,TextFormat("x%.2f  %.1f s/tree  |  %.1f logs/min",s->harvSpeed,SawHarvestCycle(s),SawHarvestPerMinute(s)*LOGS_PER_TREE));
  label(905,y+72,s->harvBusy?"FELLING":"WAITING FOR A MATURE TREE - see Forest");bar(905,y+94,465,s->harvBusy?s->harvProgress:0);
  for(int k=0;k<MILL_STAGES;k++){y=286+k*120;bool own=k<s->stagesOwned;MillStage *m=&s->stage[k];
   DrawRectangleRounded((Rectangle){890,y,505,104},.1f,6,panel);
   if(!own){UiText(TextFormat("%s  -  LOCKED",SAW_STAGE_NAME[k]),905,y+12,18,muted);
    if(k==s->stagesOwned){label(905,y+42,TextFormat("Unlocks: %s at %s per log",SAW_TIER_PRODUCT[k+1],eur(SAW_TIER_VALUE[k+1])));
     if(shop(905,y+64,240,TextFormat("Buy %s",SAW_STAGE_NAME[k]),SawUpgradeCost(s,SU_MILL),s->money,NULL))SawBuyUpgrade(s,SU_MILL);}
    else label(905,y+46,"Buy the previous machine first.");
    continue;}
   UiText(SAW_STAGE_NAME[k],905,y+12,18,RAYWHITE);
   SawUpgrade su=(SawUpgrade)(SU_STAGE_1+k);if(shop(1240,y+8,140,"Faster",SawUpgradeCost(s,su),s->money,"Max speed"))SawBuyUpgrade(s,su);
   label(905,y+46,TextFormat("x%.2f  %.1f s/log  |  Queue %d  |  Risk %.0f%%",m->speed,SawStageCycle(s,k),k==0?s->yard:m->queue,100*SawStageRisk(s,k)));
   label(905,y+72,TextFormat("%s  ->  %s",stageStatus(s,k),SAW_TIER_PRODUCT[k+1]));bar(905,y+94,465,m->progress);}
  label(890,652,TextFormat("Mill capacity %.1f logs/min  |  Harvest %.1f logs/min",SawMillPerMinute(s),SawHarvestPerMinute(s)*LOGS_PER_TREE));
 }
 if(*tab==2){ // 숲과 땅 / forest and land
  metric(890,166,"LAND PLOTS",TextFormat("%d of %d",s->plotsOwned,PLOTS));metric(1150,166,"MATURE TREES",TextFormat("%d",SawMatureTrees(s)));
  double need=SawHarvestPerMinute(s),grow=SawRegrowPerMinute(s);bool short_=need>grow*1.05;
  label(890,272,"TREE SUPPLY vs DEMAND (trees/min)");
  label(890,302,TextFormat("Regrowth   %4.1f",grow));bar(1040,306,350,grow/24);
  label(890,330,TextFormat("Harvester  %4.1f",need));bar(1040,334,350,need/24);
  UiText(short_?"SHORTAGE - buy more land":"ENOUGH TREES",890,362,20,short_?ORANGE:accent);
  if(shop(890,400,250,"Buy next plot",SawUpgradeCost(s,SU_LAND),s->money,"All land owned"))SawBuyUpgrade(s,SU_LAND);
  label(1160,410,TextFormat("+%.1f trees/min",TREES_PER_PLOT*60/SawRegrowSeconds()));
  label(890,454,TextFormat("Felled trees regrow in %.0f s. A new plot arrives",SawRegrowSeconds()));
  label(890,478,TextFormat("fully grown with %d trees (%d logs).",TREES_PER_PLOT,TREES_PER_PLOT*LOGS_PER_TREE));
  label(890,516,"PLOTS");
  for(int p=0;p<s->plotsOwned;p++){int n=0;for(int t=0;t<TREES_PER_PLOT;t++)n+=s->growth[p][t]>=1;
   label(890+(p%2)*255,546+(p/2)*28,TextFormat("Plot %d: %d/%d mature",p+1,n,TREES_PER_PLOT));}
 }
 if(*tab==3){ // 재무 / finance
  metric(890,166,"MONEY / EUR",TextFormat("%.2f",s->money));metric(1150,166,"SALES / EUR",TextFormat("%.2f",s->earned));
  metric(890,262,"DIESEL / EUR",TextFormat("%.2f",s->spentFuel));metric(1150,262,"POWER / EUR",TextFormat("%.2f",s->spentPower));
  metric(890,358,"UPGRADES / EUR",TextFormat("%.2f",s->spentUpgrades));metric(1150,358,"SCRAPPED",TextFormat("%d",s->scrapTotal));
  label(890,456,TextFormat("Operating profit: %s  (before upgrades)",eur2(s->earned-SawRunningCost(s))));
  label(890,486,"Trees are free: they regrow on your land.");
  label(890,516,"Harvester diesel about EUR 1.50 per tree.");
  label(890,546,"Split or knotty pieces are scrapped to chips.");
  if(button(890,583,250,"Export CSV report",false))*notice=SawExport(s,"sawmill-report.csv")?"Saved to the game folder":"Export failed: folder is not writable";
  label(890,636,*notice);label(890,669,"sawmill-report.csv: totals for this sawmill");
 }
}
