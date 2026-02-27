@echo off
:: Create build directory if it doesn't exist
if not exist build mkdir build

:: Enter build directory
cd build

:: Delete old cache to ensure a fresh start
if exist CMakeCache.txt del /f CMakeCache.txt

:: Run the generation
cmake -G "Visual Studio 18 2026" -A x64 ..
:: Other Versions
:: cmake -G "Visual Studio 17 2022" -A x64 ..

:: Stay open so you can see if it worked or failed
pause