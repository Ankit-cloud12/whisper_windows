@echo off
setlocal enabledelayedexpansion

:: WhisperApp Release Preparation Script
:: Master script to prepare a complete release

echo ========================================
echo WhisperApp Release Preparation Script
echo ========================================
echo.

:: Check if running from scripts directory
if not exist "..\CMakeLists.txt" (
    echo Error: This script must be run from the scripts directory
    exit /b 1
)

:: Set paths
set PROJECT_DIR=%~dp0..
set BUILD_DIR=%PROJECT_DIR%\build
set RELEASE_DIR=%BUILD_DIR%\release-package
set /p VERSION=<"%PROJECT_DIR%\VERSION"

:: Confirm version
echo Preparing release for WhisperApp v%VERSION%
echo.
set /p CONFIRM=Is this the correct version? (Y/N): 
if /i not "%CONFIRM%"=="Y" (
    echo.
    echo Please update the VERSION file first.
    exit /b 1
)

:: Create release directory
echo.
echo Creating release directory...
if exist "%RELEASE_DIR%" rmdir /s /q "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%\installers"
mkdir "%RELEASE_DIR%\checksums"
mkdir "%RELEASE_DIR%\docs"

:: Step 1: Build release
echo.
echo ========================================
echo Step 1: Building Release
echo ========================================
:: Inlined build-release.bat content
set DIST_DIR=%BUILD_DIR%\dist
set QT_DIR_ENV_VAR_NAME=Qt6_DIR
set QT_DIR_SCRIPT_VAR=C:\Qt\6.9.1\msvc2019_64

if defined %QT_DIR_ENV_VAR_NAME% (
    set "QT_INSTALL_PATH=!%QT_DIR_ENV_VAR_NAME%!"
) else if defined QT_DIR_SCRIPT_VAR (
    set "QT_INSTALL_PATH=%QT_DIR_SCRIPT_VAR%"
) else (
    echo ERROR: Qt directory not found for build-release part. Please set %QT_DIR_ENV_VAR_NAME% or QT_DIR_SCRIPT_VAR.
    exit /b 1
)
echo Using Qt from: %QT_INSTALL_PATH% for build-release part.

echo Configuring CMake for Release...
cmake -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_INSTALL_PATH%" ^
    -DBUILD_TESTS=OFF ^
    "%PROJECT_DIR%"
if errorlevel 1 (
    echo CMake configuration failed in build-release part.
    exit /b 1
)

echo Building Release version...
cmake --build "%BUILD_DIR%" --config Release --parallel
if errorlevel 1 (
    echo Release build failed in build-release part.
    exit /b 1
)

echo Packaging Release...
if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%"
copy "%BUILD_DIR%\Release\WhisperApp.exe" "%DIST_DIR%\"
echo Deploying Qt dependencies...
cd "%DIST_DIR%"
call "%QT_INSTALL_PATH%\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler --no-opengl-sw WhisperApp.exe
cd "%PROJECT_DIR%\scripts"
if errorlevel 1 (
    echo windeployqt failed in build-release part.
    exit /b 1
)
echo Copying resources and documentation for build-release part...
xcopy /E /I /Y "%PROJECT_DIR%\resources" "%DIST_DIR%\resources\"
xcopy /E /I /Y "%PROJECT_DIR%\config" "%DIST_DIR%\config\"
copy "%PROJECT_DIR%\README.md" "%DIST_DIR%\"
copy "%PROJECT_DIR%\LICENSE" "%DIST_DIR%\"
copy "%PROJECT_DIR%\CHANGELOG.md" "%DIST_DIR%\"
echo Release build and packaging complete for build-release part. Output in: %DIST_DIR%
:: End of inlined build-release.bat content

:: Step 2: Build portable package
echo.
echo ========================================
echo Step 2: Building Portable Package
echo ========================================
:: Inlined build-portable.bat content
set DIST_DIR=%BUILD_DIR%\dist
set PORTABLE_ZIP_NAME=WhisperApp-%VERSION%-portable-win64.zip
echo Creating portable package: %PORTABLE_ZIP_NAME%
if not exist "%DIST_DIR%" (
    echo ERROR: Distribution directory %DIST_DIR% not found for build-portable part.
    echo Assumed build-release part failed or was skipped.
    exit /b 1
)
echo Compressing files into %PORTABLE_ZIP_NAME% for build-portable part...
powershell -Command "Compress-Archive -Path '%DIST_DIR%\*' -DestinationPath '%BUILD_DIR%\%PORTABLE_ZIP_NAME%' -Force"
if errorlevel 1 (
    echo Failed to create portable ZIP package in build-portable part.
    exit /b 1
)
echo Portable package created: %BUILD_DIR%\%PORTABLE_ZIP_NAME%
:: End of inlined build-portable.bat content

:: Step 3: Collect dependencies
echo.
echo ========================================
echo Step 3: Collecting Dependencies
echo ========================================
call collect-dependencies.bat
if errorlevel 1 (
    echo Warning: Dependency collection had issues
)

:: Step 4: Test installer
echo.
echo ========================================
echo Step 4: Testing Installer
echo ========================================
call test-installer.bat
if errorlevel 1 (
    echo Warning: Some installer tests failed
    echo Check the log file for details
    pause
)

:: Step 5: Generate checksums
echo.
echo ========================================
echo Step 5: Generating Checksums
echo ========================================
call generate-checksums.bat
if errorlevel 1 (
    echo Error: Checksum generation failed
    exit /b 1
)

:: Step 6: Copy release files
echo.
echo ========================================
echo Step 6: Organizing Release Files
echo ========================================

:: Copy installers
echo Copying installers...
if exist "%BUILD_DIR%\WhisperApp-%VERSION%-Setup.exe" (
    copy "%BUILD_DIR%\WhisperApp-%VERSION%-Setup.exe" "%RELEASE_DIR%\installers\"
) else (
    echo Error: Installer not found
    exit /b 1
)

if exist "%BUILD_DIR%\WhisperApp-%VERSION%-portable-win64.zip" (
    copy "%BUILD_DIR%\WhisperApp-%VERSION%-portable-win64.zip" "%RELEASE_DIR%\installers\"
)

if exist "%BUILD_DIR%\WhisperApp-%VERSION%-portable-win64.exe" (
    copy "%BUILD_DIR%\WhisperApp-%VERSION%-portable-win64.exe" "%RELEASE_DIR%\installers\"
)

:: Copy checksums
echo Copying checksums...
copy "%BUILD_DIR%\*SHA256.txt*" "%RELEASE_DIR%\checksums\"

:: Copy documentation
echo Copying documentation...
copy "%PROJECT_DIR%\README.md" "%RELEASE_DIR%\docs\"
copy "%PROJECT_DIR%\INSTALL.md" "%RELEASE_DIR%\docs\"
copy "%PROJECT_DIR%\USER_GUIDE.md" "%RELEASE_DIR%\docs\"
copy "%PROJECT_DIR%\CHANGELOG.md" "%RELEASE_DIR%\docs\"
copy "%PROJECT_DIR%\LICENSE" "%RELEASE_DIR%\docs\"
copy "%PROJECT_DIR%\NOTICE" "%RELEASE_DIR%\docs\"

:: Step 7: Create release notes
echo.
echo ========================================
echo Step 7: Creating Release Notes
echo ========================================

set RELEASE_NOTES=%RELEASE_DIR%\RELEASE_NOTES.md
(
echo # WhisperApp v%VERSION% Release Notes
echo.
echo Release Date: %DATE%
echo.
echo ## Downloads
echo.
echo ### Installer (Recommended)
echo - WhisperApp-%VERSION%-Setup.exe
echo.
echo ### Portable Version  
echo - WhisperApp-%VERSION%-portable-win64.zip
echo - WhisperApp-%VERSION%-portable-win64.exe (self-extracting)
echo.
echo ## Installation
echo.
echo See INSTALL.md for detailed installation instructions.
echo.
echo ## What's New
echo.
echo See CHANGELOG.md for complete list of changes.
echo.
echo ## Verification
echo.
echo SHA256 checksums are provided in the checksums folder.
echo Verify your download before installation.
echo.
echo ## Support
echo.
echo - Documentation: docs/
echo - Issues: https://github.com/yourname/whisperapp/issues
echo - Discord: https://discord.gg/whisperapp
echo.
) > "%RELEASE_NOTES%"

:: Step 8: Create archive
echo.
echo ========================================
echo Step 8: Creating Release Archive
echo ========================================

cd "%BUILD_DIR%"
if exist "%ProgramFiles%\7-Zip\7z.exe" (
    "%ProgramFiles%\7-Zip\7z.exe" a -tzip "WhisperApp-%VERSION%-release.zip" "release-package\*"
    echo Release archive created: WhisperApp-%VERSION%-release.zip
) else (
    echo Warning: 7-Zip not found. Skipping archive creation.
)

:: Step 9: Final checklist
echo.
echo ========================================
echo Step 9: Final Checklist
echo ========================================
echo.
echo Please verify the following before publishing:
echo.
echo [ ] All files in %RELEASE_DIR%
echo [ ] Installer runs correctly
echo [ ] Portable version works
echo [ ] Checksums are correct
echo [ ] Documentation is up to date
echo [ ] Version numbers are consistent
echo [ ] No debug builds included
echo [ ] Files are digitally signed (if applicable)
echo.
echo Release package location: %RELEASE_DIR%
echo.

:: Generate file listing
echo Generating file listing...
(
echo WhisperApp v%VERSION% Release Package Contents
echo =============================================
echo Generated: %DATE% %TIME%
echo.
dir /s /b "%RELEASE_DIR%"
) > "%RELEASE_DIR%\CONTENTS.txt"

:: Summary
echo.
echo ========================================
echo Release Preparation Complete!
echo ========================================
echo.
echo Version: %VERSION%
echo Location: %RELEASE_DIR%
echo.
echo Next steps:
echo 1. Review all files in the release directory
echo 2. Test installations on clean systems
echo 3. Upload to GitHub Releases
echo 4. Update download links
echo 5. Announce the release
echo.

cd "%PROJECT_DIR%\scripts"
pause