@echo off
echo =========================================
echo WhisperApp Release Workflow Trigger
echo =========================================
echo.
echo This script will help you trigger the GitHub Actions release workflow
echo for the existing v1.0.0 tag.
echo.
echo Prerequisites:
echo - GitHub CLI (gh) must be installed
echo - You must be authenticated with GitHub
echo - The release.yml workflow must be committed and pushed
echo.

:: Check if gh is installed
gh --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: GitHub CLI (gh) is not installed or not in PATH
    echo.
    echo Please install GitHub CLI from: https://cli.github.com/
    echo Or trigger the workflow manually from GitHub web interface:
    echo https://github.com/YOUR_USERNAME/whisper_windows/actions/workflows/release.yml
    echo.
    pause
    exit /b 1
)

echo GitHub CLI detected. Checking authentication...
gh auth status
if errorlevel 1 (
    echo.
    echo Please authenticate with GitHub first:
    echo gh auth login
    echo.
    pause
    exit /b 1
)

echo.
echo Current repository status:
gh repo view --json owner,name,defaultBranch

echo.
set /p CONFIRM=Trigger release workflow for tag v1.0.0? (Y/N): 
if /i not "%CONFIRM%"=="Y" (
    echo Operation cancelled.
    exit /b 0
)

echo.
echo Triggering release workflow for v1.0.0...
gh workflow run release.yml --field tag=v1.0.0

if errorlevel 1 (
    echo.
    echo ERROR: Failed to trigger workflow
    echo.
    echo Manual trigger options:
    echo 1. GitHub Web Interface:
    echo    https://github.com/YOUR_USERNAME/whisper_windows/actions/workflows/release.yml
    echo    Click "Run workflow" and enter "v1.0.0" as the tag
    echo.
    echo 2. Re-push the tag:
    echo    git tag -d v1.0.0
    echo    git push origin :refs/tags/v1.0.0
    echo    git tag v1.0.0
    echo    git push origin v1.0.0
    echo.
    pause
    exit /b 1
)

echo.
echo =========================================
echo Workflow triggered successfully!
echo =========================================
echo.
echo You can monitor the workflow progress at:
echo https://github.com/YOUR_USERNAME/whisper_windows/actions
echo.
echo The workflow will:
echo 1. Build WhisperApp for Windows
echo 2. Create NSIS installer
echo 3. Create portable ZIP package
echo 4. Generate SHA256 checksums
echo 5. Create GitHub release with all artifacts
echo.
echo Expected artifacts:
echo - WhisperApp-1.0.0-Setup.exe
echo - WhisperApp-1.0.0-portable-win64.zip
echo - SHA256 checksum files
echo.
pause