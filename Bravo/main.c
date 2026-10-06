#include "raylib.h"

int main(void)
{
    const int screenWidth = 1200;
    const int screenHeight = 700;

    int totalProduced = 100;
    int goodProducts = 90;
    int defectiveProducts = 10;

    bool simulationRunning = false;

    InitWindow(
        screenWidth,
        screenHeight,
        "Metal Stamping Simulator"
    );

    SetTargetFPS(60);

    Rectangle startButton = {760, 570, 110, 50};
    Rectangle pauseButton = {890, 570, 110, 50};
    Rectangle resetButton = {1020, 570, 110, 50};

    while (!WindowShouldClose())
    {
        // --------------------
        // INPUT
        // --------------------

        Vector2 mousePosition = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (CheckCollisionPointRec(mousePosition, startButton))
            {
                simulationRunning = true;
            }

            if (CheckCollisionPointRec(mousePosition, pauseButton))
            {
                simulationRunning = false;
            }

            if (CheckCollisionPointRec(mousePosition, resetButton))
            {
                simulationRunning = false;

                totalProduced = 0;
                goodProducts = 0;
                defectiveProducts = 0;
            }
        }


        // --------------------
        // UPDATE
        // --------------------

        float successRate = 0.0f;

        if (totalProduced > 0)
        {
            successRate =
                ((float)goodProducts / totalProduced)
                * 100.0f;
        }


        // --------------------
        // DRAW
        // --------------------

        BeginDrawing();

        ClearBackground(RAYWHITE);


        // Left side - Simulator
        DrawRectangle(
            0, 0,
            720, 700,
            LIGHTGRAY
        );

        DrawText(
            "SIMULATOR",
            30, 30,
            30,
            DARKGRAY
        );


        // Right side - Dashboard
        DrawRectangle(
            720, 0,
            480, 700,
            RAYWHITE
        );

        DrawText(
            "DASHBOARD",
            750, 30,
            30,
            BLACK
        );


        // Production
        DrawText(
            "PRODUCTION",
            750, 100,
            20,
            DARKGRAY
        );

        DrawText(
            TextFormat(
                "Total Produced: %d",
                totalProduced
            ),
            750, 140,
            20,
            BLACK
        );

        DrawText(
            TextFormat(
                "Good Products: %d",
                goodProducts
            ),
            750, 180,
            20,
            BLACK
        );

        DrawText(
            TextFormat(
                "Defective: %d",
                defectiveProducts
            ),
            750, 220,
            20,
            BLACK
        );

        DrawText(
            TextFormat(
                "Success Rate: %.1f%%",
                successRate
            ),
            750, 260,
            20,
            BLACK
        );


        // Machine Status
        DrawText(
            "MACHINE STATUS",
            750, 330,
            20,
            DARKGRAY
        );

        if (simulationRunning)
        {
            DrawText(
                "Stamping Press: RUNNING",
                750, 370,
                20,
                BLACK
            );

            DrawText(
                "Inspection: RUNNING",
                750, 410,
                20,
                BLACK
            );
        }
        else
        {
            DrawText(
                "Stamping Press: STOPPED",
                750, 370,
                20,
                BLACK
            );

            DrawText(
                "Inspection: STOPPED",
                750, 410,
                20,
                BLACK
            );
        }


        // Controls
        DrawText(
            "CONTROLS",
            760, 520,
            20,
            DARKGRAY
        );

        DrawRectangleRec(startButton, GREEN);
        DrawRectangleRec(pauseButton, YELLOW);
        DrawRectangleRec(resetButton, RED);

        DrawText(
            "START",
            785, 585,
            20,
            BLACK
        );

        DrawText(
            "PAUSE",
            913, 585,
            20,
            BLACK
        );

        DrawText(
            "RESET",
            1043, 585,
            20,
            BLACK
        );


        EndDrawing();
    }

    CloseWindow();

    return 0;
}