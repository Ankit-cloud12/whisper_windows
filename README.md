# WhisperApp - Windows Speech-to-Text Application

A high-performance, production-ready Windows speech-to-text application using whisper.cpp for offline speech recognition.

## Overview

WhisperApp provides real-time speech-to-text transcription on Windows using OpenAI's Whisper model through the whisper.cpp implementation. The application features:

### Core Features
- 🎙️ **Offline speech recognition** - No internet required
- ⌨️ **Global hotkeys** - Quick access from anywhere
- 📌 **System tray integration** - Always accessible
- 🧠 **Multiple Whisper models** - Choose accuracy vs speed
- 📋 **Direct text insertion** - Type into any application
- 🔊 **Audio device selection** - Multiple input sources
- ⚙️ **Highly customizable** - Extensive settings

### New Features (v1.0)
- 🎨 **Theme support** - Light, dark, and high-contrast themes
- 🌍 **Localization** - Multiple language support
- 🎯 **Smart error handling** - User-friendly error messages
- ♿ **Accessibility** - Screen reader support, keyboard navigation
- 📊 **Export templates** - Multiple export formats
- ⚡ **Performance optimizations** - Threading and caching
- 🔧 **Configuration presets** - Quick setup options

## Technology Stack

- **Speech Recognition**: whisper.cpp (C++ implementation of OpenAI Whisper)
- **GUI Framework**: Qt 6.5+
- **Audio Capture**: Windows Audio Session API (WASAPI)
- **Build System**: CMake 3.20+
- **Compiler**: MSVC 2022

## Prerequisites

### Development Environment
- Visual Studio 2022 (Community/Professional) with C++ workload
- Qt 6.5+ with MSVC kit
- CMake 3.20+
- Git
- Windows 10 SDK (19041 or later)

### Hardware Requirements
- Windows 10/11 (x64)
- Minimum 8GB RAM
- 500MB+ disk space for models
- Microphone or audio input device

## Project Structure

```
WhisperApp/
├── src/
│   ├── core/           # Core functionality
│   ├── ui/             # User interface
│   ├── system/         # System integration
│   └── main.cpp        # Application entry
├── resources/
│   ├── icons/          # Application icons
│   ├── sounds/         # Notification sounds
│   ├── styles/         # Theme stylesheets
│   └── resources.qrc   # Resource file
├── translations/       # Localization files
├── config/            # Configuration templates
├── models/            # Whisper model files
├── tests/             # Unit and integration tests
├── third_party/       # External dependencies
├── installer/         # Installation scripts
└── scripts/           # Development scripts
```

## Building from Source

1. Clone the repository with submodules:
   ```bash
   git clone --recursive https://github.com/yourusername/WhisperApp.git
   cd WhisperApp
   ```

2. Initialize third-party dependencies:
   ```bash
   git submodule update --init --recursive
   ```

3. Create build directory:
   ```bash
   mkdir build && cd build
   ```

4. Configure with CMake:
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

5. Build the project:
   ```bash
   cmake --build . --config Release
   ```

## Quick Start

### For Users

1. Download the latest installer from [Releases](https://github.com/yourusername/WhisperApp/releases)
2. Run `WhisperApp-Setup.exe`
3. Launch WhisperApp from Start Menu
4. Download a model (recommended: `base` for most users)
5. Configure hotkeys and start using!

### For Developers

Run the setup script to prepare your development environment:
```bash
scripts\setup-dev-env.bat
```

Generate application icons:
```bash
scripts\generate-icons.bat
```

## Model Management

Download Whisper models using the provided script:
```bash
scripts\download-models.bat
```

Available models:
- **tiny**: Fastest, lowest accuracy (75MB)
- **base**: Good balance (142MB)
- **small**: Better accuracy (466MB)
- **medium**: High accuracy (1.5GB)
- **large**: Best accuracy (3GB)

## Usage

### Basic Usage

1. **Launch WhisperApp** - The app starts in the system tray
2. **Select Model** - Choose based on your needs:
   - `tiny/base` - Fast, good for quick notes
   - `small/medium` - Balanced performance
   - `large` - Best accuracy for professional use
3. **Configure Hotkeys** - Default: `Ctrl+Shift+Space` to toggle recording
4. **Start Recording** - Press hotkey, speak naturally
5. **Stop Recording** - Press hotkey again or wait for auto-stop
6. **Use Transcription** - Text appears automatically or press `Ctrl+Shift+C` to copy

### Advanced Features

#### Hotkey Presets
- **Default** - Standard configuration
- **Minimal** - Essential hotkeys only (F9/F10)
- **Media Keys** - Use play/pause buttons
- **Accessibility** - Single-key shortcuts

#### Export Options
- Plain text, timestamps, SRT/VTT subtitles
- Markdown, JSON, CSV, HTML formats
- Custom templates supported

#### Theme Selection
- **Light** - Clean, modern interface
- **Dark** - Easy on the eyes
- **High Contrast** - Accessibility focused

## Features

### Core Features
- Real-time speech-to-text transcription
- Multiple language support
- Offline operation (no internet required)
- Low latency transcription

### User Interface
- Clean, modern Windows interface
- System tray integration
- Settings dialog for customization
- Model download manager
- Transcription history

### System Integration
- Global hotkeys (customizable)
- Direct text insertion
- Clipboard integration
- Auto-start with Windows

## Configuration

### Settings Location
- User settings: `%APPDATA%\WhisperApp\config.json`
- Default config: `config\default-config.json`
- Hotkey presets: `config\hotkey-presets.json`
- Export templates: `config\export-templates.json`

### Performance Tuning
```json
{
  "performance": {
    "gpuAcceleration": "auto",  // "auto", "on", "off"
    "threadCount": 0,           // 0 = auto-detect
    "modelCaching": true,       // Keep models in memory
    "maxMemoryUsage": 4096      // MB
  }
}
```

## Troubleshooting

### Common Issues

**No audio input detected**
- Check microphone permissions in Windows Settings
- Try selecting a different audio device
- Ensure microphone is not muted

**Model download fails**
- Check internet connection
- Verify firewall settings
- Try manual download from model repository

**Transcription is slow**
- Use a smaller model (tiny/base)
- Enable GPU acceleration if available
- Close other resource-intensive applications

**Hotkeys not working**
- Run as administrator
- Check for conflicts with other applications
- Try alternative hotkey combinations

### Debug Mode
Enable debug logging:
```json
{
  "application": {
    "logLevel": "debug"
  }
}
```
Logs location: `%APPDATA%\WhisperApp\logs\`

## Testing

Run the test suite:
```bash
scripts\run-tests.bat
```

See [TESTING.md](TESTING.md) for detailed testing information.

## Contributing

Please read [DEVELOPMENT.md](DEVELOPMENT.md) for development guidelines and contribution instructions.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [whisper.cpp](https://github.com/ggerganov/whisper.cpp) by Georgi Gerganov
- [OpenAI Whisper](https://github.com/openai/whisper) for the original model
- [Qt Framework](https://www.qt.io/) for the cross-platform UI
- All contributors and testers

## Support

- 📧 Email: support@whisperapp.example.com
- 💬 Discord: [Join our community](https://discord.gg/whisperapp)
- 🐛 Issues: [GitHub Issues](https://github.com/yourusername/WhisperApp/issues)
- 📖 Wiki: [Documentation](https://github.com/yourusername/WhisperApp/wiki)