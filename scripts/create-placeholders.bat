@echo off
REM Create placeholder files for WhisperApp resources

echo Creating placeholder icon and sound files...

REM Create placeholder icon files
cd /d "%~dp0..\resources\icons"

REM Create a simple 1x1 pixel PNG placeholder for each icon
echo. > app.png
echo. > app-16.png
echo. > app-32.png
echo. > app-64.png
echo. > app-128.png
echo. > app-256.png
echo. > record.png
echo. > stop.png
echo. > pause.png
echo. > play.png
echo. > settings.png
echo. > help.png
echo. > about.png
echo. > exit.png
echo. > new.png
echo. > open.png
echo. > save.png
echo. > save-as.png
echo. > export.png
echo. > cut.png
echo. > copy.png
echo. > paste.png
echo. > undo.png
echo. > redo.png
echo. > find.png
echo. > replace.png
echo. > zoom-in.png
echo. > zoom-out.png
echo. > zoom-reset.png
echo. > fullscreen.png
echo. > online.png
echo. > offline.png
echo. > downloading.png
echo. > error.png
echo. > warning.png
echo. > info.png
echo. > success.png
echo. > microphone.png
echo. > speaker.png
echo. > headphones.png
echo. > model.png
echo. > download.png
echo. > delete.png
echo. > refresh.png
echo. > flag-us.png
echo. > flag-es.png
echo. > flag-fr.png
echo. > flag-de.png
echo. > flag-cn.png
echo. > flag-jp.png

REM Create placeholder sound files
cd /d "%~dp0..\resources\sounds"
echo. > start-recording.wav
echo. > stop-recording.wav
echo. > transcription-complete.wav
echo. > error.wav
echo. > notification.wav

echo Placeholder files created!