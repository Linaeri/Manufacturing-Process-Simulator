#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "simulation.h"

SimulationCommand DashboardReadInput(void);
void DashboardDraw(const SimulationState *state);

#endif
