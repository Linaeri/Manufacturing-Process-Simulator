#ifndef PANEL_STATE_H
#define PANEL_STATE_H
#include <stdbool.h>
#define MACHINES 3
#define RECORDS 1000
typedef struct { int id, machine; double time; bool defective; } Product;
typedef struct { bool enabled, busy; double speed, progress; int cycles, good, defects, wear; } Machine;
// 생산과 패널이 공유하는 상태입니다. Play 중 시간에 따라 갱신됩니다.
// Shared production and panel state, updated over time during Play.
typedef struct {
 bool running; int stock, supplied, total, good, defects, target, process;
 double speed, elapsed, energy; unsigned rng;
 Machine machines[MACHINES]; Product records[RECORDS]; int recordCount, nextRecord;
} PanelState;
void PanelInit(PanelState *s);
void PanelUpdate(PanelState *s,double dt);
void PanelRestock(PanelState *s);
void PanelMaintain(PanelState *s,int id);
double PanelRate(const PanelState *s);
double PanelCost(const PanelState *s);
double PanelRevenue(const PanelState *s);
int PanelAlerts(const PanelState *s,const char **messages,int capacity);
bool PanelExport(const PanelState *s,const char *path);
#endif
