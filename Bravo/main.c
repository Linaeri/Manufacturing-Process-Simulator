#include "raylib.h"

int main(void)
{
    const int screenWidth = 1200;
    const int screenHeight = 700;
    const float productionInterval = 2.0f;  // Seconds per product
    const int defectChancePercent = 10;

    // Start a fresh production run instead of using sample values.
    int totalProduced = 0;
    int goodProducts = 0;
    int defectiveProducts = 0;
    bool simulationRunning = false;
    float productionTimer = 0.0f;

    InitWindow(screenWidth, screenHeight, "Metal Stamping Simulator");
    SetTargetFPS(60);

    Rectangle startButton = {760, 570, 110, 50};
    Rectangle pauseButton = {890, 570, 110, 50};
    Rectangle resetButton = {1020, 570, 110, 50};

    while (!WindowShouldClose())
    {
        // INPUT
        float deltaTime = GetFrameTime();
        bool wasRunning = simulationRunning;
        Vector2 mousePosition = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (CheckCollisionPointRec(mousePosition, startButton))
            {
                simulationRunning = true;
            }
            else if (CheckCollisionPointRec(mousePosition, pauseButton))
            {
                simulationRunning = false;
            }
            else if (CheckCollisionPointRec(mousePosition, resetButton))
            {
                simulationRunning = false;
                totalProduced = 0;
                goodProducts = 0;
                defectiveProducts = 0;
                productionTimer = 0.0f;
            }
        }

        // UPDATE
        // Skip the Start/resume frame so stopped time is not counted.
        if (simulationRunning && wasRunning)
        {
            productionTimer += deltaTime;

            // Preserve leftover time, including after a slow frame.
            while (productionTimer >= productionInterval)
            {
                productionTimer -= productionInterval;
                totalProduced++;

                if (GetRandomValue(1, 100) <= defectChancePercent)
                {
                    defectiveProducts++;
                }
                else
                {
                    goodProducts++;
                }
            }
        }

        float successRate = 0.0f;
        if (totalProduced > 0)
        {
            successRate = ((float)goodProducts / totalProduced) * 100.0f;
        }

        // DRAW
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawRectangle(0, 0, 720, 700, LIGHTGRAY);
        DrawText("SIMULATOR", 30, 30, 30, DARKGRAY);
        DrawText("Graphics integration planned", 30, 80, 20, DARKGRAY);

        DrawRectangle(720, 0, 480, 700, RAYWHITE);
        DrawText("DASHBOARD", 750, 30, 30, BLACK);
        DrawText("PRODUCTION", 750, 100, 20, DARKGRAY);
        DrawText(TextFormat("Total Produced: %d", totalProduced), 750, 140, 20, BLACK);
        DrawText(TextFormat("Good Products: %d", goodProducts), 750, 180, 20, BLACK);
        DrawText(TextFormat("Defective: %d", defectiveProducts), 750, 220, 20, BLACK);
        DrawText(TextFormat("Success Rate: %.1f%%", successRate), 750, 260, 20, BLACK);

        DrawText("MACHINE STATUS", 750, 330, 20, DARKGRAY);
        if (simulationRunning)
        {
            DrawText("Stamping Press: RUNNING", 750, 370, 20, BLACK);
            DrawText("Inspection: RUNNING", 750, 410, 20, BLACK);
        }
        else
        {
            DrawText("Stamping Press: STOPPED", 750, 370, 20, BLACK);
            DrawText("Inspection: STOPPED", 750, 410, 20, BLACK);
        }
        DrawText(TextFormat("Cycle progress: %.1f / %.1f s", productionTimer, productionInterval),
                 750, 460, 20, DARKGRAY);

        DrawText("CONTROLS", 760, 520, 20, DARKGRAY);
        DrawRectangleRec(startButton, GREEN);
        DrawRectangleRec(pauseButton, YELLOW);
        DrawRectangleRec(resetButton, RED);
        DrawText("START", 785, 585, 20, BLACK);
        DrawText("PAUSE", 913, 585, 20, BLACK);
        DrawText("RESET", 1043, 585, 20, BLACK);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
