@echo off
setlocal enabledelayedexpansion

:: WhisperApp Git Repository Initialization Script
:: This script helps initialize and push the WhisperApp to GitHub

echo ========================================
echo WhisperApp Git Repository Setup
echo ========================================
echo.

:: Check if git is installed
where git >nul 2>nul
if errorlevel 1 (
    echo Error: Git is not installed or not in PATH
    echo Please install Git from https://git-scm.com/
    exit /b 1
)

:: Check if already in a git repository
if exist ".git" (
    echo Warning: This directory is already a Git repository
    set /p CONTINUE=Do you want to continue? (Y/N): 
    if /i not "!CONTINUE!"=="Y" exit /b 0
)

:: Get GitHub username
set /p GITHUB_USERNAME=Enter your GitHub username: 
if "%GITHUB_USERNAME%"=="" (
    echo Error: GitHub username is required
    exit /b 1
)

:: Initialize repository
echo.
echo Initializing Git repository...
git init

:: Add all files
echo.
echo Adding files to repository...
git add .

:: Create initial commit
echo.
echo Creating initial commit...
git commit -m "Initial commit: WhisperApp v1.0.0 - Complete speech-to-text application"

:: Set main branch
echo.
echo Setting main branch...
git branch -M main

:: Add remote
echo.
echo Adding GitHub remote...
git remote add origin https://github.com/%GITHUB_USERNAME%/WhisperApp.git

:: Show status
echo.
echo Repository initialized successfully!
echo.
git status

echo.
echo ========================================
echo Next Steps:
echo ========================================
echo.
echo 1. Create a new repository on GitHub:
echo    https://github.com/new
echo    - Name: WhisperApp
echo    - Set to Public
echo    - Don't initialize with README
echo.
echo 2. Push to GitHub:
echo    git push -u origin main
echo.
echo 3. Create your first release:
echo    git tag -a v1.0.0 -m "Initial release"
echo    git push origin v1.0.0
echo.
echo 4. Check GitHub Actions:
echo    https://github.com/%GITHUB_USERNAME%/WhisperApp/actions
echo.
echo For detailed instructions, see GITHUB_SETUP.md
echo.

pause