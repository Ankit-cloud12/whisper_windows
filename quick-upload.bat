@echo off
echo ========================================
echo QUICK GITHUB UPLOAD SCRIPT
echo ========================================

set "PACKAGE_FILE=WhisperApp-v1.0.0-portable-win64.zip"

echo [1/2] Checking if package exists...
if not exist "%PACKAGE_FILE%" (
    echo ERROR: Package file not found: %PACKAGE_FILE%
    echo Run quick-package.bat first!
    pause
    exit /b 1
)

echo Found package: %PACKAGE_FILE%
echo Package size:
dir "%PACKAGE_FILE%" | findstr /v "Directory"

echo [2/2] Uploading to GitHub release...
gh release upload v1.0.0 "%PACKAGE_FILE%" --clobber
if %ERRORLEVEL% neq 0 (
    echo ERROR: Upload failed
    pause
    exit /b 1
)

echo ========================================
echo UPLOAD COMPLETED SUCCESSFULLY!
echo ========================================
echo.
echo Package %PACKAGE_FILE% uploaded to GitHub release v1.0.0
echo.
echo Check: https://github.com/Ankit-cloud12/whisper_windows/releases/tag/v1.0.0
pause