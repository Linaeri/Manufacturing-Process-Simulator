@echo off
rem Runs the balance test and keeps the window open.
rem DELAY = how often the test player acts (seconds). FACTORY = metal or saw.
cd /d "%~dp0"
set DELAY=4
set FACTORY=metal
if not "%~1"=="" set DELAY=%~1
if not "%~2"=="" set FACTORY=%~2
balance_test.exe %DELAY% %FACTORY%
echo.
pause
