@echo off 
echo Building WhisperApp... 
mkdir build && cd build 
cmake .. -G "Visual Studio 17 2022" -A x64 
cmake --build . --config Release 
echo Build complete! Check build\Release\ for WhisperApp.exe 
pause 
