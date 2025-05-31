@echo off
REM Generate test data for WhisperApp testing

echo ========================================
echo WhisperApp Test Data Generator
echo ========================================
echo.

set TEST_DATA_DIR=%~dp0..\data
set AUDIO_DIR=%TEST_DATA_DIR%\audio
set CONFIG_DIR=%TEST_DATA_DIR%\configs
set EXPECTED_DIR=%TEST_DATA_DIR%\expected

REM Create directories
if not exist "%AUDIO_DIR%" mkdir "%AUDIO_DIR%"
if not exist "%CONFIG_DIR%" mkdir "%CONFIG_DIR%"
if not exist "%EXPECTED_DIR%" mkdir "%EXPECTED_DIR%"

echo Creating test audio files...
echo (Note: This creates placeholder files. Real audio generation requires additional tools)

REM Create placeholder audio files
echo. > "%AUDIO_DIR%\silence.wav"
echo. > "%AUDIO_DIR%\speech_english.wav"
echo. > "%AUDIO_DIR%\speech_noisy.wav"
echo. > "%AUDIO_DIR%\music.wav"
echo. > "%AUDIO_DIR%\mixed_content.wav"

echo Creating test configuration files...

REM Minimal configuration
(
echo {
echo   "language": "en",
echo   "model": "base.en",
echo   "sampleRate": 16000
echo }
) > "%CONFIG_DIR%\minimal.json"

REM Full configuration
(
echo {
echo   "general": {
echo     "language": "en",
echo     "autoStart": true,
echo     "minimizeToTray": true,
echo     "showNotifications": true,
echo     "checkForUpdates": true
echo   },
echo   "recording": {
echo     "inputDevice": "",
echo     "sampleRate": 16000,
echo     "channels": 1,
echo     "bitsPerSample": 16,
echo     "vadEnabled": true,
echo     "vadThreshold": 0.5,
echo     "vadPaddingMs": 300,
echo     "maxDuration": 300
echo   },
echo   "transcription": {
echo     "model": "base.en",
echo     "computeType": "auto",
echo     "translateToEnglish": false,
echo     "maxSegmentLength": 0,
echo     "wordTimestamps": true,
echo     "numThreads": 0,
echo     "beamSize": 5,
echo     "temperature": 0.0
echo   },
echo   "hotkeys": {
echo     "record": "Ctrl+Shift+R",
echo     "pause": "Ctrl+Shift+P",
echo     "stop": "Ctrl+Shift+S",
echo     "cancel": "Escape"
echo   },
echo   "output": {
echo     "autoCopyToClipboard": true,
echo     "autoTypeOutput": false,
echo     "saveTranscriptions": true,
echo     "outputDirectory": "./transcriptions",
echo     "transcriptionFormat": "txt",
echo     "timestampFormat": "[%H:%M:%S]"
echo   }
echo }
) > "%CONFIG_DIR%\full.json"

REM Invalid configuration for error testing
(
echo {
echo   "language": 123,
echo   "model": null,
echo   "sampleRate": "invalid",
echo   "vadThreshold": 2.0
echo }
) > "%CONFIG_DIR%\invalid.json"

echo Creating expected output files...

REM Expected transcription
(
echo This is a test transcription.
echo It contains multiple sentences.
echo The audio quality is good.
) > "%EXPECTED_DIR%\speech_english.txt"

REM Expected SRT format
(
echo 1
echo 00:00:00,000 --^> 00:00:02,000
echo This is a test transcription.
echo.
echo 2
echo 00:00:02,000 --^> 00:00:04,000
echo It contains multiple sentences.
echo.
echo 3
echo 00:00:04,000 --^> 00:00:06,000
echo The audio quality is good.
) > "%EXPECTED_DIR%\speech_english.srt"

REM Expected translation
(
echo This is a test transcription.
echo It has been translated to English.
) > "%EXPECTED_DIR%\speech_translated.txt"

echo.
echo Test data generation complete!
echo.
echo Generated files:
echo   Audio:    %AUDIO_DIR%
echo   Configs:  %CONFIG_DIR%
echo   Expected: %EXPECTED_DIR%
echo.
echo Note: Audio files are placeholders. Use external tools to generate real audio samples.
echo.