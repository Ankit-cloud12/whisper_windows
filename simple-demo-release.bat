@echo off
echo ========================================
echo CREATING IMMEDIATE DEMO RELEASE
echo ========================================

set "PACKAGE_NAME=WhisperApp-v1.0.0-demo"

echo [1/3] Creating demo package...
mkdir "%PACKAGE_NAME%" 2>nul

echo [2/3] Creating basic files...
copy "README.md" "%PACKAGE_NAME%\" >nul
copy "LICENSE" "%PACKAGE_NAME%\" >nul
copy "BUILD_INSTRUCTIONS.md" "%PACKAGE_NAME%\" >nul

echo Note: Executable build in progress > "%PACKAGE_NAME%\BUILD_STATUS.txt"
echo. >> "%PACKAGE_NAME%\BUILD_STATUS.txt"
echo This package contains the source code and build instructions. >> "%PACKAGE_NAME%\BUILD_STATUS.txt"
echo A pre-built executable will be available shortly. >> "%PACKAGE_NAME%\BUILD_STATUS.txt"
echo. >> "%PACKAGE_NAME%\BUILD_STATUS.txt"
echo To build yourself: >> "%PACKAGE_NAME%\BUILD_STATUS.txt"
echo 1. Install CMake and Visual Studio 2022 >> "%PACKAGE_NAME%\BUILD_STATUS.txt"
echo 2. Run: cmake .. -G "Visual Studio 17 2022" -A x64 >> "%PACKAGE_NAME%\BUILD_STATUS.txt"
echo 3. Run: cmake --build . --config Release >> "%PACKAGE_NAME%\BUILD_STATUS.txt"

echo [3/3] Creating ZIP...
tar -czf "%PACKAGE_NAME%.zip" "%PACKAGE_NAME%"

echo ========================================
echo DEMO PACKAGE READY!
echo ========================================
dir "%PACKAGE_NAME%.zip" | findstr /v "Directory"