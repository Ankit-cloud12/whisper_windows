@echo off
REM Generate icons for WhisperApp

echo Installing required Python packages...
pip install Pillow

echo.
echo Generating icons...
python "%~dp0generate-icons.py"

echo.
echo Icon generation complete!
pause