#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdbool.h>

#define HISTORY_CAPACITY 120

typedef enum SimulationCommand
{
    COMMAND_NONE,
    COMMAND_START,
    COMMAND_PAUSE,
    COMMAND_RESET
} SimulationCommand;

typedef struct ProductionSample
{
    double time;
    int total;
} ProductionSample;

// Shared contract: simulation writes the data; dashboard only reads it.
// Record one history sample per inspected product, using active elapsed time.
// Keep historyCount between 1 and HISTORY_CAPACITY, with ordered timestamps.
typedef struct SimulationState
{
    int totalProduced;
    int goodProducts;
    int defectiveProducts;
    bool simulationRunning;
    double productionTimer;
    double productionInterval;
    double activeTime;
    ProductionSample history[HISTORY_CAPACITY];
    int historyCount;
} SimulationState;

void SimulationInit(SimulationState *state);
void SimulationUpdate(SimulationState *state, SimulationCommand command, double deltaTime);
void SimulationDraw(const SimulationState *state);

#endif
