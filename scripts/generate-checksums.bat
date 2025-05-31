@echo off
setlocal enabledelayedexpansion

:: WhisperApp Checksum Generation Script
:: Generates SHA256 checksums for all distribution files

echo ========================================
echo WhisperApp Checksum Generation Script
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
set /p VERSION=<"%PROJECT_DIR%\VERSION"

:: Check for distribution files
set FILES_FOUND=0
set CHECKSUM_FILE=%BUILD_DIR%\WhisperApp-%VERSION%-SHA256.txt

:: Initialize checksum file
echo WhisperApp %VERSION% SHA256 Checksums > "%CHECKSUM_FILE%"
echo Generated: %DATE% %TIME% >> "%CHECKSUM_FILE%"
echo ======================================== >> "%CHECKSUM_FILE%"
echo. >> "%CHECKSUM_FILE%"

:: Generate checksum for installer
set INSTALLER=%BUILD_DIR%\WhisperApp-%VERSION%-Setup.exe
if exist "%INSTALLER%" (
    echo Generating checksum for installer...
    for /f "skip=1 tokens=* delims=" %%A in ('certutil -hashfile "%INSTALLER%" SHA256 ^| findstr /v ":"') do (
        echo %%A  WhisperApp-%VERSION%-Setup.exe >> "%CHECKSUM_FILE%"
        echo Installer: %%A
        set /a FILES_FOUND+=1
        goto :done_installer
    )
    :done_installer
) else (
    echo Warning: Installer not found
)

:: Generate checksum for portable ZIP
set PORTABLE_ZIP=%BUILD_DIR%\WhisperApp-%VERSION%-portable-win64.zip
if exist "%PORTABLE_ZIP%" (
    echo Generating checksum for portable ZIP...
    for /f "skip=1 tokens=* delims=" %%A in ('certutil -hashfile "%PORTABLE_ZIP%" SHA256 ^| findstr /v ":"') do (
        echo %%A  WhisperApp-%VERSION%-portable-win64.zip >> "%CHECKSUM_FILE%"
        echo Portable ZIP: %%A
        set /a FILES_FOUND+=1
        goto :done_zip
    )
    :done_zip
) else (
    echo Warning: Portable ZIP not found
)

:: Generate checksum for self-extracting EXE
set PORTABLE_EXE=%BUILD_DIR%\WhisperApp-%VERSION%-portable-win64.exe
if exist "%PORTABLE_EXE%" (
    echo Generating checksum for self-extracting EXE...
    for /f "skip=1 tokens=* delims=" %%A in ('certutil -hashfile "%PORTABLE_EXE%" SHA256 ^| findstr /v ":"') do (
        echo %%A  WhisperApp-%VERSION%-portable-win64.exe >> "%CHECKSUM_FILE%"
        echo Self-extracting: %%A
        set /a FILES_FOUND+=1
        goto :done_exe
    )
    :done_exe
) else (
    echo Info: Self-extracting EXE not found (optional)
)

:: Add verification instructions
echo. >> "%CHECKSUM_FILE%"
echo ======================================== >> "%CHECKSUM_FILE%"
echo Verification Instructions: >> "%CHECKSUM_FILE%"
echo. >> "%CHECKSUM_FILE%"
echo Windows (Command Prompt): >> "%CHECKSUM_FILE%"
echo   certutil -hashfile filename SHA256 >> "%CHECKSUM_FILE%"
echo. >> "%CHECKSUM_FILE%"
echo Windows (PowerShell): >> "%CHECKSUM_FILE%"
echo   Get-FileHash filename -Algorithm SHA256 >> "%CHECKSUM_FILE%"
echo. >> "%CHECKSUM_FILE%"
echo Linux/macOS: >> "%CHECKSUM_FILE%"
echo   sha256sum filename >> "%CHECKSUM_FILE%"
echo. >> "%CHECKSUM_FILE%"
echo The checksum should match exactly (case-insensitive). >> "%CHECKSUM_FILE%"
echo ======================================== >> "%CHECKSUM_FILE%"

:: Create GPG signature placeholder
echo.
echo Creating GPG signature placeholder...
(
echo -----BEGIN PGP SIGNED MESSAGE-----
echo Hash: SHA256
echo.
type "%CHECKSUM_FILE%"
echo.
echo -----BEGIN PGP SIGNATURE-----
echo.
echo [Digital signature would go here]
echo [Sign with: gpg --clearsign WhisperApp-%VERSION%-SHA256.txt]
echo.
echo -----END PGP SIGNATURE-----
) > "%BUILD_DIR%\WhisperApp-%VERSION%-SHA256.txt.asc"

:: Summary
echo.
echo ========================================
echo Checksum Generation Complete!
echo ========================================
echo.
if %FILES_FOUND%==0 (
    echo Error: No distribution files found
    echo Run build-release.bat first
    exit /b 1
) else (
    echo Generated checksums for %FILES_FOUND% files
    echo.
    echo Files created:
    echo - WhisperApp-%VERSION%-SHA256.txt
    echo - WhisperApp-%VERSION%-SHA256.txt.asc (placeholder)
    echo.
    echo Location: %BUILD_DIR%
)

cd "%PROJECT_DIR%\scripts"