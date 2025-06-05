# WhisperApp Release Build Guide

## 🚨 Immediate Local Build Solution

If the GitHub Actions workflow failed, use this guide to build the installer locally and create release assets.

## Prerequisites

### Required Software
1. **Windows 10/11 (64-bit)**
2. **Visual Studio 2019/2022** with C++ Development Tools
3. **Qt 6.6.3 or later** with MSVC 2019 64-bit compiler
4. **NSIS 3.08 or later** (for installer creation)
5. **CMake 3.25 or later** (usually comes with Visual Studio)

### Quick Setup Commands
```cmd
# Install with Chocolatey (if available)
choco install nsis -y
choco install cmake -y

# Or download manually:
# Qt: https://www.qt.io/download-qt-installer
# NSIS: https://nsis.sourceforge.io/Download
# Visual Studio: https://visualstudio.microsoft.com/downloads/
```

## 🎯 Quick Build (Immediate Solution)

### Option 1: Quick Build for Testing
```cmd
cd scripts
quick-build.bat
```

This creates:
- `build/Release/WhisperApp.exe` - Standalone executable
- `build/WhisperApp-1.0.0-portable.zip` - Portable package

### Option 2: Complete Build with Installer
```cmd
cd scripts
build-and-package.bat
```

This creates:
- `build/WhisperApp-1.0.0-Setup.exe` - Full installer
- `build/WhisperApp-1.0.0-portable-win64.zip` - Portable package
- SHA256 checksums for both files

## 🔧 Troubleshooting Common Issues

### Qt Not Found
**Error**: `Qt installation not found`

**Solutions**:
1. Set environment variable: `set Qt6_DIR=C:\Qt\6.6.3\msvc2019_64`
2. Install Qt to standard location: `C:\Qt\6.6.3\msvc2019_64`
3. Update script paths in `build-and-package.bat`

### Visual Studio Not Found
**Error**: `Visual Studio with MSVC not found`

**Solutions**:
1. Install Visual Studio 2022 Community with C++ workload
2. Or install Visual Studio 2019 with MSVC v142 toolset
3. Ensure vcvars64.bat is available

### NSIS Not Found
**Error**: `NSIS not found`

**Solutions**:
1. Install NSIS: `choco install nsis -y`
2. Or download from: https://nsis.sourceforge.io/Download
3. Add to PATH: `C:\Program Files (x86)\NSIS`

### CMake Configuration Failed
**Error**: `CMake configuration failed`

**Solutions**:
1. Check Qt path is correct
2. Ensure Visual Studio is properly installed
3. Try cleaning build directory: `rmdir /s /q build`

### windeployqt Failed
**Error**: `windeployqt failed`

**Solutions**:
1. Ensure Qt bin directory is in PATH
2. Check that executable was built successfully
3. Run manually: `%Qt6_DIR%\bin\windeployqt.exe WhisperApp.exe`

## 📁 Manual Build Steps (If Scripts Fail)

### 1. Environment Setup
```cmd
# Set Qt path
set Qt6_DIR=C:\Qt\6.6.3\msvc2019_64

# Setup MSVC
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

### 2. Configure CMake
```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH=%Qt6_DIR% ^
    -DBUILD_TESTS=OFF
```

### 3. Build Application
```cmd
cmake --build build --config Release --parallel
```

### 4. Package Dependencies
```cmd
mkdir build\dist
copy build\Release\WhisperApp.exe build\dist\
cd build\dist
%Qt6_DIR%\bin\windeployqt.exe --release --no-translations WhisperApp.exe
cd ..\..
```

### 5. Copy Resources
```cmd
xcopy /E /I /Y resources build\dist\resources\
xcopy /E /I /Y config build\dist\config\
copy README.md build\dist\
copy LICENSE build\dist\
copy CHANGELOG.md build\dist\
copy USER_GUIDE.md build\dist\
mkdir build\dist\models
echo # Models directory > build\dist\models\README.txt
```

### 6. Create Portable Package
```cmd
cd build
powershell -Command "Compress-Archive -Path 'dist\*' -DestinationPath 'WhisperApp-1.0.0-portable-win64.zip' -Force"
cd ..
```

### 7. Create Installer (If NSIS Available)
```cmd
cd installer
makensis installer.nsi
cd ..
```

## 🎯 GitHub Actions Workflow Issues

### Common Workflow Failures

1. **Qt Installation Timeout**
   - Solution: Use cached Qt installation
   - Fixed in `release-fixed.yml`

2. **NSIS Download Failure**
   - Solution: Use Chocolatey for reliable installation
   - Fixed in `release-fixed.yml`

3. **File Path Issues**
   - Solution: Use PowerShell for better path handling
   - Fixed in `release-fixed.yml`

4. **Missing Dependencies**
   - Solution: Use `jurplel/install-qt-action@v4`
   - Fixed in `release-fixed.yml`

### Using Fixed Workflow
```cmd
# Trigger the fixed workflow
git tag v1.0.0
git push origin v1.0.0

# Or use workflow_dispatch in GitHub Actions tab
```

## 📦 Release Checklist

### Before Release
- [ ] Update VERSION file
- [ ] Update CHANGELOG.md
- [ ] Test build locally
- [ ] Verify installer works
- [ ] Check portable package

### Files to Upload
- [ ] `WhisperApp-1.0.0-Setup.exe`
- [ ] `WhisperApp-1.0.0-portable-win64.zip`
- [ ] `WhisperApp-1.0.0-Setup.exe.sha256`
- [ ] `WhisperApp-1.0.0-portable-win64.zip.sha256`

### Post-Release
- [ ] Test downloads work
- [ ] Update documentation
- [ ] Announce release

## 🆘 Emergency Build Process

If everything fails, use this minimal process:

1. **Build in Qt Creator**:
   - Open CMakeLists.txt in Qt Creator
   - Set Release mode
   - Build All

2. **Package Manually**:
   ```cmd
   mkdir release
   copy build\Release\WhisperApp.exe release\
   windeployqt.exe release\WhisperApp.exe
   ```

3. **Create ZIP**:
   ```cmd
   powershell -Command "Compress-Archive -Path 'release\*' -DestinationPath 'WhisperApp-emergency-build.zip'"
   ```

## 📞 Support

If you encounter issues:
1. Check this guide first
2. Review error messages carefully
3. Try the emergency build process
4. Check GitHub Issues for similar problems

## 🔄 Quick Commands Reference

```cmd
# Full build with installer
cd scripts && build-and-package.bat

# Quick build for testing
cd scripts && quick-build.bat

# Clean build
rmdir /s /q build

# Manual Qt deploy
%Qt6_DIR%\bin\windeployqt.exe --release WhisperApp.exe

# Create portable ZIP
powershell -Command "Compress-Archive -Path 'dist\*' -DestinationPath 'portable.zip'"

# Generate checksum
powershell -Command "Get-FileHash -Algorithm SHA256 'file.exe'"
```

This guide provides multiple fallback options to ensure you can always build and release WhisperApp, regardless of GitHub Actions issues.