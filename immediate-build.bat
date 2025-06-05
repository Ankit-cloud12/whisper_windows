@echo off
echo ========================================
echo IMMEDIATE BUILD SOLUTION
echo ========================================

echo [1/3] Creating build directory...
if not exist "build-simple" mkdir "build-simple"

echo [2/3] Running CMake configuration...
cd build-simple
"C:\Program Files\CMake\bin\cmake.exe" .. -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

echo [3/3] Building the project...
"C:\Program Files\CMake\bin\cmake.exe" --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo ========================================
echo BUILD COMPLETED SUCCESSFULLY!
echo ========================================

echo Looking for executable...
dir /s /b *.exe | findstr /i whisper

echo.
echo Build completed. Check build-simple directory for outputs.
pause