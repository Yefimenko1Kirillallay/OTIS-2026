@echo off
setlocal
echo Configuring and building Task 02...
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
if errorlevel 1 exit /b 1
cmake --build build --target Task02
if errorlevel 1 exit /b 1
echo Starting the application...
build\Task02.exe
endlocal
