#ifndef PANEL_STATE_H
#define PANEL_STATE_H
#include <stdbool.h>
#define MACHINES 3
#define RECORDS 1000
#define SPEED_LEVELS 8
#define QUALITY_LEVELS 4

// 구매 가능한 업그레이드. UP_PROCESS는 두 번째 공정(성형/대패)을 엽니다.
// Purchasable upgrades; UP_PROCESS unlocks the second process (forming / planing).
typedef enum { UP_SPEED_1, UP_SPEED_2, UP_SPEED_3, UP_MACHINE, UP_QUALITY, UP_AUTOBUY, UP_AUTOSERVICE, UP_PROCESS, UP_COUNT } Upgrade;

// ---- 공장 설정: 이름과 밸런스 값. 두 공장이 같은 경제 코드를 씁니다. ----
// ---- Factory configuration: names and balance values. Both factories share the same economy code. ----
typedef struct {
 // 이름 / names
 const char *title, *subtitle;          // "METALWORKS", "METAL STAMPING PLANT"
 const char *material, *materials;       // "sheet", "sheets"
 const char *materialsUpper;             // "SHEETS"
 const char *machineName[2];             // 공정별 기계 이름 / machine name for each process
 const char *processName[2];             // "Metal stamping", "Sheet forming"
 const char *processUpgrade;             // 업그레이드 버튼 / upgrade button: "Forming"
 const char *processUpgradeLong;         // 테스트 출력 / test output: "Sheet forming"
 const char *serviceWord;                // "Service" / "New blade"
 const char *reportFile;                 // CSV 파일 이름 / CSV file name
 // 경제 / economy
 double startMoney, materialPrice, energyPrice, serviceCost, contractGoal;
 double cycle[2], value[2];              // 공정별 사이클(초)과 판매가 / per process: cycle (s) and sale price
 double speedStep, speedBaseCost, speedGrowth, machineCost[MACHINES-1];
 double qualityBaseCost, qualityGrowth, qualityFactor;
 double autobuyCost, autoserviceCost, processCost;
 double risk0, riskSpeed, riskWear;      // 불량 위험 = (기본 + 속도 단계 + 마모) × 품질 / defect risk = (base + speed level + wear) × quality
 double kwBase, kwSpeed;                 // 전력 kW = 기본 + 속도² × 계수 / power kW = base + speed² × factor
 int wearWarn, wearLimit, palletCap, palletRefill, startInput, autoBuyDefault;
} SimConfig;
extern const SimConfig METAL_CONFIG;

typedef struct { int id, machine; double time; bool defective; } Product;
// pendingDefect: 작업 시작 시 결정되어 3D 화면이 불량품 경로를 미리 알 수 있습니다.
// input: 이 기계 팔레트의 재료 수. 기계는 자기 팔레트에서만 재료를 가져갑니다.
// pendingDefect is rolled when a job starts, so the 3D view knows the part's route in advance.
// input: material on this machine's own pallet; a machine only takes material from its own pallet.
typedef struct { bool enabled, busy, pendingDefect; double speed, progress; int cycles, good, defects, wear, input; } Machine;

// 생산·경제 공유 상태입니다. / Shared production and economy state.
typedef struct {
 const SimConfig *cfg;
 // stock: 창고(구매한 재료, 아직 팔레트에 없음). / stock: the store of bought material not yet on a pallet.
 // courier: 3D 화면 없이 실행할 때 작업자 배달을 흉내 냅니다. / courier: imitates the worker's deliveries without a 3D view.
 bool running, autoBuy, contractDone, courier;
 int courierMask; double courierTime;
 int stock, supplied, total, good, defects, process, owned, autoBuyLevel, credits;
 int level[UP_COUNT];
 double speed, elapsed, energy, money, earned, spentMaterial, spentEnergy, spentService, spentUpgrades, incomeEma, contractTime;
 unsigned rng;
 Machine machines[MACHINES]; Product records[RECORDS]; int recordCount, nextRecord;
} PanelState;

void PanelInit(PanelState *s,const SimConfig *cfg);
void PanelUpdate(PanelState *s,double dt);
bool PanelBuyMaterial(PanelState *s,int count);
// 창고에서 기계 i의 팔레트로 재료를 옮깁니다(작업자 도착 시). most: 한 번의 배달에서 공평하게 나누기 위한 최대 수.
// Moves material from the store onto machine i's pallet (when the worker arrives); most = cap so one trip shares fairly.
int PanelDeliver(PanelState *s,int machine,int most);
int PanelPalletTotal(const PanelState *s);
bool PanelNeedsDelivery(const PanelState *s,int machine);
bool PanelMaintain(PanelState *s,int id);
double PanelCycleTime(const PanelState *s,int id);
double PanelDefectRisk(const PanelState *s,int id);
double PanelPartValue(const PanelState *s);
double PanelUpgradeCost(const PanelState *s,Upgrade u);
bool PanelBuyUpgrade(PanelState *s,Upgrade u);
double PanelRate(const PanelState *s);
double PanelIncomePerMinute(const PanelState *s);
double PanelRecentDefectRate(const PanelState *s,int window);
double PanelCost(const PanelState *s);
double PanelRevenue(const PanelState *s);
// 경고 문장은 재료 이름을 포함하므로 호출자의 버퍼에 씁니다. / Alerts name the material, so they're written into the caller's buffers.
#define ALERT_LEN 72
int PanelAlerts(const PanelState *s,char messages[][ALERT_LEN],int capacity);
bool PanelExport(const PanelState *s,const char *path);
#endif
