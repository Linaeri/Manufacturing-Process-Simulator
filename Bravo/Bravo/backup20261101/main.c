#include "raylib.h"
#include "dashboard.h"
#include "simulation.h"

int main(void)
{
    InitWindow(1400, 800, "Metal Stamping Simulator");
    SetTargetFPS(60);

    SimulationState state;
    SimulationInit(&state);
    while (!WindowShouldClose())
    {
        SimulationUpdate(&state, DashboardReadInput(), GetFrameTime());

        BeginDrawing();
        ClearBackground(RAYWHITE);
        SimulationDraw(&state);
        DashboardDraw(&state);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
