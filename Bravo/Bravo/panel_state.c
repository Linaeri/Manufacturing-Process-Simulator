#include "panel_state.h"
#include <math.h>
#include <stdio.h>
#include <limits.h>

// ---- 공장별 밸런스 값: 진행 속도는 여기서만 조정합니다. ----
// ---- Per-factory balance values: tune progression pacing here only. ----
const SimConfig METAL_CONFIG={
 .title="METALWORKS",.subtitle="METAL STAMPING PLANT",
 .material="sheet",.materials="sheets",.materialsUpper="SHEETS",
 .machineName={"STAMPING PRESS","FORMING CELL"},.processName={"Metal stamping","Sheet forming"},
 .processUpgrade="Forming",.processUpgradeLong="Sheet forming",.serviceWord="Service",.reportFile="metalworks-report.csv",
 .startMoney=20,.materialPrice=2.5,.energyPrice=.22,.serviceCost=30,.contractGoal=7500,
 .cycle={6.0,9.0},.value={15,40},                       // 초/부품(속도 x1)과 판매가 / seconds per part at x1, sale price
 .speedStep=.25,.speedBaseCost=20,.speedGrowth=1.5,.machineCost={100,450},
 .qualityBaseCost=150,.qualityGrowth=2,.qualityFactor=.65,
 .autobuyCost=250,.autoserviceCost=600,.processCost=700,
 .risk0=.05,.riskSpeed=.012,.riskWear=.0004,.kwBase=4,.kwSpeed=4,
 .wearWarn=90,.wearLimit=120,.palletCap=40,.palletRefill=20,.startInput=20,.autoBuyDefault=30,
};
static const double INCOME_TAU=20; // 수입/분 표시의 평활 시간 / smoothing for income per minute

void PanelInit(PanelState *s,const SimConfig *cfg) {
 *s=(PanelState){.cfg=cfg,.stock=0,.supplied=cfg->startInput,.speed=1,.rng=42,.money=cfg->startMoney,.owned=1,.autoBuy=true,.autoBuyLevel=cfg->autoBuyDefault};
 for(int i=0;i<MACHINES;i++) s->machines[i]=(Machine){.enabled=true,.speed=1};
 s->machines[0].input=cfg->startInput; // 첫 기계는 팔레트에 재료를 갖고 시작 / the first machine starts with material on its pallet
}
static bool spend(PanelState *s,double cost,double *bucket){if(cost<0||s->money+1e-9<cost)return false;s->money-=cost;*bucket+=cost;return true;}
bool PanelBuyMaterial(PanelState *s,int count){
 if(count<=0||s->supplied>INT_MAX-count||!spend(s,count*s->cfg->materialPrice,&s->spentMaterial))return false;
 s->stock+=count;s->supplied+=count;return true;
}
int PanelPalletTotal(const PanelState *s){int n=0;for(int i=0;i<s->owned;i++)n+=s->machines[i].input;return n;}
bool PanelNeedsDelivery(const PanelState *s,int i){return i>=0&&i<s->owned&&s->stock>0&&s->machines[i].input<s->cfg->palletRefill;}
int PanelDeliver(PanelState *s,int i,int most){
 if(i<0||i>=s->owned)return 0;
 int n=s->cfg->palletCap-s->machines[i].input;if(n>s->stock)n=s->stock;if(n>most)n=most;if(n<0)n=0;
 s->stock-=n;s->machines[i].input+=n;return n;
}
bool PanelMaintain(PanelState *s,int id){
 if(id<0||id>=s->owned||s->machines[id].wear==0||!spend(s,s->cfg->serviceCost,&s->spentService))return false;
 s->machines[id].wear=0;return true;
}
double PanelPartValue(const PanelState *s){return s->cfg->value[s->process?1:0];}
// 시뮬레이션 초 단위 사이클 시간(빨리 감기 제외). / Cycle time in simulated seconds (fast-forward excluded).
double PanelCycleTime(const PanelState *s,int id){return s->cfg->cycle[s->process?1:0]/s->machines[id].speed;}
// 속도가 높을수록, 마모될수록 불량 위험이 커지고 품질 업그레이드가 이를 줄입니다.
// Higher speed and wear raise defect risk; quality upgrades reduce it.
double PanelDefectRisk(const PanelState *s,int id){
 const Machine *m=&s->machines[id];
 const SimConfig *c=s->cfg;
 return (c->risk0+c->riskSpeed*s->level[UP_SPEED_1+id]+c->riskWear*m->wear)*pow(c->qualityFactor,s->level[UP_QUALITY]);
}
// 다음 단계 비용, 구매할 수 없으면 -1. / Cost of the next level, or -1 if unavailable.
double PanelUpgradeCost(const PanelState *s,Upgrade u){
 int l=(u>=0&&u<UP_COUNT)?s->level[u]:0;const SimConfig *c=s->cfg;
 switch(u){
 case UP_SPEED_1:case UP_SPEED_2:case UP_SPEED_3:
  return ((int)u-UP_SPEED_1<s->owned&&l<SPEED_LEVELS)?round(c->speedBaseCost*pow(c->speedGrowth,l)):-1;
 case UP_MACHINE: return s->owned<MACHINES?c->machineCost[s->owned-1]:-1;
 case UP_QUALITY: return l<QUALITY_LEVELS?round(c->qualityBaseCost*pow(c->qualityGrowth,l)):-1;
 case UP_AUTOBUY: return l?-1:c->autobuyCost;
 case UP_AUTOSERVICE: return l?-1:c->autoserviceCost;
 case UP_PROCESS: return l?-1:c->processCost;
 default: return -1;
 }
}
bool PanelBuyUpgrade(PanelState *s,Upgrade u){
 if(!spend(s,PanelUpgradeCost(s,u),&s->spentUpgrades))return false;
 s->level[u]++;
 if(u<=UP_SPEED_3)s->machines[u-UP_SPEED_1].speed=1+s->cfg->speedStep*s->level[u];
 if(u==UP_MACHINE)s->owned++;
 if(u==UP_PROCESS)s->process=1;
 return true;
}
// 설정 기반 예상 부품/분(시뮬레이션 시간). / Expected parts per simulated minute from the configuration.
double PanelRate(const PanelState *s){double r=0;for(int i=0;i<s->owned;i++)if(s->machines[i].enabled&&s->machines[i].wear<s->cfg->wearLimit)r+=60/PanelCycleTime(s,i);return r;}
double PanelIncomePerMinute(const PanelState *s){return s->incomeEma*60;}
double PanelRecentDefectRate(const PanelState *s,int window){
 int n=window<s->recordCount?window:s->recordCount,bad=0;if(!n)return 0;
 for(int j=0;j<n;j++)bad+=s->records[(s->nextRecord-1-j+RECORDS)%RECORDS].defective;
 return (double)bad/n;
}
// 운영 비용(업그레이드 제외)과 판매 수익. / Operating cost (upgrades excluded) and sales revenue.
double PanelCost(const PanelState *s){return s->spentMaterial+s->spentEnergy+s->spentService;}
double PanelRevenue(const PanelState *s){return s->earned;}

// 자동 구매·자동 정비. 돈이 없어 멈춘 공장은 공급업체 크레딧으로 구제합니다.
// Auto-buy and auto-service; a factory stuck with no money gets supplier credit so the game can't soft-lock.
static void automation(PanelState *s){
 if(s->level[UP_AUTOBUY]&&s->autoBuy)while(s->stock<s->autoBuyLevel&&PanelBuyMaterial(s,10)){}
 if(s->level[UP_AUTOSERVICE])for(int i=0;i<s->owned;i++){Machine *m=&s->machines[i];if(!m->busy&&m->wear>=s->cfg->wearWarn)PanelMaintain(s,i);}
 bool working=false;for(int i=0;i<s->owned;i++)working|=s->machines[i].busy;
 if(!working&&s->stock==0&&PanelPalletTotal(s)==0&&s->money<s->cfg->materialPrice){s->stock+=5;s->supplied+=5;s->credits++;}
 for(int i=0;i<s->owned;i++){Machine *m=&s->machines[i];
  if(m->wear>=s->cfg->wearLimit&&s->money<s->cfg->serviceCost){bool other=false;for(int j=0;j<s->owned;j++)other|=j!=i&&s->machines[j].enabled&&s->machines[j].wear<s->cfg->wearLimit;
   if(!other){m->wear=0;s->credits++;}}}
}
// 3D 화면이 없을 때 작업자 배달을 흉내 냅니다: 출발 후 약 14초에 첫 팔레트, 이후 팔레트마다 7초.
// Without the 3D view, imitate the worker: first pallet about 14 s after setting off, then 7 s per extra pallet.
static void courier(PanelState *s,double dt){
 if(!s->courierMask){for(int i=0;i<s->owned;i++)if(PanelNeedsDelivery(s,i))s->courierMask|=1<<i;s->courierTime=0;return;}
 s->courierTime+=dt;int k=0;
 int left=0;for(int i=0;i<MACHINES;i++)left+=(s->courierMask>>i)&1;
 for(int i=0;i<MACHINES;i++)if(s->courierMask&(1<<i)){if(s->courierTime>=14+7*k){PanelDeliver(s,i,(s->stock+left-1)/left);s->courierMask&=~(1<<i);left--;}k++;}
}
void PanelUpdate(PanelState *s,double dt){
 if(!s->running||!isfinite(dt)||dt<=0)return;
 if(dt>.25)dt=.25; // 창 이동 등으로 멈춘 뒤 큰 점프를 막습니다. / Avoid a huge jump after the window stalls.
 // 작은 시간 단위로 나누어 기계 간 자원 소비를 균등하게 처리합니다.
 // Small time slices keep resource sharing fair between machines.
 while(dt>1e-9){double step=fmin(dt,.02);dt-=step;double sim=step*s->speed;s->elapsed+=sim;
 s->incomeEma*=exp(-sim/INCOME_TAU);
 automation(s);
 if(s->courier)courier(s,sim);
 for(int i=0;i<s->owned;i++){
 Machine *m=&s->machines[i];if(!m->enabled||m->wear>=s->cfg->wearLimit)continue;
 if(!m->busy){if(m->input==0){m->progress=0;continue;}
  m->input--;m->busy=true;s->rng=s->rng*1664525u+1013904223u;
  m->pendingDefect=(s->rng>>8)%10000<PanelDefectRisk(s,i)*10000;}
 double kwh=sim*(s->cfg->kwBase+s->cfg->kwSpeed*m->speed*m->speed)/3600;s->energy+=kwh;s->spentEnergy+=kwh*s->cfg->energyPrice;s->money-=kwh*s->cfg->energyPrice;
 m->progress+=sim/PanelCycleTime(s,i);
 if(m->progress>=1-1e-9){
 m->progress=fmax(0,m->progress-1);m->busy=false;m->cycles++;m->wear++;
 bool bad=m->pendingDefect;s->total++;
 if(bad){s->defects++;m->defects++;}
 else{double v=PanelPartValue(s);s->good++;m->good++;s->money+=v;s->earned+=v;s->incomeEma+=v/INCOME_TAU;}
 s->records[s->nextRecord]=(Product){s->total,i,s->elapsed,bad};s->nextRecord=(s->nextRecord+1)%RECORDS;if(s->recordCount<RECORDS)s->recordCount++;
 if(!s->contractDone&&s->earned>=s->cfg->contractGoal){s->contractDone=true;s->contractTime=s->elapsed;}
 }
 }
 }
}
int PanelAlerts(const PanelState *s,char out[][ALERT_LEN],int cap){int n=0;const SimConfig *c=s->cfg;
#define ALERT(...) do{if(n<cap)snprintf(out[n],ALERT_LEN,__VA_ARGS__);n++;}while(0)
 int onPallets=PanelPalletTotal(s);bool waiting=false;
 for(int i=0;i<s->owned;i++)waiting|=s->machines[i].enabled&&s->machines[i].input==0&&!s->machines[i].busy&&s->stock>0;
 if(s->stock==0&&onPallets==0)ALERT("OUT OF %s - Buy %s on the Inventory tab",c->materialsUpper,c->materials);
 else if(s->stock+onPallets<=10)ALERT("LOW %s STOCK - 10 %s or fewer",c->materialsUpper,c->materials);
 if(waiting)ALERT("WAITING FOR DELIVERY - %s are on the way",c->materialsUpper);
 bool due=false,stopped=false,energy=false;for(int i=0;i<s->owned;i++){due|=s->machines[i].wear>=c->wearWarn;stopped|=!s->machines[i].enabled||s->machines[i].wear>=c->wearLimit;energy|=s->machines[i].speed>2.5;}
 if(due)ALERT("MAINTENANCE DUE - Service worn machines");
 if(stopped)ALERT("MACHINE STOPPED - Check Controls tab");
 if(s->recordCount>=20&&PanelRecentDefectRate(s,20)>.12)ALERT("HIGH DEFECT RATE - Buy quality upgrades");
 if(energy&&s->running)ALERT("HIGH ENERGY USE - High speed operation");
 return n;
#undef ALERT
}
// 현재 패널 데이터를 저장합니다. 파일 열기/닫기 실패를 호출자에게 알립니다.
// Save current panel data and report file open/close failures to the caller.
bool PanelExport(const PanelState *s,const char *path){FILE *f=fopen(path,"w");if(!f)return false;
 fprintf(f,"metric,value\nfactory,%s\nprocess,%s\nmachines_owned,%d\ntotal,%d\ngood,%d\ndefects,%d\nstock_store,%d\nstock_pallets,%d\nsimulated_seconds,%.3f\nenergy_kWh,%.6f\n"
  "money_EUR,%.2f\nsales_EUR,%.2f\nmaterial_EUR,%.2f\nenergy_EUR,%.2f\nservice_EUR,%.2f\nupgrades_EUR,%.2f\ncontract_complete,%s\ncontract_seconds,%.1f\n\n"
  "product_id,machine,simulated_seconds,result\n",
  s->cfg->title,s->cfg->processName[s->process?1:0],s->owned,s->total,s->good,s->defects,s->stock,PanelPalletTotal(s),s->elapsed,s->energy,
  s->money,s->earned,s->spentMaterial,s->spentEnergy,s->spentService,s->spentUpgrades,s->contractDone?"yes":"no",s->contractTime);
 for(int j=0;j<s->recordCount;j++){int k=(s->nextRecord-s->recordCount+j+RECORDS)%RECORDS;Product p=s->records[k];fprintf(f,"%d,%d,%.3f,%s\n",p.id,p.machine+1,p.time,p.defective?"defect":"good");}
 return fclose(f)==0;
}
