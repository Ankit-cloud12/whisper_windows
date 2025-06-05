@echo off
setlocal enabledelayedexpansion

:: ========================================
:: WhisperApp Quick Local Build Script
:: Minimal dependencies version
:: ========================================

echo ========================================
echo WhisperApp Quick Build (Minimal Version)
echo ========================================
echo.

:: Check if running from scripts directory
if not exist "..\CMakeLists.txt" (
    echo Error: Run this script from the scripts directory
    pause
    exit /b 1
)

:: Set basic paths
set PROJECT_DIR=%~dp0..
set BUILD_DIR=%PROJECT_DIR%\build
set /p VERSION=<"%PROJECT_DIR%\VERSION"

echo Building WhisperApp v%VERSION%
echo.

:: Find Qt - simplified approach
set QT_PATH=
for %%D in (
    "%Qt6_DIR%"
    "C:\Qt\6.6.3\msvc2019_64"
    "C:\Qt\6.7.0\msvc2019_64" 
    "C:\Qt\6.8.0\msvc2019_64"
    "C:\Qt\6.9.1\msvc2019_64"
) do (
    if exist "%%~D\bin\qmake.exe" (
        set "QT_PATH=%%~D"
        goto :qt_found
    )
)

:qt_found
if "%QT_PATH%"=="" (
    echo ERROR: Qt not found! Install Qt 6.6+ with MSVC 2019 64-bit
    pause
    exit /b 1
)

echo Using Qt: %QT_PATH%

:: Setup MSVC (try common paths)
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
) else (
    echo ERROR: Visual Studio with MSVC not found!
    pause
    exit /b 1
)

:: Clean build
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"

:: Configure
echo Configuring...
cmake -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_PATH%" ^
    -DBUILD_TESTS=OFF ^
    "%PROJECT_DIR%"

if errorlevel 1 (
    echo CMake configure failed!
    pause
    exit /b 1
)

:: Build
echo Building...
cmake --build "%BUILD_DIR%" --config Release --parallel

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

:: Quick package
echo Packaging...
set DIST_DIR=%BUILD_DIR%\dist
mkdir "%DIST_DIR%"
copy "%BUILD_DIR%\Release\WhisperApp.exe" "%DIST_DIR%\"

cd "%DIST_DIR%"
"%QT_PATH%\bin\windeployqt.exe" --release --no-translations WhisperApp.exe
cd "%PROJECT_DIR%\scripts"

:: Copy essentials
xcopy /E /I /Y "%PROJECT_DIR%\resources" "%DIST_DIR%\resources\"
xcopy /E /I /Y "%PROJECT_DIR%\config" "%DIST_DIR%\config\"
copy "%PROJECT_DIR%\README.md" "%DIST_DIR%\"
copy "%PROJECT_DIR%\LICENSE" "%DIST_DIR%\"

:: Create portable ZIP
cd "%BUILD_DIR%"
powershell -Command "Compress-Archive -Path 'dist\*' -DestinationPath 'WhisperApp-%VERSION%-portable.zip' -Force"
cd "%PROJECT_DIR%\scripts"

echo.
echo ========================================
echo Quick Build Complete!
echo ========================================
echo.
echo Executable: %BUILD_DIR%\Release\WhisperApp.exe
echo Portable: %BUILD_DIR%\WhisperApp-%VERSION%-portable.zip
echo Distribution: %DIST_DIR%\
echo.
echo You can now:
echo 1. Test the executable directly
echo 2. Use the portable ZIP
echo 3. Run the full build-and-package.bat for installer
echo.
pause