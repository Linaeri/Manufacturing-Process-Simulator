#include "raylib.h"
// #include "dashboard.h"


int main(void)
{
    InitWindow(1500, 900, "Metal Stamping Simulator");
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
