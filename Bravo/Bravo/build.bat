@echo off
rem Windows build for the raylib installer (w64devkit). Double-click to build and run.
cd /d "%~dp0"
set RL=C:\raylib\raylib\src
set PATH=C:\raylib\w64devkit\bin;%PATH%
gcc -std=c99 -Wall -Wextra main.c panel_state.c factory_view.c view3d.c sawmill_state.c sawmill_panel.c sawmill_view.c ui.c -o metalworks.exe -I %RL% -L %RL% -lraylib -lopengl32 -lgdi32 -lwinmm
if errorlevel 1 (pause & exit /b 1)
gcc -std=c99 -O2 balance_test.c panel_state.c sawmill_state.c -o balance_test.exe
metalworks.exe
