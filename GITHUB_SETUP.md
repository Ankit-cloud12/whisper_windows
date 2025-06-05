# GitHub Setup and Release Guide

This guide explains how to push WhisperApp to GitHub and use the automated release workflow.

## Prerequisites

- Git installed on your system
- GitHub account
- GitHub CLI (optional but recommended): https://cli.github.com/

## Step 1: Initialize Git Repository

Open a terminal in the WhisperApp directory and run:

```cmd
cd c:\Users\Admin\vs_code\exported-assets\WhisperApp
git init
git add .
git commit -m "Initial commit: WhisperApp v1.0.0"
```

## Step 2: Create GitHub Repository

### Option A: Using GitHub CLI (Recommended)
```cmd
gh repo create WhisperApp --public --source=. --remote=origin --push
```

### Option B: Using GitHub Web Interface
1. Go to https://github.com/new
2. Repository name: `WhisperApp`
3. Description: `Fast, accurate speech-to-text transcription powered by OpenAI's Whisper`
4. Set to Public
5. Don't initialize with README (we already have one)
6. Create repository

Then link your local repository:
```cmd
git remote add origin https://github.com/YOUR_USERNAME/WhisperApp.git
git branch -M main
git push -u origin main
```

## Step 3: Configure Repository Settings

1. Go to your repository on GitHub
2. Navigate to Settings → Secrets and variables → Actions
3. Add any required secrets (if using code signing):
   - `SIGN_CERT`: Base64 encoded certificate
   - `SIGN_PASS`: Certificate password

## Step 4: Enable GitHub Actions

GitHub Actions should be enabled by default. The workflows will trigger automatically:

- **Build Workflow** (`.github/workflows/build.yml`):
  - Triggers on every push to main/develop branches
  - Triggers on pull requests
  - Runs tests and code quality checks

- **Release Workflow** (`.github/workflows/release.yml`):
  - Triggers when you push a version tag
  - Builds installer and portable packages
  - Creates draft GitHub release

## Step 5: Creating a Release

### 1. Update Version
```cmd
REM Update version in VERSION file
echo 1.0.1 > VERSION

REM Update CHANGELOG.md with release notes
notepad CHANGELOG.md

REM Commit changes
git add VERSION CHANGELOG.md
git commit -m "Bump version to 1.0.1"
git push
```

### 2. Create and Push Tag
```cmd
REM Create annotated tag
git tag -a v1.0.1 -m "Release v1.0.1"

REM Push tag to trigger release workflow
git push origin v1.0.1
```

### 3. Monitor Release Progress
1. Go to Actions tab in your GitHub repository
2. Watch the "Release WhisperApp" workflow
3. Once complete, go to Releases tab
4. You'll see a draft release with all artifacts

### 4. Publish Release
1. Edit the draft release
2. Review auto-generated release notes
3. Add any additional information
4. Click "Publish release"

## Automated Release Process

When you push a version tag, the workflow automatically:

1. **Builds Release**:
   - Sets up Qt 6.7.0 environment
   - Configures MSVC compiler
   - Downloads Whisper models
   - Builds release version

2. **Creates Packages**:
   - NSIS installer: `WhisperApp-v1.0.1-Setup.exe`
   - Portable ZIP: `WhisperApp-v1.0.1-portable-win64.zip`
   - SHA256 checksums

3. **Uploads Artifacts**:
   - Uploads all packages to GitHub release
   - Includes checksums for verification
   - Creates draft release with notes

## Manual Workflow Trigger

You can also trigger the release workflow manually:

1. Go to Actions → Release WhisperApp
2. Click "Run workflow"
3. Enter version number (e.g., "1.0.1")
4. Click "Run workflow"

## Build Status Badges

Add these badges to your README.md:

```markdown
[![Build Status](https://github.com/YOUR_USERNAME/WhisperApp/workflows/Build%20and%20Test/badge.svg)](https://github.com/YOUR_USERNAME/WhisperApp/actions)
[![Release](https://img.shields.io/github/v/release/YOUR_USERNAME/WhisperApp)](https://github.com/YOUR_USERNAME/WhisperApp/releases)
[![License](https://img.shields.io/github/license/YOUR_USERNAME/WhisperApp)](LICENSE)
```

## Troubleshooting

### Build Failures
- Check Actions tab for detailed logs
- Ensure all dependencies are correctly specified
- Verify Qt version matches your local setup

### Release Creation Issues
- Ensure tag follows format: `v1.0.0`
- Check that VERSION file is updated
- Verify GitHub token has required permissions

### Missing Artifacts
- Check build logs for errors
- Ensure scripts have correct paths
- Verify NSIS is installed in workflow

## Development Workflow

1. Create feature branch:
   ```cmd
   git checkout -b feature/new-feature
   ```

2. Make changes and commit:
   ```cmd
   git add .
   git commit -m "Add new feature"
   ```

3. Push branch and create PR:
   ```cmd
   git push -u origin feature/new-feature
   gh pr create
   ```

4. After PR approval and merge, delete branch:
   ```cmd
   git checkout main
   git pull
   git branch -d feature/new-feature
   ```

## Security Notes

- Never commit sensitive data
- Use GitHub Secrets for certificates/passwords
- Enable branch protection for main branch
- Require PR reviews for production releases
- Enable Dependabot for security updates

## Support

For issues with:
- GitHub Actions: Check workflow logs
- Release process: See `.github/workflows/release.yml`
- Build errors: See `.github/workflows/build.yml`

Remember to update `YOUR_USERNAME` with your actual GitHub username throughout this guide.