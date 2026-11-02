#include "sawmill_state.h"
#include <math.h>
#include <stdio.h>

// ---- 밸런스 값: 진행 속도는 여기서만 조정합니다. ----
// ---- Balance values: tune progression pacing here only. ----
const char *SAW_TIER_PRODUCT[MILL_STAGES+1]={"Stripped logs","Rough planks","Planed boards","Finished mouldings"};
const char *SAW_STAGE_NAME[MILL_STAGES]={"BAND SAW","PLANER","MOULDER"};
const double SAW_TIER_VALUE[MILL_STAGES+1]={10,28,55,95};        // 통나무 1개분 판매가 / sale value per log's worth
const double SAW_CONTRACT_GOAL=6500;
static const double START_MONEY=20;
static const double HARVEST_CYCLE=10;                           // 나무 1그루(속도 x1): 이동, 벌채, 가지치기, 절단 / one tree at x1: move, fell, strip, cross-cut
static const double REGROW=120;                                  // 벤 나무가 다시 자라는 시간 / time for a felled tree to regrow
static const double STAGE_CYCLE[MILL_STAGES]={5,4,5};           // 통나무당 초(x1) / seconds per log at x1
static const double SPEED_STEP=.25;
static const double HARV_BASE_COST=25,SPEED_GROWTH=1.5;
static const double STAGE_BASE_COST[MILL_STAGES]={30,60,120};
static const double MILL_COST[MILL_STAGES]={120,450,1000};      // 다음 가공 단계 / next processing tier
static const double LAND_COST[PLOTS-1]={80,200,400,700,1100};  // 다음 구획 / next plot of land
static const double STAGE_RISK[MILL_STAGES]={.06,.03,.03},RISK_PER_LEVEL=.008;
static const double FUEL_PER_TREE=1.5,STAGE_KW=10,POWER_PRICE=.22,INCOME_TAU=20;

void SawInit(SawState *s){
 *s=(SawState){.speed=1,.rng=7,.money=START_MONEY,.plotsOwned=1,.harvSpeed=1};
 for(int p=0;p<PLOTS;p++)for(int t=0;t<TREES_PER_PLOT;t++)s->growth[p][t]=1; // 모든 숲은 다 자란 상태로 시작 / all forest starts mature
 for(int k=0;k<MILL_STAGES;k++)s->stage[k].speed=1;
}
double SawHarvestCycle(const SawState *s){return HARVEST_CYCLE/s->harvSpeed;}
double SawStageCycle(const SawState *s,int k){return STAGE_CYCLE[k]/s->stage[k].speed;}
double SawStageRisk(const SawState *s,int k){return STAGE_RISK[k]+RISK_PER_LEVEL*s->level[SU_STAGE_1+k];}
double SawCurrentValue(const SawState *s){return SAW_TIER_VALUE[s->stagesOwned];}
int SawMatureTrees(const SawState *s){int n=0;for(int p=0;p<s->plotsOwned;p++)for(int t=0;t<TREES_PER_PLOT;t++)n+=s->growth[p][t]>=1;return n;}
double SawRegrowPerMinute(const SawState *s){return s->plotsOwned*TREES_PER_PLOT*60/REGROW;}
double SawHarvestPerMinute(const SawState *s){return 60/SawHarvestCycle(s);}
double SawIncomePerMinute(const SawState *s){return s->incomeEma*60;}
double SawRegrowSeconds(void){return REGROW;}
double SawMillPerMinute(const SawState *s){double r=0;for(int k=0;k<s->stagesOwned;k++){double v=60/SawStageCycle(s,k);if(k==0||v<r)r=v;}return r;}
double SawRunningCost(const SawState *s){return s->spentFuel+s->spentPower;}

double SawUpgradeCost(const SawState *s,SawUpgrade u){
 int l=(u>=0&&u<SU_COUNT)?s->level[u]:0;
 switch(u){
 case SU_HARVESTER: return l<SAW_SPEED_LEVELS?round(HARV_BASE_COST*pow(SPEED_GROWTH,l)):-1;
 case SU_STAGE_1:case SU_STAGE_2:case SU_STAGE_3:{int k=u-SU_STAGE_1;return (k<s->stagesOwned&&l<SAW_SPEED_LEVELS)?round(STAGE_BASE_COST[k]*pow(SPEED_GROWTH,l)):-1;}
 case SU_MILL: return s->stagesOwned<MILL_STAGES?MILL_COST[s->stagesOwned]:-1;
 case SU_LAND: return s->plotsOwned<PLOTS?LAND_COST[s->plotsOwned-1]:-1;
 default: return -1;
 }
}
bool SawBuyUpgrade(SawState *s,SawUpgrade u){
 double c=SawUpgradeCost(s,u);if(c<0||s->money+1e-9<c)return false;
 s->money-=c;s->spentUpgrades+=c;s->level[u]++;
 if(u==SU_HARVESTER)s->harvSpeed=1+SPEED_STEP*s->level[u];
 if(u>=SU_STAGE_1&&u<=SU_STAGE_3)s->stage[u-SU_STAGE_1].speed=1+SPEED_STEP*s->level[u];
 if(u==SU_MILL)s->stagesOwned++;
 if(u==SU_LAND)s->plotsOwned++;   // 새 구획은 다 자란 나무로 가득 / a new plot arrives full of mature trees
 return true;
}
int SawTakeLogs(SawState *s,int most){int n=s->roadside<most?s->roadside:most;if(n<0)n=0;s->roadside-=n;return n;}
int SawUnloadLogs(SawState *s,int n){int room=YARD_CAP-s->yard;if(n>room)n=room;if(n<0)n=0;s->yard+=n;return n;}
// 3D 화면이 없을 때 포워더 운행을 흉내 냅니다(왕복 약 16초, 한 번에 12개).
// Without the 3D view, imitate the forwarder: about 16 s per round trip, 12 logs per load.
static void courier(SawState *s,double dt){
 if(s->courierLoad==0){if(s->roadside>0){s->courierLoad=SawTakeLogs(s,FORWARDER_LOAD);s->courierTime=0;}return;}
 s->courierTime+=dt;if(s->courierTime>=16)s->courierLoad-=SawUnloadLogs(s,s->courierLoad);
}
static void sell(SawState *s,double v){
 s->money+=v;s->earned+=v;s->incomeEma+=v/INCOME_TAU;
 if(!s->contractDone&&s->earned>=SAW_CONTRACT_GOAL){s->contractDone=true;s->contractTime=s->elapsed;}
}
static bool roll(SawState *s,double risk){s->rng=s->rng*1664525u+1013904223u;return (s->rng>>8)%10000<risk*10000;}

void SawUpdate(SawState *s,double dt){
 if(!s->running||!isfinite(dt)||dt<=0)return;
 if(dt>.25)dt=.25;
 while(dt>1e-9){double step=fmin(dt,.02);dt-=step;double sim=step*s->speed;s->elapsed+=sim;
  s->incomeEma*=exp(-sim/INCOME_TAU);
  // 나무가 다시 자랍니다 / trees regrow
  for(int p=0;p<s->plotsOwned;p++)for(int t=0;t<TREES_PER_PLOT;t++)if(s->growth[p][t]<1)s->growth[p][t]=(float)fmin(1,s->growth[p][t]+sim/REGROW);
  // 하베스터: 다 자란 나무를 골라 베고, 가지를 치고, 통나무 3개로 자릅니다.
  // Harvester: picks a mature tree, fells it, strips it and cuts it into 3 logs.
  if(!s->harvBusy){
   for(int p=0;p<s->plotsOwned&&!s->harvBusy;p++)for(int t=0;t<TREES_PER_PLOT;t++)if(s->growth[p][t]>=1){s->harvBusy=true;s->harvPlot=p;s->harvTree=t;s->harvProgress=0;break;}
   if(!s->harvBusy)s->harvIdle+=sim;}
  s->starveEma+=((s->harvBusy?0:1)-s->starveEma)*(1-exp(-sim/20));
  if(s->harvBusy){
   s->harvWork+=sim;double fuel=FUEL_PER_TREE*sim/SawHarvestCycle(s);s->spentFuel+=fuel;s->money-=fuel;
   s->harvProgress+=sim/SawHarvestCycle(s);
   if(s->harvProgress>=1){
    s->growth[s->harvPlot][s->harvTree]=0;s->harvBusy=false;s->harvProgress=0;s->treesFelled++;
    for(int k=0;k<LOGS_PER_TREE;k++){s->logsCut++;
     // 제재기가 없으면 바로 판매, 길가가 가득 차면 남는 통나무만 판매
     // No mill yet: sell right away. Roadside full: sell only the surplus.
     if(s->stagesOwned==0){sell(s,SAW_TIER_VALUE[0]);s->soldRaw++;s->sold[0]++;}
     else if(s->roadside<ROADSIDE_CAP)s->roadside++;   // 길가에서 포워더를 기다림 / waits by the road for the forwarder
     else{sell(s,SAW_TIER_VALUE[0]);s->surplusRaw++;s->sold[0]++;}}}
  }
  if(s->courier)courier(s,sim);
  // 제재 라인: 뒤 단계부터 처리해 자리가 먼저 비도록 합니다.
  // Mill line: handle later stages first so space frees up before earlier stages finish.
  for(int k=s->stagesOwned-1;k>=0;k--){
   MillStage *m=&s->stage[k];int *in=k==0?&s->yard:&m->queue;
   if(m->blocked){if(s->stage[k+1].queue<STAGE_QUEUE_CAP){s->stage[k+1].queue++;m->blocked=false;m->busy=false;m->done++;m->progress=0;}else continue;}
   // 막혔던 단계가 이제 마지막이 아니면(새 기계 구매) 다음 대기열로 보냅니다. / handled above
   if(!m->busy){if(*in>0){(*in)--;m->busy=true;m->progress=0;m->pendingDefect=roll(s,SawStageRisk(s,k));}else continue;}
   double kwh=sim*STAGE_KW/3600;s->energy+=kwh;s->spentPower+=kwh*POWER_PRICE;s->money-=kwh*POWER_PRICE;
   m->progress+=sim/SawStageCycle(s,k);
   if(m->progress>=1){
    if(m->pendingDefect){m->scrap++;s->scrapTotal++;m->busy=false;m->progress=0;}       // 갈라짐·옹이: 칩으로 폐기 / split or knotty: scrapped to chips
    else if(k==s->stagesOwned-1){sell(s,SAW_TIER_VALUE[k+1]);s->sold[k+1]++;m->done++;m->busy=false;m->progress=0;}
    else if(s->stage[k+1].queue<STAGE_QUEUE_CAP){s->stage[k+1].queue++;m->done++;m->busy=false;m->progress=0;}
    else{m->blocked=true;m->progress=1;}
   }
  }
 }
}
int SawAlerts(const SawState *s,char out[][SAW_ALERT_LEN],int cap){int n=0;
#define ALERT(...) do{if(n<cap)snprintf(out[n],SAW_ALERT_LEN,__VA_ARGS__);n++;}while(0)
 if(s->running&&!s->harvBusy&&s->starveEma>.3)ALERT("NOT ENOUGH MATURE TREES - Buy more land");
 if(s->stagesOwned>0&&s->roadside>=ROADSIDE_CAP)ALERT("ROADSIDE FULL - Surplus logs sold unprocessed");
 for(int k=0;k<s->stagesOwned;k++)if(s->stage[k].blocked){ALERT("MILL BOTTLENECK - Speed up the %s",SAW_STAGE_NAME[k+1]);break;}
 if(s->plotsOwned<PLOTS&&SawHarvestPerMinute(s)>SawRegrowPerMinute(s)*1.05)ALERT("HARVESTER OUTPACES REGROWTH - More land needed");
 return n;
#undef ALERT
}
bool SawExport(const SawState *s,const char *path){FILE *f=fopen(path,"w");if(!f)return false;
 fprintf(f,"metric,value\nfactory,SAWMILL\nproduct,%s\nplots_owned,%d\nmill_stages,%d\ntrees_felled,%d\nlogs_cut,%d\nlogs_sold_raw,%d\nsurplus_logs_sold,%d\n"
  "rough_planks_sold,%d\nplaned_boards_sold,%d\nmouldings_sold,%d\nscrapped,%d\nsimulated_seconds,%.1f\nharvester_waiting_seconds,%.1f\n"
  "money_EUR,%.2f\nsales_EUR,%.2f\nfuel_EUR,%.2f\npower_EUR,%.2f\nupgrades_EUR,%.2f\ncontract_complete,%s\ncontract_seconds,%.1f\n",
  SAW_TIER_PRODUCT[s->stagesOwned],s->plotsOwned,s->stagesOwned,s->treesFelled,s->logsCut,s->soldRaw,s->surplusRaw,s->sold[1],s->sold[2],s->sold[3],s->scrapTotal,
  s->elapsed,s->harvIdle,s->money,s->earned,s->spentFuel,s->spentPower,s->spentUpgrades,s->contractDone?"yes":"no",s->contractTime);
 return fclose(f)==0;
}
