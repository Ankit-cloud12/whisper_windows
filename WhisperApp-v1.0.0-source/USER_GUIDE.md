# WhisperApp User Guide

Welcome to WhisperApp! This guide will help you get started with real-time speech-to-text transcription.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Basic Usage](#basic-usage)
3. [Recording Audio](#recording-audio)
4. [Managing Transcriptions](#managing-transcriptions)
5. [Settings and Customization](#settings-and-customization)
6. [Keyboard Shortcuts](#keyboard-shortcuts)
7. [Tips and Tricks](#tips-and-tricks)
8. [Troubleshooting](#troubleshooting)

## Getting Started

### First Launch

When you first launch WhisperApp, you'll see:

![Main Window](docs/images/main-window.png)

1. **Model Selection** - Choose your Whisper model
2. **Language Selection** - Set transcription language
3. **Record Button** - Start/stop recording
4. **Audio Level Indicator** - Shows input level
5. **Transcription Area** - Displays transcribed text

### Downloading Models

Before you can start transcribing, you need to download at least one Whisper model:

1. Go to **Tools → Model Manager**
2. Select a model based on your needs:
   - **Tiny (75MB)** - Fastest, good for quick notes
   - **Base (142MB)** - Recommended for most users
   - **Small (466MB)** - Better accuracy
   - **Medium (1.5GB)** - High accuracy
   - **Large (3GB)** - Best accuracy
3. Click **Download**

![Model Manager](docs/images/model-manager.png)

## Basic Usage

### Quick Start

1. **Select Model** - Choose your downloaded model from the dropdown
2. **Press Record** - Click the record button or use `Ctrl+Shift+Space`
3. **Speak Clearly** - Talk naturally into your microphone
4. **Stop Recording** - Click stop or press the hotkey again
5. **Review Text** - Your transcription appears immediately

### System Tray

WhisperApp runs in the system tray for quick access:

![System Tray](docs/images/system-tray.png)

- **Right-click** for menu options
- **Double-click** to show/hide main window
- **Hover** to see current status

## Recording Audio

### Recording Methods

#### Method 1: Main Window
- Click the large **Record** button
- Recording indicator shows duration
- Click again to stop

#### Method 2: Global Hotkey
- Press `Ctrl+Shift+Space` from anywhere
- No need to switch windows
- Press again to stop

#### Method 3: System Tray
- Right-click tray icon
- Select **Start Recording**
- Select **Stop Recording** when done

### Recording Tips

- **Speak clearly** - Normal conversation speed works best
- **Minimize background noise** - Find a quiet environment
- **Position microphone** - Keep it 6-12 inches from your mouth
- **Test levels** - Watch the audio level indicator

### Auto-Stop Features

- **Voice Activity Detection** - Stops when you stop speaking
- **Maximum Duration** - Prevents accidental long recordings
- **Customizable** - Adjust in Settings → Audio

## Managing Transcriptions

### During Recording

- **Real-time display** - Text appears as you speak
- **Confidence indicators** - Color-coded accuracy levels
- **Timestamps** - Optional time markers

### After Recording

#### Copy to Clipboard
- Press `Ctrl+C` or use **Edit → Copy**
- Automatically copied if enabled in settings

#### Insert into Applications
1. Place cursor where you want text
2. Press `Ctrl+Shift+V` or configured hotkey
3. Text is typed automatically

#### Save Transcription
- **File → Save** (`Ctrl+S`) - Save as text file
- **File → Export** (`Ctrl+E`) - Multiple formats available

### Export Formats

![Export Dialog](docs/images/export-dialog.png)

- **Plain Text** (.txt) - Simple text format
- **Timestamped** (.txt) - With time markers
- **SRT Subtitle** (.srt) - For video captioning
- **Markdown** (.md) - Formatted documentation
- **HTML** (.html) - Web-ready with styling
- **JSON** (.json) - Structured data

## Settings and Customization

### General Settings

![Settings General](docs/images/settings-general.png)

- **Language** - UI language selection
- **Theme** - Light, Dark, or System
- **Startup** - Launch with Windows
- **Tray** - Minimize to system tray

### Audio Settings

![Settings Audio](docs/images/settings-audio.png)

- **Input Device** - Select microphone
- **Sample Rate** - Audio quality (16kHz recommended)
- **Noise Reduction** - Enable for cleaner audio
- **Voice Detection** - Auto-stop when silent

### Transcription Settings

![Settings Transcription](docs/images/settings-transcription.png)

- **Default Model** - Preferred Whisper model
- **Language** - Target language or Auto
- **Auto-Save** - Save transcriptions automatically
- **Copy to Clipboard** - Auto-copy when done

### Hotkey Configuration

![Settings Hotkeys](docs/images/settings-hotkeys.png)

Customize all keyboard shortcuts:
1. Click on a hotkey field
2. Press your desired key combination
3. Click **Apply**

**Preset Options:**
- Default - Standard configuration
- Minimal - Essential keys only
- Media Keys - Use multimedia buttons
- Accessibility - Single-key shortcuts

## Keyboard Shortcuts

### Default Hotkeys

| Action | Shortcut |
|--------|----------|
| Toggle Recording | `Ctrl+Shift+Space` |
| Start Recording | `Ctrl+Shift+R` |
| Stop Recording | `Ctrl+Shift+S` |
| Cancel Recording | `Escape` |
| Copy Last Transcription | `Ctrl+Shift+C` |
| Insert Text | `Ctrl+Shift+V` |
| Show/Hide Window | `Ctrl+Shift+W` |

### Editing Shortcuts

| Action | Shortcut |
|--------|----------|
| Copy | `Ctrl+C` |
| Cut | `Ctrl+X` |
| Paste | `Ctrl+V` |
| Select All | `Ctrl+A` |
| Undo | `Ctrl+Z` |
| Redo | `Ctrl+Y` |
| Find | `Ctrl+F` |

## Tips and Tricks

### Productivity Tips

1. **Quick Dictation**
   - Use global hotkeys to record without switching windows
   - Enable "Type in Active Window" for instant insertion

2. **Meeting Notes**
   - Use continuous recording mode
   - Export as timestamped text for easy review

3. **Multi-language**
   - Set language to "Auto" for mixed content
   - Switch models for different accuracy needs

### Advanced Features

#### Custom Export Templates
Edit `config/export-templates.json` to create custom formats

#### Batch Processing
Process multiple audio files:
```cmd
WhisperApp.exe --batch --input "C:\AudioFiles" --output "C:\Transcripts"
```

#### Command Line Usage
```cmd
WhisperApp.exe [options]
  --model <name>     Use specific model
  --language <code>  Set language (en, es, fr, etc.)
  --file <path>      Transcribe audio file
  --minimize         Start minimized
```

### Performance Optimization

1. **GPU Acceleration**
   - Enable in Settings → Performance
   - Requires NVIDIA GPU with CUDA

2. **Model Selection**
   - Use smaller models for real-time needs
   - Large models for accuracy-critical work

3. **Memory Management**
   - Close unused applications
   - Limit concurrent transcriptions

## Troubleshooting

### Common Issues

#### No Audio Input
- Check Windows sound settings
- Ensure microphone is not muted
- Try different audio device
- Check privacy settings

#### Poor Transcription Quality
- Use a better microphone
- Reduce background noise
- Try a larger model
- Speak more clearly

#### Application Crashes
- Update graphics drivers
- Disable GPU acceleration
- Check available memory
- Reinstall application

#### Hotkeys Not Working
- Run as administrator
- Check for conflicts
- Reset to defaults
- Disable other hotkey apps

### Getting Help

1. **Built-in Help**
   - Press `F1` or **Help → Help**
   - Hover over UI elements for tooltips

2. **Error Messages**
   - Read the full message
   - Follow suggested solutions
   - Check log files if needed

3. **Support Resources**
   - GitHub Issues
   - Community Discord
   - Email support

### Debug Mode

Enable detailed logging:
1. Settings → Advanced → Debug Mode
2. Reproduce the issue
3. Find logs in `%APPDATA%\WhisperApp\logs`

## Accessibility

### Screen Reader Support
- Full NVDA/JAWS compatibility
- Descriptive labels on all controls
- Keyboard navigation throughout

### Visual Accommodations
- High contrast theme available
- Adjustable font sizes
- Color-blind friendly indicators

### Keyboard Navigation
- `Tab` - Move between controls
- `Space` - Activate buttons
- `Arrow keys` - Navigate lists
- `Alt` - Access menu bar

---

For more information, visit our [GitHub repository](https://github.com/yourusername/WhisperApp) or join our [Discord community](https://discord.gg/whisperapp).