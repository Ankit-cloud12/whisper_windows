@echo off
setlocal enabledelayedexpansion

:: WhisperApp Test Build Script
:: This script performs a quick test build to verify the project compiles

echo ========================================
echo WhisperApp Test Build Script
echo ========================================
echo.

:: Check if running from scripts directory
if not exist "..\CMakeLists.txt" (
    echo Error: This script must be run from the scripts directory
    exit /b 1
)

:: Set paths
set PROJECT_DIR=%~dp0..
set BUILD_DIR=%PROJECT_DIR%\build\test
set QT_DIR=C:\Qt\6.7.0\msvc2019_64
set CMAKE_PREFIX_PATH=%QT_DIR%

:: Check for required tools
echo [1/4] Checking for required tools...
where cmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake and add it to your PATH
    exit /b 1
) else (
    echo SUCCESS: CMake found
)

where cl >nul 2>nul
if errorlevel 1 (
    echo ERROR: Visual Studio C++ compiler not found
    echo Please run this from a Visual Studio Developer Command Prompt
    exit /b 1
) else (
    echo SUCCESS: C++ compiler found
)

:: Check for Qt
if not exist "%QT_DIR%" (
    echo WARNING: Qt not found at expected location: %QT_DIR%
    echo Please update QT_DIR in this script or install Qt 6.7.0
    echo Continuing with system Qt...
    set CMAKE_PREFIX_PATH=
)

:: Clean and create build directory
echo.
echo [2/4] Preparing build directory...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

:: Configure with CMake
echo.
echo [3/4] Configuring project with CMake...
if defined CMAKE_PREFIX_PATH (
    cmake -G "Visual Studio 17 2022" -A x64 ^
        -DCMAKE_BUILD_TYPE=Debug ^
        -DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH% ^
        -DBUILD_TESTS=OFF ^
        ..\..
) else (
    cmake -G "Visual Studio 17 2022" -A x64 ^
        -DCMAKE_BUILD_TYPE=Debug ^
        -DBUILD_TESTS=OFF ^
        ..\..
)

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    echo.
    echo Common issues:
    echo - Qt not found: Install Qt 6.7.0 or update QT_DIR
    echo - Wrong Visual Studio version: Use VS 2019 or 2022
    echo - Missing dependencies: Run setup-dev-env.bat first
    exit /b 1
) else (
    echo SUCCESS: CMake configuration completed
)

:: Build debug version
echo.
echo [4/4] Building debug version...
cmake --build . --config Debug --parallel 4

if errorlevel 1 (
    echo ERROR: Build failed
    echo.
    echo Check the error messages above for details.
    echo Common issues:
    echo - Missing headers: Ensure Qt is properly installed
    echo - Linker errors: Check for missing implementations
    echo - Resource errors: Verify all resource files exist
    exit /b 1
) else (
    echo SUCCESS: Build completed successfully!
)

:: Check if executable was created
if exist "Debug\WhisperApp.exe" (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo.
    echo Executable created: %BUILD_DIR%\Debug\WhisperApp.exe
    echo.
    echo You can now:
    echo 1. Run the application to test basic functionality
    echo 2. Add whisper.cpp for full transcription features
    echo 3. Build the release version using build-release.bat
    echo.
) else (
    echo.
    echo ERROR: Build completed but executable not found
    echo This may indicate a linking issue.
    exit /b 1
)

cd "%PROJECT_DIR%\scripts"
