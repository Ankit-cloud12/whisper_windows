# Development Guidelines

This document provides guidelines for developers working on the WhisperApp project.

## Development Environment Setup

### Required Tools

1. **Visual Studio 2022**
   - Install with "Desktop development with C++" workload
   - Include Windows 10 SDK (19041 or later)
   - Include CMake tools for Windows

2. **Qt Framework 6.5+**
   - Download from https://www.qt.io/download
   - Install Qt Creator IDE
   - Configure MSVC 2022 kit

3. **Additional Tools**
   - Git for Windows
   - CMake 3.20+ (standalone)
   - Python 3.8+ (for scripts)

### First-Time Setup

1. Run the development environment setup script:
   ```cmd
   scripts\setup-dev-env.bat
   ```

2. Configure Git:
   ```cmd
   git config core.autocrlf true
   git config user.name "Your Name"
   git config user.email "your.email@example.com"
   ```

## Code Style Guidelines

### C++ Style

- Use modern C++17 features
- Follow the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- Use `snake_case` for variables and functions
- Use `PascalCase` for classes and structs
- Use `UPPER_CASE` for constants and macros
- Prefer `const` and `constexpr` where applicable
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) instead of raw pointers

### Code Format

```cpp
// Class definition example
class WhisperEngine {
public:
    WhisperEngine();
    ~WhisperEngine();
    
    bool loadModel(const std::string& model_path);
    std::string transcribeAudio(const std::vector<float>& audio_data);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
```

### File Organization

- Header files: `.h` extension
- Implementation files: `.cpp` extension
- One class per file (with matching names)
- Use include guards or `#pragma once`
- Group includes: std headers, Qt headers, project headers

## Build Instructions

### Debug Build

```cmd
mkdir build-debug
cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

### Release Build

```cmd
mkdir build-release
cd build-release
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Running Tests

```cmd
cd build-debug
ctest -C Debug --output-on-failure
```

## Architecture Overview

### Core Components

1. **WhisperEngine**
   - Manages whisper.cpp integration
   - Handles model loading and transcription
   - Thread-safe operations
   - Performance optimizations with caching

2. **AudioCapture**
   - WASAPI integration for Windows audio
   - Real-time audio capture and buffering
   - Device enumeration and selection
   - Voice activity detection (VAD)

3. **ModelManager**
   - Downloads and manages Whisper models
   - Verifies model integrity
   - Handles model updates
   - Lazy loading for memory efficiency

4. **Settings**
   - Application configuration management
   - User preferences persistence
   - Default values and validation
   - JSON-based configuration system

5. **ErrorCodes**
   - Comprehensive error handling system
   - User-friendly error messages
   - Error recovery suggestions
   - Logging integration

6. **Localization**
   - Multi-language support infrastructure
   - Translation management
   - Dynamic language switching
   - Qt Linguist integration

### UI Components

1. **MainWindow**
   - Primary application interface
   - Coordinates UI components
   - Handles user interactions
   - Tooltips on all controls

2. **SettingsDialog**
   - Configuration interface
   - Hotkey setup
   - Model selection
   - Theme switching

3. **TrayIcon**
   - System tray integration
   - Quick access menu
   - Status notifications
   - Minimize to tray support

4. **ModelDownloader**
   - Model download progress UI
   - Download queue management
   - Error handling display
   - Resume capability

5. **TranscriptionWidget**
   - Real-time text display
   - Confidence highlighting
   - Export functionality
   - Find/Replace support

6. **AudioLevelWidget**
   - Visual audio feedback
   - Level monitoring
   - Peak detection
   - Smooth animations

### System Integration

1. **GlobalHotkeys**
   - Windows hotkey registration
   - Hotkey event handling
   - Conflict detection

2. **ClipboardManager**
   - Text insertion functionality
   - Clipboard operations
   - Format handling

3. **WindowManager**
   - Window state management
   - Focus handling
   - Multi-monitor support

## Polish Features

### Theme System

1. **Stylesheets**
   - `default.qss`: Light theme with animations
   - `dark.qss`: Dark theme for reduced eye strain
   - `light.qss`: Clean material design
   - CSS transitions for smooth effects

2. **Accessibility**
   - High contrast mode
   - Screen reader support
   - Keyboard navigation
   - Font size adjustments

### Configuration System

1. **Default Configuration**
   - `config/default-config.json`: Application defaults
   - Comprehensive settings structure
   - Performance tuning options

2. **Preset System**
   - `config/hotkey-presets.json`: Hotkey configurations
   - Multiple preset options
   - User-friendly names

3. **Export Templates**
   - `config/export-templates.json`: Export formats
   - Multiple output formats
   - Customizable templates

### Resource Management

1. **Icons**
   - Application icons in multiple sizes
   - Action icons for UI elements
   - Status indicators
   - Flag icons for languages

2. **Sounds**
   - Notification sounds
   - Recording start/stop
   - Error alerts
   - Success confirmations

### Localization

1. **Translation Files**
   - Qt `.ts` format
   - Multiple language support
   - Context-aware translations
   - Plural form handling

2. **String Management**
   - Centralized string IDs
   - Consistent terminology
   - Easy maintenance

## Testing Strategy

### Unit Tests

- Test individual components in isolation
- Use Google Test framework
- Aim for >80% code coverage
- Mock external dependencies

### Integration Tests

- Test component interactions
- Verify API contracts
- Test error scenarios
- Performance benchmarks

### Manual Testing

- UI responsiveness
- Hotkey functionality
- Model switching
- Error handling
- Accessibility testing

## Debugging

### Debug Output

Enable debug logging:
```cpp
#ifdef _DEBUG
    qDebug() << "Debug message";
#endif
```

### Performance Profiling

- Use Visual Studio Performance Profiler
- Monitor CPU and memory usage
- Identify bottlenecks
- Optimize critical paths

### Memory Profiling

- Monitor memory usage patterns
- Check for memory leaks
- Validate model caching
- Test resource cleanup

## Git Workflow

### Branch Structure

- `main`: Stable release branch
- `develop`: Development branch
- `feature/*`: Feature branches
- `bugfix/*`: Bug fix branches
- `release/*`: Release preparation

### Commit Messages

Follow conventional commits:
```
type(scope): subject

body (optional)

footer (optional)
```

Types: feat, fix, docs, style, refactor, test, chore

### Pull Request Process

1. Create feature branch from `develop`
2. Make changes and commit
3. Push branch and create PR
4. Code review and testing
5. Merge to `develop`

## Dependencies

### Third-Party Libraries

- **whisper.cpp**: Speech recognition engine
- **Qt 6.5+**: UI framework
- **Google Test**: Unit testing (optional)

### Managing Dependencies

- Use Git submodules for whisper.cpp
- Qt managed through installer
- Other dependencies via vcpkg (optional)

## Release Process

1. **Version Bump**
   - Update version in CMakeLists.txt
   - Update README.md
   - Create changelog entry

2. **Testing**
   - Run full test suite
   - Manual testing checklist
   - Performance benchmarks

3. **Build Release**
   - Clean build in Release mode
   - Code signing (if available)
   - Create installer

4. **Distribution**
   - GitHub release
   - Update website
   - Notify users

## Troubleshooting

### Common Issues

1. **Build Failures**
   - Check Visual Studio installation
   - Verify Qt configuration
   - Clean and rebuild

2. **Runtime Errors**
   - Check Windows Audio service
   - Verify model files
   - Check permissions

3. **Performance Issues**
   - Profile CPU usage
   - Check memory leaks
   - Optimize model selection

### Development Tips

1. **Performance Optimization**
   - Use threading for long operations
   - Implement progress indicators
   - Cache frequently used data
   - Profile before optimizing

2. **Error Handling**
   - Use ErrorCodes system
   - Provide user-friendly messages
   - Log technical details
   - Suggest recovery actions

3. **UI Development**
   - Follow Qt best practices
   - Test with different DPI settings
   - Ensure keyboard accessibility
   - Add tooltips to controls

4. **Testing**
   - Write unit tests for new features
   - Test error scenarios
   - Verify memory cleanup
   - Check thread safety

## Resources

- [whisper.cpp Documentation](https://github.com/ggerganov/whisper.cpp)
- [Qt Documentation](https://doc.qt.io/)
- [Windows Audio APIs](https://docs.microsoft.com/en-us/windows/win32/coreaudio/core-audio-apis-in-windows-vista)
- [CMake Documentation](https://cmake.org/documentation/)
- [Qt Style Sheets](https://doc.qt.io/qt-6/stylesheet.html)
- [Qt Linguist Manual](https://doc.qt.io/qt-6/qtlinguist-index.html)

## Contact

For questions or support:
- Create an issue on GitHub
- Join our Discord server
- Email: support@whisperapp.com