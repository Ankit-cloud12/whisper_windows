@echo off
setlocal enabledelayedexpansion

:: WhisperApp Dependency Collection Script
:: Collects all runtime dependencies for distribution

echo ========================================
echo WhisperApp Dependency Collection Script
echo ========================================
echo.

:: Check if running from scripts directory
if not exist "..\CMakeLists.txt" (
    echo Error: This script must be run from the scripts directory
    exit /b 1
)

:: Set paths
set PROJECT_DIR=%~dp0..
set BUILD_DIR=%PROJECT_DIR%\build\release
set DEPS_DIR=%PROJECT_DIR%\build\dependencies
set QT_DIR=C:\Qt\6.7.0\msvc2019_64
set VCREDIST_DIR=%VCToolsRedistDir%x64\Microsoft.VC143.CRT
set WHISPER_DIR=%PROJECT_DIR%\third_party\whisper.cpp\build

:: Check if release build exists
if not exist "%BUILD_DIR%\WhisperApp.exe" (
    echo Error: Release build not found. Run build-release.bat first.
    exit /b 1
)

:: Create dependencies directory
echo Creating dependencies directory...
if exist "%DEPS_DIR%" rmdir /s /q "%DEPS_DIR%"
mkdir "%DEPS_DIR%"
mkdir "%DEPS_DIR%\qt"
mkdir "%DEPS_DIR%\msvc"
mkdir "%DEPS_DIR%\whisper"
mkdir "%DEPS_DIR%\third_party"

:: Collect Qt dependencies
echo.
echo Collecting Qt dependencies...
cd "%BUILD_DIR%"

:: Use windeployqt to identify Qt dependencies
windeployqt --release --dir "%DEPS_DIR%\qt-temp" --list mapping WhisperApp.exe > "%DEPS_DIR%\qt-dependencies.txt"

:: Copy Qt runtime libraries
echo Copying Qt libraries...
copy "%QT_DIR%\bin\Qt6Core.dll" "%DEPS_DIR%\qt\"
copy "%QT_DIR%\bin\Qt6Gui.dll" "%DEPS_DIR%\qt\"
copy "%QT_DIR%\bin\Qt6Widgets.dll" "%DEPS_DIR%\qt\"
copy "%QT_DIR%\bin\Qt6Network.dll" "%DEPS_DIR%\qt\"
copy "%QT_DIR%\bin\Qt6Multimedia.dll" "%DEPS_DIR%\qt\"
copy "%QT_DIR%\bin\Qt6Concurrent.dll" "%DEPS_DIR%\qt\"
copy "%QT_DIR%\bin\Qt6Svg.dll" "%DEPS_DIR%\qt\"

:: Copy Qt plugins
echo Copying Qt plugins...
xcopy /E /I /Y "%QT_DIR%\plugins\platforms" "%DEPS_DIR%\qt\platforms"
xcopy /E /I /Y "%QT_DIR%\plugins\styles" "%DEPS_DIR%\qt\styles"
xcopy /E /I /Y "%QT_DIR%\plugins\imageformats" "%DEPS_DIR%\qt\imageformats"
xcopy /E /I /Y "%QT_DIR%\plugins\iconengines" "%DEPS_DIR%\qt\iconengines"
xcopy /E /I /Y "%QT_DIR%\plugins\audio" "%DEPS_DIR%\qt\audio"
xcopy /E /I /Y "%QT_DIR%\plugins\mediaservice" "%DEPS_DIR%\qt\mediaservice"

:: Collect MSVC runtime
echo.
echo Collecting MSVC runtime...
if exist "%VCREDIST_DIR%" (
    copy "%VCREDIST_DIR%\*.dll" "%DEPS_DIR%\msvc\"
    echo Found %VCREDIST_DIR%
) else (
    echo Warning: MSVC runtime not found at expected location
    echo Trying alternative locations...
    
    :: Try common locations
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC\*\x64\Microsoft.VC143.CRT" (
        for /d %%D in ("%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC\*") do (
            if exist "%%D\x64\Microsoft.VC143.CRT\*.dll" (
                copy "%%D\x64\Microsoft.VC143.CRT\*.dll" "%DEPS_DIR%\msvc\"
                echo Found runtime at %%D
            )
        )
    )
)

:: Download VC++ Redistributable installer
echo.
echo Downloading Visual C++ Redistributable...
if not exist "%DEPS_DIR%\vc_redist.x64.exe" (
    powershell -Command "Invoke-WebRequest -Uri 'https://aka.ms/vs/17/release/vc_redist.x64.exe' -OutFile '%DEPS_DIR%\vc_redist.x64.exe'"
)

:: Collect Whisper.cpp dependencies
echo.
echo Collecting Whisper dependencies...
if exist "%WHISPER_DIR%" (
    copy "%WHISPER_DIR%\bin\Release\whisper.dll" "%DEPS_DIR%\whisper\"
    copy "%WHISPER_DIR%\bin\Release\ggml.dll" "%DEPS_DIR%\whisper\"
) else if exist "%BUILD_DIR%\whisper.dll" (
    copy "%BUILD_DIR%\whisper.dll" "%DEPS_DIR%\whisper\"
    copy "%BUILD_DIR%\ggml.dll" "%DEPS_DIR%\whisper\"
) else (
    echo Warning: Whisper libraries not found
)

:: Collect OpenBLAS
echo.
echo Collecting OpenBLAS...
if exist "%PROJECT_DIR%\third_party\OpenBLAS\bin\libopenblas.dll" (
    copy "%PROJECT_DIR%\third_party\OpenBLAS\bin\libopenblas.dll" "%DEPS_DIR%\third_party\"
) else if exist "%BUILD_DIR%\libopenblas.dll" (
    copy "%BUILD_DIR%\libopenblas.dll" "%DEPS_DIR%\third_party\"
) else (
    echo Warning: OpenBLAS not found
)

:: Create dependency manifest
echo.
echo Creating dependency manifest...
(
echo WhisperApp Dependency Manifest
echo ==============================
echo Generated: %DATE% %TIME%
echo.
echo Qt Version: 6.7.0
echo MSVC Version: 14.3x (Visual Studio 2022)
echo.
echo Required Runtime Libraries:
echo -------------------------
echo.
echo Qt Libraries:
dir /b "%DEPS_DIR%\qt\*.dll" 2>nul
echo.
echo Qt Plugins:
dir /b /s "%DEPS_DIR%\qt\platforms" 2>nul | find ".dll"
dir /b /s "%DEPS_DIR%\qt\styles" 2>nul | find ".dll"
dir /b /s "%DEPS_DIR%\qt\imageformats" 2>nul | find ".dll"
echo.
echo MSVC Runtime:
dir /b "%DEPS_DIR%\msvc\*.dll" 2>nul
echo.
echo Whisper Libraries:
dir /b "%DEPS_DIR%\whisper\*.dll" 2>nul
echo.
echo Third-party Libraries:
dir /b "%DEPS_DIR%\third_party\*.dll" 2>nul
echo.
echo Total Size:
powershell -Command "(Get-ChildItem '%DEPS_DIR%' -Recurse | Measure-Object -Property Length -Sum).Sum / 1MB" | find "."
echo MB
) > "%DEPS_DIR%\DEPENDENCIES.txt"

:: Create installation helper script
echo.
echo Creating installation helper...
(
echo @echo off
echo :: WhisperApp Dependency Installer
echo :: This script copies all dependencies to the target directory
echo.
echo set TARGET_DIR=%%1
echo if "%%TARGET_DIR%%"=="" (
echo     echo Usage: install-deps.bat [target_directory]
echo     exit /b 1
echo )
echo.
echo echo Installing WhisperApp dependencies to %%TARGET_DIR%%...
echo.
echo :: Copy Qt libraries
echo xcopy /Y "qt\*.dll" "%%TARGET_DIR%%\"
echo xcopy /E /I /Y "qt\platforms" "%%TARGET_DIR%%\platforms"
echo xcopy /E /I /Y "qt\styles" "%%TARGET_DIR%%\styles"
echo xcopy /E /I /Y "qt\imageformats" "%%TARGET_DIR%%\imageformats"
echo xcopy /E /I /Y "qt\iconengines" "%%TARGET_DIR%%\iconengines"
echo xcopy /E /I /Y "qt\audio" "%%TARGET_DIR%%\audio"
echo.
echo :: Copy MSVC runtime
echo xcopy /Y "msvc\*.dll" "%%TARGET_DIR%%\"
echo.
echo :: Copy Whisper libraries
echo xcopy /Y "whisper\*.dll" "%%TARGET_DIR%%\"
echo.
echo :: Copy third-party libraries
echo xcopy /Y "third_party\*.dll" "%%TARGET_DIR%%\"
echo.
echo echo Dependencies installed successfully!
) > "%DEPS_DIR%\install-deps.bat"

:: Create README
echo.
echo Creating dependency README...
(
echo WhisperApp Runtime Dependencies
echo ===============================
echo.
echo This folder contains all runtime dependencies required by WhisperApp.
echo.
echo Contents:
echo ---------
echo - qt/          : Qt framework libraries and plugins
echo - msvc/        : Microsoft Visual C++ runtime libraries
echo - whisper/     : Whisper.cpp libraries
echo - third_party/ : Other third-party libraries (OpenBLAS, etc.)
echo.
echo Installation:
echo ------------
echo 1. For manual installation, copy all DLLs to the WhisperApp directory
echo 2. For automated installation, run: install-deps.bat [target_directory]
echo 3. For end users, use the installer which includes all dependencies
echo.
echo Visual C++ Redistributable:
echo --------------------------
echo The vc_redist.x64.exe installer is included for systems that don't have
echo the Visual C++ runtime installed. Run it before running WhisperApp.
echo.
echo License Information:
echo -------------------
echo - Qt: LGPLv3 / Commercial
echo - MSVC Runtime: Microsoft Software License
echo - Whisper.cpp: MIT License
echo - OpenBLAS: BSD License
echo.
echo See individual license files for details.
) > "%DEPS_DIR%\README.txt"

:: Summary
echo.
echo ========================================
echo Dependency Collection Complete!
echo ========================================
echo.
echo Dependencies collected in: %DEPS_DIR%
echo.
echo Files created:
echo - DEPENDENCIES.txt (manifest)
echo - README.txt (documentation)
echo - install-deps.bat (helper script)
echo - vc_redist.x64.exe (runtime installer)
echo.
echo Total size: 
powershell -Command "(Get-ChildItem '%DEPS_DIR%' -Recurse | Measure-Object -Property Length -Sum).Sum / 1MB"
echo MB
echo.

cd "%PROJECT_DIR%\scripts"