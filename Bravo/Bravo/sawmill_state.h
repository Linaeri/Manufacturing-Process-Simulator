#ifndef SAWMILL_STATE_H
#define SAWMILL_STATE_H
#include <stdbool.h>

// 숲 구획, 하베스터 한 대, 단계별 제재 라인. 나무는 사지 않고 자동으로 다시 자랍니다.
// Forest plots, one harvester and a staged mill line. Trees are never bought; they regrow by themselves.
#define PLOTS 6
#define TREES_PER_PLOT 8
#define MILL_STAGES 3
#define SAW_SPEED_LEVELS 8

// 구매 가능한 업그레이드. / Purchasable upgrades.
typedef enum { SU_HARVESTER, SU_STAGE_1, SU_STAGE_2, SU_STAGE_3, SU_MILL, SU_LAND, SU_COUNT } SawUpgrade;

// 제재 단계 하나: 앞에 쌓인 대기열(queue)에서 하나씩 가공합니다.
// One mill stage: processes one item at a time from the queue in front of it.
typedef struct {
 bool busy, blocked, pendingDefect;   // blocked: 끝났지만 다음 대기열이 가득 참 / finished but the next queue is full
 double progress, speed; int queue, done, scrap;
} MillStage;

typedef struct {
 bool running, contractDone;
 double speed, elapsed, money, earned, spentFuel, spentPower, spentUpgrades, incomeEma, contractTime, energy;
 unsigned rng;
 int level[SU_COUNT];
 // 숲: growth 1 = 다 자람(벨 수 있음). / Forest: growth 1 = mature, ready to fell.
 int plotsOwned; float growth[PLOTS][TREES_PER_PLOT];
 // 하베스터: 나무 하나를 베고, 가지를 치고, 통나무로 자릅니다. / Harvester: fells, strips and cross-cuts one tree.
 bool harvBusy; int harvPlot, harvTree; double harvProgress, harvSpeed, harvIdle, harvWork, starveEma; // starveEma: 최근 나무를 기다린 비율 / recent share of time spent waiting for trees
 int treesFelled, logsCut;
 // 통나무 야적장과 제재 라인 / log yard and mill line
 // roadside: 벤 통나무가 길가에서 포워더를 기다림. courier: 3D 없이 포워더 운행을 흉내 냄(밸런스 테스트).
 // roadside: felled logs waiting by the road for the forwarder. courier: imitates forwarder trips without 3D (balance test).
 int roadside; bool courier; double courierTime; int courierLoad;
 int yard, stagesOwned; MillStage stage[MILL_STAGES];
 int soldRaw, surplusRaw, sold[MILL_STAGES+1], scrapTotal;
} SawState;

// 이름과 밸런스 값 / names and balance values
extern const char *SAW_TIER_PRODUCT[MILL_STAGES+1];   // "Stripped logs", "Rough planks", ...
extern const char *SAW_STAGE_NAME[MILL_STAGES];        // "BAND SAW", "PLANER", "MOULDER"
extern const double SAW_TIER_VALUE[MILL_STAGES+1];
extern const double SAW_CONTRACT_GOAL;
#define YARD_CAP 24
#define STAGE_QUEUE_CAP 4
#define LOGS_PER_TREE 2
#define ROADSIDE_CAP 30
#define FORWARDER_LOAD 12

void SawInit(SawState *s);
void SawUpdate(SawState *s,double dt);
double SawUpgradeCost(const SawState *s,SawUpgrade u);   // -1 = 구매 불가 / unavailable
bool SawBuyUpgrade(SawState *s,SawUpgrade u);
double SawHarvestCycle(const SawState *s);               // 나무당 초 / seconds per tree
double SawStageCycle(const SawState *s,int k);           // 통나무당 초 / seconds per log
double SawStageRisk(const SawState *s,int k);
int SawMatureTrees(const SawState *s);
double SawRegrowPerMinute(const SawState *s);             // 다시 자라는 나무/분 / trees regrowing per minute
double SawHarvestPerMinute(const SawState *s);            // 하베스터가 벨 수 있는 나무/분 / trees the harvester can fell per minute
double SawIncomePerMinute(const SawState *s);
double SawRegrowSeconds(void);                            // 벤 나무가 다시 자라는 시간 / time for a felled tree to regrow
double SawMillPerMinute(const SawState *s);               // 가장 느린 단계 기준 통나무/분 / logs per minute through the slowest stage
double SawRunningCost(const SawState *s);
double SawCurrentValue(const SawState *s);
// 포워더: 길가에서 통나무를 싣고(most개까지), 야적장에 내립니다(자리만큼). / Forwarder: load from the roadside (up to most), unload into the yard (as space allows).
int SawTakeLogs(SawState *s,int most);
int SawUnloadLogs(SawState *s,int n);                // 지금 파는 제품의 통나무당 가격 / value per log of what's sold now
#define SAW_ALERT_LEN 72
int SawAlerts(const SawState *s,char out[][SAW_ALERT_LEN],int cap);
bool SawExport(const SawState *s,const char *path);
#endif
