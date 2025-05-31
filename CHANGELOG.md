# Changelog

All notable changes to WhisperApp will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-05-31

### Added
- Initial release of WhisperApp
- Real-time speech-to-text transcription using whisper.cpp
- Support for multiple Whisper models (tiny, base, small, medium, large)
- Global hotkey system for quick access
- System tray integration with minimize to tray
- Multi-language support infrastructure
- Theme system (Light, Dark, High Contrast)
- Audio device selection and management
- Voice Activity Detection (VAD)
- Transcription history tracking
- Export functionality with multiple formats (TXT, SRT, VTT, JSON, HTML, etc.)
- Model download manager with progress tracking
- Customizable hotkey presets
- Accessibility features (screen reader support, keyboard navigation)
- Configuration templates and presets
- Comprehensive error handling with user-friendly messages
- Performance optimizations (threading, caching, lazy loading)
- Unit and integration test suite
- User guide and developer documentation

### Core Features
- **Audio System**: WASAPI integration for Windows audio capture
- **Speech Recognition**: Offline transcription using whisper.cpp
- **User Interface**: Qt 6.5+ based modern UI
- **System Integration**: Windows-specific features (hotkeys, tray, clipboard)
- **Configuration**: JSON-based settings with validation
- **Localization**: Multi-language support with Qt translation system
- **Testing**: Comprehensive test coverage with Google Test

### Security
- Offline operation ensures privacy
- No data transmission to external servers
- Local model storage and processing

### Performance
- GPU acceleration support (CUDA)
- Multi-threaded audio processing
- Efficient memory management
- Model caching for faster switching

### Known Issues
- GPU acceleration requires NVIDIA GPU with CUDA support
- Large models require significant RAM (3GB+ for large model)
- Some hotkey combinations may conflict with system shortcuts

### Future Enhancements
- Additional language support
- Cloud model synchronization (optional)
- Plugin system for extensions
- Mobile companion app
- Batch file processing
- Real-time translation features

## [0.9.0-beta] - 2025-05-15

### Added
- Beta release for testing
- Core transcription functionality
- Basic UI implementation
- Initial model support

## [0.1.0-alpha] - 2025-04-01

### Added
- Initial alpha release
- Proof of concept implementation
- Basic whisper.cpp integration

---

For more information about changes, see the [commit history](https://github.com/yourusername/WhisperApp/commits/main).