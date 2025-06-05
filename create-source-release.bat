@echo off
echo ========================================
echo CREATING IMMEDIATE SOURCE RELEASE
echo ========================================

set "PACKAGE_NAME=WhisperApp-v1.0.0-source"
set "PACKAGE_DIR=%PACKAGE_NAME%"

echo [1/4] Creating source package directory...
if exist "%PACKAGE_DIR%" rmdir /s /q "%PACKAGE_DIR%"
mkdir "%PACKAGE_DIR%"

echo [2/4] Copying source files...
xcopy src "%PACKAGE_DIR%\src" /E /I /Q
xcopy third_party "%PACKAGE_DIR%\third_party" /E /I /Q
xcopy resources "%PACKAGE_DIR%\resources" /E /I /Q
xcopy config "%PACKAGE_DIR%\config" /E /I /Q
xcopy scripts "%PACKAGE_DIR%\scripts" /E /I /Q
copy CMakeLists.txt "%PACKAGE_DIR%\"
copy README.md "%PACKAGE_DIR%\"
copy LICENSE "%PACKAGE_DIR%\"
copy BUILD_INSTRUCTIONS.md "%PACKAGE_DIR%\"
copy USER_GUIDE.md "%PACKAGE_DIR%\"

echo [3/4] Creating quick build scripts for users...
echo @echo off > "%PACKAGE_DIR%\BUILD.bat"
echo echo Building WhisperApp... >> "%PACKAGE_DIR%\BUILD.bat"
echo mkdir build ^&^& cd build >> "%PACKAGE_DIR%\BUILD.bat"
echo cmake .. -G "Visual Studio 17 2022" -A x64 >> "%PACKAGE_DIR%\BUILD.bat"
echo cmake --build . --config Release >> "%PACKAGE_DIR%\BUILD.bat"
echo echo Build complete! Check build\Release\ for WhisperApp.exe >> "%PACKAGE_DIR%\BUILD.bat"
echo pause >> "%PACKAGE_DIR%\BUILD.bat"

echo [4/4] Creating ZIP package...
powershell -Command "Compress-Archive -Path '%PACKAGE_DIR%\*' -DestinationPath '%PACKAGE_NAME%.zip' -Force"

echo ========================================
echo SOURCE RELEASE CREATED!
echo ========================================
echo Package: %PACKAGE_NAME%.zip
dir "%PACKAGE_NAME%.zip" | findstr /v "Directory"
echo.
echo Ready for immediate upload to GitHub!