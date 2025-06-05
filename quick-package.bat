@echo off
echo ========================================
echo QUICK PACKAGING SCRIPT
echo ========================================

set "BUILD_DIR=build-simple"
set "PACKAGE_DIR=WhisperApp-Quick-Package"

echo [1/4] Looking for executable...
for /r "%BUILD_DIR%" %%i in (*.exe) do (
    if /i "%%~ni" == "WhisperApp" (
        set "EXE_PATH=%%i"
        echo Found: %%i
        goto :found
    )
)

echo ERROR: WhisperApp.exe not found in %BUILD_DIR%
echo Available executables:
dir /s /b "%BUILD_DIR%\*.exe" 2>nul
pause
exit /b 1

:found
echo [2/4] Creating package directory...
if exist "%PACKAGE_DIR%" rmdir /s /q "%PACKAGE_DIR%"
mkdir "%PACKAGE_DIR%"

echo [3/4] Copying executable and files...
copy "%EXE_PATH%" "%PACKAGE_DIR%\"
copy "README.md" "%PACKAGE_DIR%\"
copy "LICENSE" "%PACKAGE_DIR%\"

echo [4/4] Creating ZIP package...
powershell -Command "Compress-Archive -Path '%PACKAGE_DIR%\*' -DestinationPath 'WhisperApp-v1.0.0-portable-win64.zip' -Force"

echo ========================================
echo PACKAGE CREATED SUCCESSFULLY!
echo ========================================
echo.
echo Package: WhisperApp-v1.0.0-portable-win64.zip
echo Size:
dir "WhisperApp-v1.0.0-portable-win64.zip" | findstr /v "Directory"
echo.
echo Ready for GitHub release upload!
pause