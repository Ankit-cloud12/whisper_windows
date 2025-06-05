@echo off
rem Windows batch script for setting up WhisperApp development environment
rem This script sets up the development environment for building WhisperApp

echo =============================================================
echo WhisperApp Development Environment Setup
echo =============================================================
echo.

rem Check if running as administrator
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: This script must be run as Administrator.
    echo Please right-click and select "Run as administrator"
    pause
    exit /b 1
)

echo [1/5] Checking for required tools...
echo.

rem Check for CMake
where cmake >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: CMake not found. Please install CMake from https://cmake.org/
    echo.
) else (
    echo SUCCESS: CMake found
    cmake --version
    echo.
)

rem Check for Visual Studio or Build Tools
where cl >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: Visual Studio C++ compiler not found.
    echo Please install Visual Studio 2019 or later with C++ development tools
    echo or Visual Studio Build Tools from https://visualstudio.microsoft.com/
    echo.
) else (
    echo SUCCESS: C++ compiler found
    echo.
)

rem Check for Git
where git >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: Git not found. Please install Git from https://git-scm.com/
    echo.
) else (
    echo SUCCESS: Git found
    git --version
    echo.
)

rem Check for Python (needed for some build scripts)
where python >nul 2>&1
if %errorLevel% neq 0 (
    echo WARNING: Python not found. Some build scripts may not work.
    echo Install Python from https://www.python.org/
    echo.
) else (
    echo SUCCESS: Python found
    python --version
    echo.
)

echo [2/5] Creating build directory structure...
if not exist "%~dp0..\..\build" (
    mkdir "%~dp0..\..\build"
    echo Created build directory
)

if not exist "%~dp0..\..\build\Debug" (
    mkdir "%~dp0..\..\build\Debug"
    echo Created build\Debug directory
)

if not exist "%~dp0..\..\build\Release" (
    mkdir "%~dp0..\..\build\Release"
    echo Created build\Release directory
)

echo.
echo [3/5] Setting up third-party dependencies directory...
if not exist "%~dp0..\..\third_party\whisper.cpp" (
    echo.
    echo TODO: Clone whisper.cpp repository
    echo Run: git clone https://github.com/ggerganov/whisper.cpp.git third_party/whisper.cpp
    echo.
)

if not exist "%~dp0..\..\third_party\Qt" (
    echo.
    echo TODO: Download and install Qt Framework
    echo Visit: https://www.qt.io/download-qt-installer
    echo Install Qt 6.x with MSVC compiler support
    echo.
)

echo [4/5] Setting environment variables...
rem Set Qt path (update this based on actual Qt installation)
set QT_PATH=C:\Qt\6.5.0\msvc2019_64
if exist "%QT_PATH%" (
    echo Found Qt at %QT_PATH%
    setx QT_DIR "%QT_PATH%" >nul 2>&1
    echo Set QT_DIR environment variable
) else (
    echo WARNING: Qt not found at default location
    echo Please update QT_PATH in this script after installing Qt
)

echo.
echo [5/5] Generating project files...
cd "%~dp0..\.."
if exist "CMakeLists.txt" (
    echo Generating Visual Studio solution...
    cmake -B build -G "Visual Studio 16 2019" -A x64
    if %errorLevel% eq 0 (
        echo.
        echo SUCCESS: Project files generated in build directory
        echo You can now open build\WhisperApp.sln in Visual Studio
    ) else (
        echo ERROR: Failed to generate project files
        echo Please check CMake output for errors
    )
) else (
    echo ERROR: CMakeLists.txt not found in project root
)

echo.
echo =============================================================
echo Setup complete!
echo =============================================================
echo.
echo Next steps:
echo 1. Install Qt Framework if not already installed
echo 2. Clone whisper.cpp repository to third_party directory
echo 3. Run scripts\download-models.bat to download Whisper models
echo 4. Open build\WhisperApp.sln in Visual Studio to build the project
echo.
pause