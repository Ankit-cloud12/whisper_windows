@echo off
setlocal enabledelayedexpansion

:: WhisperApp Installer Testing Script
:: Automated testing for installation packages

echo ========================================
echo WhisperApp Installer Testing Script
echo ========================================
echo.

:: Check if running from scripts directory
if not exist "..\installer\installer.nsi" (
    echo Error: This script must be run from the scripts directory
    exit /b 1
)

:: Set paths
set PROJECT_DIR=%~dp0..
set BUILD_DIR=%PROJECT_DIR%\build
set TEST_DIR=%PROJECT_DIR%\build\test-install
set TEMP_DIR=%TEMP%\WhisperApp-Test
set LOG_FILE=%BUILD_DIR%\installer-test.log

:: Get version
set /p VERSION=<"%PROJECT_DIR%\VERSION"

:: Initialize log
echo WhisperApp Installer Test Log > "%LOG_FILE%"
echo Version: %VERSION% >> "%LOG_FILE%"
echo Date: %DATE% %TIME% >> "%LOG_FILE%"
echo ======================================== >> "%LOG_FILE%"

:: Check if installer exists
set INSTALLER=%BUILD_DIR%\WhisperApp-%VERSION%-Setup.exe
if not exist "%INSTALLER%" (
    echo Error: Installer not found at %INSTALLER%
    echo Please run build-release.bat first
    exit /b 1
)

:: Clean test directories
echo Cleaning test directories...
if exist "%TEST_DIR%" rmdir /s /q "%TEST_DIR%"
if exist "%TEMP_DIR%" rmdir /s /q "%TEMP_DIR%"
mkdir "%TEST_DIR%"

:: Test 1: Verify installer integrity
echo.
echo [TEST 1] Verifying installer integrity...
echo [TEST 1] Verifying installer integrity >> "%LOG_FILE%"

:: Check file size
for %%F in ("%INSTALLER%") do set INSTALLER_SIZE=%%~zF
echo Installer size: %INSTALLER_SIZE% bytes >> "%LOG_FILE%"
if %INSTALLER_SIZE% LSS 10000000 (
    echo FAIL: Installer seems too small ^(less than 10MB^)
    echo FAIL: Installer too small >> "%LOG_FILE%"
    set /a FAILURES+=1
) else (
    echo PASS: Installer size OK
    echo PASS: Installer size OK >> "%LOG_FILE%"
)

:: Check digital signature
echo.
echo [TEST 2] Checking digital signature...
echo [TEST 2] Checking digital signature >> "%LOG_FILE%"
signtool verify /pa "%INSTALLER%" >nul 2>&1
if errorlevel 1 (
    echo WARN: Installer is not digitally signed
    echo WARN: No digital signature >> "%LOG_FILE%"
    set /a WARNINGS+=1
) else (
    echo PASS: Digital signature valid
    echo PASS: Digital signature valid >> "%LOG_FILE%"
)

:: Test 3: Silent installation
echo.
echo [TEST 3] Testing silent installation...
echo [TEST 3] Testing silent installation >> "%LOG_FILE%"
"%INSTALLER%" /S /D=%TEST_DIR%\silent-install

:: Wait for installation to complete
timeout /t 10 /nobreak >nul

if exist "%TEST_DIR%\silent-install\WhisperApp.exe" (
    echo PASS: Silent installation successful
    echo PASS: Silent installation successful >> "%LOG_FILE%"
) else (
    echo FAIL: Silent installation failed
    echo FAIL: Silent installation failed >> "%LOG_FILE%"
    set /a FAILURES+=1
)

:: Test 4: Check installed files
echo.
echo [TEST 4] Verifying installed files...
echo [TEST 4] Verifying installed files >> "%LOG_FILE%"

set REQUIRED_FILES=WhisperApp.exe Qt6Core.dll Qt6Widgets.dll README.md LICENSE
set MISSING_FILES=0

for %%F in (%REQUIRED_FILES%) do (
    if not exist "%TEST_DIR%\silent-install\%%F" (
        echo FAIL: Missing required file: %%F
        echo FAIL: Missing %%F >> "%LOG_FILE%"
        set /a MISSING_FILES+=1
    )
)

if %MISSING_FILES%==0 (
    echo PASS: All required files present
    echo PASS: All required files present >> "%LOG_FILE%"
) else (
    set /a FAILURES+=1
)

:: Test 5: Registry entries
echo.
echo [TEST 5] Checking registry entries...
echo [TEST 5] Checking registry entries >> "%LOG_FILE%"

reg query "HKLM\Software\WhisperApp" /v InstallPath >nul 2>&1
if errorlevel 1 (
    echo FAIL: Registry entries not created
    echo FAIL: Registry entries missing >> "%LOG_FILE%"
    set /a FAILURES+=1
) else (
    echo PASS: Registry entries created
    echo PASS: Registry entries created >> "%LOG_FILE%"
)

:: Test 6: File associations
echo.
echo [TEST 6] Testing file associations...
echo [TEST 6] Testing file associations >> "%LOG_FILE%"

assoc .wsp >nul 2>&1
if errorlevel 1 (
    echo WARN: File association not registered
    echo WARN: .wsp association missing >> "%LOG_FILE%"
    set /a WARNINGS+=1
) else (
    echo PASS: File associations registered
    echo PASS: File associations OK >> "%LOG_FILE%"
)

:: Test 7: Uninstaller
echo.
echo [TEST 7] Testing uninstaller...
echo [TEST 7] Testing uninstaller >> "%LOG_FILE%"

if exist "%TEST_DIR%\silent-install\uninst.exe" (
    echo PASS: Uninstaller created
    echo PASS: Uninstaller present >> "%LOG_FILE%"
    
    :: Run uninstaller silently
    "%TEST_DIR%\silent-install\uninst.exe" /S
    timeout /t 5 /nobreak >nul
    
    if not exist "%TEST_DIR%\silent-install\WhisperApp.exe" (
        echo PASS: Uninstallation successful
        echo PASS: Uninstallation OK >> "%LOG_FILE%"
    ) else (
        echo FAIL: Uninstallation failed
        echo FAIL: Uninstallation failed >> "%LOG_FILE%"
        set /a FAILURES+=1
    )
) else (
    echo FAIL: Uninstaller not found
    echo FAIL: Uninstaller missing >> "%LOG_FILE%"
    set /a FAILURES+=1
)

:: Test 8: Portable package
echo.
echo [TEST 8] Testing portable package...
echo [TEST 8] Testing portable package >> "%LOG_FILE%"

set PORTABLE_PACKAGE=%BUILD_DIR%\WhisperApp-%VERSION%-portable-win64.zip
if exist "%PORTABLE_PACKAGE%" (
    :: Extract portable package
    mkdir "%TEST_DIR%\portable"
    powershell -Command "Expand-Archive -Path '%PORTABLE_PACKAGE%' -DestinationPath '%TEST_DIR%\portable' -Force"
    
    if exist "%TEST_DIR%\portable\WhisperApp\WhisperApp.exe" (
        echo PASS: Portable package extraction successful
        echo PASS: Portable extraction OK >> "%LOG_FILE%"
        
        :: Check portable mode
        if exist "%TEST_DIR%\portable\WhisperApp\portable.txt" (
            echo PASS: Portable mode indicator present
            echo PASS: Portable mode OK >> "%LOG_FILE%"
        ) else (
            echo FAIL: Portable mode indicator missing
            echo FAIL: No portable.txt >> "%LOG_FILE%"
            set /a FAILURES+=1
        )
    ) else (
        echo FAIL: Portable package extraction failed
        echo FAIL: Portable extraction failed >> "%LOG_FILE%"
        set /a FAILURES+=1
    )
) else (
    echo SKIP: Portable package not found
    echo SKIP: No portable package >> "%LOG_FILE%"
)

:: Test 9: Version check
echo.
echo [TEST 9] Verifying version information...
echo [TEST 9] Verifying version >> "%LOG_FILE%"

:: This would require actually running the exe, which might not work in test environment
echo SKIP: Version check requires GUI environment
echo SKIP: Version check >> "%LOG_FILE%"

:: Test 10: Checksum verification
echo.
echo [TEST 10] Verifying checksums...
echo [TEST 10] Verifying checksums >> "%LOG_FILE%"

if exist "%BUILD_DIR%\WhisperApp-SHA256.txt" (
    echo INFO: Checksum file found
    echo INFO: Verifying installer checksum...
    
    :: Verify installer checksum
    certutil -hashfile "%INSTALLER%" SHA256 | find /v ":" > "%TEMP_DIR%\actual.txt"
    type "%BUILD_DIR%\WhisperApp-SHA256.txt" | find "%VERSION%-Setup.exe" > "%TEMP_DIR%\expected.txt"
    
    echo SKIP: Manual checksum comparison required
    echo SKIP: Checksum verification >> "%LOG_FILE%"
) else (
    echo WARN: Checksum file not found
    echo WARN: No checksum file >> "%LOG_FILE%"
    set /a WARNINGS+=1
)

:: Summary
echo.
echo ========================================
echo Test Summary
echo ========================================
echo.
if not defined FAILURES set FAILURES=0
if not defined WARNINGS set WARNINGS=0

echo Failures: %FAILURES%
echo Warnings: %WARNINGS%
echo.

echo ======================================== >> "%LOG_FILE%"
echo Summary: %FAILURES% failures, %WARNINGS% warnings >> "%LOG_FILE%"

if %FAILURES% GTR 0 (
    echo RESULT: TESTS FAILED
    echo Some tests failed. See %LOG_FILE% for details.
    exit /b 1
) else (
    echo RESULT: ALL TESTS PASSED
    if %WARNINGS% GTR 0 (
        echo Note: %WARNINGS% warnings encountered
    )
    echo See %LOG_FILE% for details.
)

:: Cleanup
echo.
echo Cleaning up test files...
if exist "%TEST_DIR%" rmdir /s /q "%TEST_DIR%"
if exist "%TEMP_DIR%" rmdir /s /q "%TEMP_DIR%"

cd "%PROJECT_DIR%\scripts"