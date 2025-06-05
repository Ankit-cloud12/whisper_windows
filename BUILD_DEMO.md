# Building WhisperApp Demo Version

This document provides instructions for building the WhisperApp demo version without the whisper.cpp dependency.

## Overview

The demo version of WhisperApp provides:
- ✅ Full Qt6-based user interface
- ✅ System tray integration
- ✅ Settings and configuration
- ✅ Audio device management
- ✅ Hotkey system
- ✅ Theme support
- ❌ Actual speech recognition (uses stub implementation)

## Quick Build Instructions

### Prerequisites

1. **Qt 6.5+** - Download from https://www.qt.io/download
2. **CMake 3.20+** - Download from https://cmake.org/download/
3. **Visual Studio 2022** - With C++ desktop development workload

### Building

1. Clone the repository:
   ```cmd
   git clone https://github.com/Ankit-cloud12/whisper_windows.git
   cd whisper_windows\WhisperApp
   ```

2. Create build directory:
   ```cmd
   mkdir build
   cd build
   ```

3. Configure with CMake:
   ```cmd
   cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.7.0/msvc2019_64"
   ```
   (Adjust the Qt path to match your installation)

4. Build:
   ```cmd
   cmake --build . --config Release
   ```

5. Run the demo:
   ```cmd
   Release\WhisperApp.exe
   ```

## What Works in Demo Mode

- **UI Navigation**: All windows, dialogs, and menus are fully functional
- **Settings**: Can be changed and saved (though transcription settings won't affect anything)
- **Audio Devices**: Can be selected and monitored
- **Hotkeys**: Can be configured (though they won't trigger actual transcription)
- **Themes**: All themes work correctly
- **Export**: UI for export works, but there's no actual transcription data to export

## What Doesn't Work

- **Speech Recognition**: No actual transcription occurs
- **Model Download**: Models can't be used even if downloaded
- **Transcription History**: Will remain empty

## Converting to Full Version

To enable full functionality:

1. Clone whisper.cpp:
   ```cmd
   cd third_party
   git clone https://github.com/ggerganov/whisper.cpp.git
   cd whisper.cpp
   git checkout <stable-version>
   ```

2. Build whisper.cpp according to its instructions

3. Rebuild WhisperApp - CMake will automatically detect whisper.cpp and enable full functionality

## Troubleshooting

### Qt Not Found
- Ensure Qt is installed and CMAKE_PREFIX_PATH points to the correct Qt installation
- Example: `-DCMAKE_PREFIX_PATH="C:/Qt/6.7.0/msvc2019_64"`

### Build Errors
- Make sure you have Visual Studio 2022 with C++ desktop development workload
- Ensure all submodules are initialized: `git submodule update --init --recursive`

### Missing DLLs at Runtime
- Copy Qt DLLs to the output directory or use `windeployqt`:
  ```cmd
  C:\Qt\6.7.0\msvc2019_64\bin\windeployqt.exe Release\WhisperApp.exe
  ```

## GitHub Actions Status

The repository includes GitHub Actions workflows for:
- **Build Testing**: Validates that the code compiles correctly
- **Release Creation**: Automatically creates releases when tags are pushed

Current build status: Check the [Actions tab](https://github.com/Ankit-cloud12/whisper_windows/actions) on GitHub.

## Contributing

If you'd like to contribute to making the full version work:
1. Fork the repository
2. Add whisper.cpp integration
3. Test thoroughly
4. Submit a pull request

## Questions?

- Check existing [Issues](https://github.com/Ankit-cloud12/whisper_windows/issues)
- Create a new issue if you encounter problems
- Note in your issue that you're using the demo version