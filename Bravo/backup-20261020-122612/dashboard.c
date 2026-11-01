// #include "dashboard.h"
#include "raylib.h"

static const Rectangle startButton = {930, 765, 120, 50};
static const Rectangle pauseButton = {1110, 765, 120, 50};
static const Rectangle resetButton = {1290, 765, 120, 50};
static const int buttonFontSize = 24;

// A one-pixel second pass gives the built-in font a stronger weight.
static void DrawDashboardText(const char *text, int x, int y, int size, Color color)
{
    DrawText(text, x, y, size, color);
    DrawText(text, x + 1, y, size, color);
}

static void DrawProductionGraph(const ProductionSample history[], int count,
                                double activeTime, int total)
{
    const Rectangle plot = {990, 310, 460, 180};
    const double startTime = history[0].time;
    double span = activeTime - startTime;
    if (span < 20.0) span = 20.0;
    const float maxTotal = (float)((total / 10 + 1) * 10);

    DrawDashboardText("CUMULATIVE PRODUCTION", 930, 250, 26, BLACK);
    for (int i = 0; i <= 2; i++)
    {
        float y = plot.y + plot.height - plot.height * i / 2.0f;
        DrawLine((int)plot.x, (int)y, (int)(plot.x + plot.width), (int)y,
                 (Color){220, 225, 230, 255});
        DrawDashboardText(TextFormat("%.0f", maxTotal * i / 2.0f),
                 930, (int)y - 10, 20, BLACK);
    }
    DrawDashboardText("Items", 930, 285, 20, BLACK);
    DrawLine((int)plot.x, (int)plot.y, (int)plot.x,
             (int)(plot.y + plot.height), GRAY);
    for (int i = 0; i <= 2; i++)
    {
        float x = plot.x + plot.width * i / 2.0f;
        const char *label = TextFormat("%.0f", startTime + span * i / 2.0);
        DrawDashboardText(label, (int)x - MeasureText(label, 20) / 2, 502, 20, BLACK);
    }
    DrawDashboardText("Active time (seconds)", 1080, 536, 20, BLACK);

    Vector2 previous = {plot.x,
        plot.y + plot.height - history[0].total / maxTotal * plot.height};
    for (int i = 1; i < count; i++)
    {
        Vector2 next = {
            plot.x + (float)((history[i].time - startTime) / span) * plot.width,
            plot.y + plot.height - history[i].total / maxTotal * plot.height
        };
        // A product count changes instantly at completion: use steps.
        Vector2 corner = {next.x, previous.y};
        DrawLineEx(previous, corner, 2.0f, BLUE);
        DrawLineEx(corner, next, 2.0f, BLUE);
        previous = next;
    }
    Vector2 end = {
        plot.x + (float)((activeTime - startTime) / span) * plot.width,
        previous.y
    };
    DrawLineEx(previous, end, 2.0f, BLUE);
    DrawCircleV(end, 3.0f, BLUE);
}

static void DrawQualityBars(int good, int defective)
{
    int largest = good > defective ? good : defective;
    if (largest < 1) largest = 1;
    DrawDashboardText("QUALITY COUNTS", 930, 585, 26, BLACK);
    DrawDashboardText("Good", 930, 632, 24, BLACK);
    DrawDashboardText("Defective", 930, 675, 24, BLACK);
    DrawRectangle(1070, 630, 290, 28, (Color){230, 234, 238, 255});
    DrawRectangle(1070, 673, 290, 28, (Color){230, 234, 238, 255});
    DrawRectangle(1070, 630, (int)(290.0 * good / largest), 28, DARKGREEN);
    DrawRectangle(1070, 673, (int)(290.0 * defective / largest), 28, MAROON);
    DrawDashboardText(TextFormat("%d", good), 1380, 632, 24, BLACK);
    DrawDashboardText(TextFormat("%d", defective), 1380, 675, 24, BLACK);
}


SimulationCommand DashboardReadInput(void)
{
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return COMMAND_NONE;
    Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, startButton)) return COMMAND_START;
    if (CheckCollisionPointRec(mouse, pauseButton)) return COMMAND_PAUSE;
    if (CheckCollisionPointRec(mouse, resetButton)) return COMMAND_RESET;
    return COMMAND_NONE;
}

void DashboardDraw(const SimulationState *state)
{
    float successRate = state->totalProduced > 0
        ? 100.0f * state->goodProducts / state->totalProduced : 0.0f;
    DrawRectangle(900, 0, 600, 900, RAYWHITE);
    DrawDashboardText("DASHBOARD", 930, 30, 38, BLACK);
    DrawDashboardText(TextFormat("Total: %d", state->totalProduced), 930, 100, 28, BLACK);
    DrawDashboardText(TextFormat("Success: %.1f%%", successRate), 1190, 100, 28, BLACK);
    DrawDashboardText(TextFormat("Stamping / Inspection: %s",
                       state->simulationRunning ? "RUNNING" : "STOPPED"),
             930, 155, 26, BLACK);
    DrawDashboardText(TextFormat("Cycle: %.1f / %.1f s", state->productionTimer, state->productionInterval),
             930, 200, 26, BLACK);

    DrawProductionGraph(state->history, state->historyCount, state->activeTime, state->totalProduced);
    DrawQualityBars(state->goodProducts, state->defectiveProducts);
    DrawDashboardText("CONTROLS", 930, 725, 26, BLACK);
    DrawRectangleRec(startButton, GREEN);
    DrawRectangleRec(pauseButton, YELLOW);
    DrawRectangleRec(resetButton, RED);
    DrawDashboardText("START",
             (int)(startButton.x + (startButton.width - MeasureText("START", buttonFontSize)) / 2),
             (int)(startButton.y + (startButton.height - buttonFontSize) / 2),
             buttonFontSize, BLACK);
    DrawDashboardText("PAUSE",
             (int)(pauseButton.x + (pauseButton.width - MeasureText("PAUSE", buttonFontSize)) / 2),
             (int)(pauseButton.y + (pauseButton.height - buttonFontSize) / 2),
             buttonFontSize, BLACK);
    DrawDashboardText("RESET",
             (int)(resetButton.x + (resetButton.width - MeasureText("RESET", buttonFontSize)) / 2),
             (int)(resetButton.y + (resetButton.height - buttonFontSize) / 2),
             buttonFontSize, BLACK);
    DrawDashboardText("Chart: latest 120 samples | Counts: entire run",
             930, 850, 18, BLACK);
}
