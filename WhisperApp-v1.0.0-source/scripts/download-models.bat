@echo off
rem Windows batch script for downloading Whisper AI models
rem This script downloads the required Whisper models for speech-to-text

echo =============================================================
echo WhisperApp Model Downloader
echo =============================================================
echo.

rem Set the models directory path
set MODELS_DIR=%~dp0..\models

rem Create models directory if it doesn't exist
if not exist "%MODELS_DIR%" (
    mkdir "%MODELS_DIR%"
    echo Created models directory: %MODELS_DIR%
    echo.
)

rem Define model URLs (from Hugging Face)
set BASE_URL=https://huggingface.co/ggerganov/whisper.cpp/resolve/main

rem Available models with their sizes
echo Available Whisper models:
echo.
echo   1. tiny    (39 MB)  - Fastest, least accurate
echo   2. base    (74 MB)  - Fast, good for testing
echo   3. small   (244 MB) - Balanced speed/accuracy
echo   4. medium  (769 MB) - Good accuracy, slower
echo   5. large   (1550 MB) - Best accuracy, slowest
echo.

rem Ask user which models to download
set /p MODEL_CHOICE="Enter model numbers to download (e.g., 1,2,3) or 'all' for all models: "

rem Function to download a model
goto :main

:download_model
set MODEL_NAME=%~1
set MODEL_FILE=ggml-%MODEL_NAME%.bin
set MODEL_URL=%BASE_URL%/%MODEL_FILE%
set MODEL_PATH=%MODELS_DIR%\%MODEL_FILE%

if exist "%MODEL_PATH%" (
    echo Model %MODEL_NAME% already exists at %MODEL_PATH%
    echo Skipping download...
    goto :eof
)

echo.
echo Downloading %MODEL_NAME% model...
echo URL: %MODEL_URL%
echo Destination: %MODEL_PATH%
echo.

rem Try to use curl first (comes with Windows 10/11)
where curl >nul 2>&1
if %errorLevel% eq 0 (
    curl -L -o "%MODEL_PATH%" "%MODEL_URL%"
    if %errorLevel% eq 0 (
        echo Successfully downloaded %MODEL_NAME% model
    ) else (
        echo ERROR: Failed to download %MODEL_NAME% model
        del "%MODEL_PATH%" >nul 2>&1
    )
    goto :eof
)

rem Try PowerShell if curl is not available
echo Using PowerShell to download...
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%MODEL_URL%' -OutFile '%MODEL_PATH%'}"
if %errorLevel% eq 0 (
    echo Successfully downloaded %MODEL_NAME% model
) else (
    echo ERROR: Failed to download %MODEL_NAME% model
    del "%MODEL_PATH%" >nul 2>&1
)
goto :eof

:main
echo.
echo Starting downloads...
echo.

rem Check if user wants all models
if /i "%MODEL_CHOICE%"=="all" (
    call :download_model tiny
    call :download_model base
    call :download_model small
    call :download_model medium
    call :download_model large
    goto :complete
)

rem Download selected models
echo %MODEL_CHOICE% | findstr /i "1" >nul
if %errorLevel% eq 0 call :download_model tiny

echo %MODEL_CHOICE% | findstr /i "2" >nul
if %errorLevel% eq 0 call :download_model base

echo %MODEL_CHOICE% | findstr /i "3" >nul
if %errorLevel% eq 0 call :download_model small

echo %MODEL_CHOICE% | findstr /i "4" >nul
if %errorLevel% eq 0 call :download_model medium

echo %MODEL_CHOICE% | findstr /i "5" >nul
if %errorLevel% eq 0 call :download_model large

:complete
echo.
echo =============================================================
echo Model download complete!
echo =============================================================
echo.

rem List downloaded models
echo Downloaded models in %MODELS_DIR%:
echo.
dir /b "%MODELS_DIR%\ggml-*.bin" 2>nul
if %errorLevel% neq 0 (
    echo No models found. Please check for download errors above.
)

echo.
echo NOTE: If downloads failed, you can manually download models from:
echo https://huggingface.co/ggerganov/whisper.cpp/tree/main
echo.
echo Place the .bin files in: %MODELS_DIR%
echo.
pause