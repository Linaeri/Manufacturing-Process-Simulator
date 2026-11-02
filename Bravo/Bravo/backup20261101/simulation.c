// Temporary simulation: replace this module with your partner's implementation.
#include "simulation.h"
#include "raylib.h"

// Keep a bounded history so long runs do not exhaust memory.
static void RecordProduction(ProductionSample history[], int *count,
                             double time, int total)
{
    if (*count == HISTORY_CAPACITY)
    {
        for (int i = 1; i < HISTORY_CAPACITY; i++)
            history[i - 1] = history[i];
        (*count)--;
    }
    history[(*count)++] = (ProductionSample){time, total};
}


void SimulationInit(SimulationState *state)
{
    *state = (SimulationState){.productionInterval = 2.0, .historyCount = 1};
}

void SimulationUpdate(SimulationState *state, SimulationCommand command, double deltaTime)
{
    bool wasRunning = state->simulationRunning;
    if (command == COMMAND_RESET)
    {
        SimulationInit(state);
        return;
    }
    if (command == COMMAND_START) state->simulationRunning = true;
    if (command == COMMAND_PAUSE) state->simulationRunning = false;
    // Do not include stopped time on the first Start/resume frame.
    if (!state->simulationRunning || !wasRunning) return;

    state->productionTimer += deltaTime;
    state->activeTime += deltaTime;
    while (state->productionTimer >= state->productionInterval)
    {
        state->productionTimer -= state->productionInterval;
        state->totalProduced++;
        if (GetRandomValue(1, 100) <= 10) state->defectiveProducts++;
        else state->goodProducts++;
        RecordProduction(state->history, &state->historyCount,
                         state->totalProduced * state->productionInterval,
                         state->totalProduced);
    }
}

void SimulationDraw(const SimulationState *state)
{
    (void)state; // Available for the partner's machine/product graphics.
    DrawRectangle(0, 0, 800, 800, LIGHTGRAY);
    DrawText("SIMULATOR", 30, 30, 38, BLACK);
    DrawText("SIMULATOR", 31, 30, 38, BLACK);
    DrawText("Graphics integration planned", 30, 90, 26, BLACK);
    DrawText("Graphics integration planned", 31, 90, 26, BLACK);
}
