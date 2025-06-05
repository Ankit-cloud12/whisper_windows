@echo off
echo ========================================
echo AUTO-COMPLETION MONITOR
echo ========================================

echo Monitoring build completion...
echo Checking every 10 seconds for executable...
echo.

:monitor_loop
echo [%time%] Checking for WhisperApp.exe...
for /r "build-simple" %%i in (*.exe) do (
    if /i "%%~ni" == "WhisperApp" (
        echo.
        echo ========================================
        echo EXECUTABLE FOUND! Starting auto-package...
        echo ========================================
        goto :found_exe
    )
)

echo Not ready yet... waiting 10 seconds
timeout /t 10 /nobreak >nul
goto :monitor_loop

:found_exe
echo [STEP 1/2] Packaging executable...
call quick-package.bat
if %ERRORLEVEL% neq 0 (
    echo ERROR: Packaging failed
    pause
    exit /b 1
)

echo [STEP 2/2] Uploading to GitHub...
call quick-upload.bat
if %ERRORLEVEL% neq 0 (
    echo ERROR: Upload failed  
    pause
    exit /b 1
)

echo ========================================
echo MISSION ACCOMPLISHED!
echo ========================================
echo.
echo WhisperApp executable has been built, packaged, and uploaded!
echo Check: https://github.com/Ankit-cloud12/whisper_windows/releases/tag/v1.0.0
echo.
pause