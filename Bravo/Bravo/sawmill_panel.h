#ifndef SAWMILL_PANEL_H
#define SAWMILL_PANEL_H
#include "sawmill_state.h"
// 오른쪽 운영 패널: 생산, 기계, 숲, 재무. / Right-hand operations panel: Production, Machines, Forest, Finance.
void SawDrawPanel(SawState *s,int *tab,const char **notice);
#endif
