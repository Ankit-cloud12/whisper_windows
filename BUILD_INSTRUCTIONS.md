# WhisperApp Build Instructions

This document provides step-by-step instructions for building WhisperApp on Windows.

## Prerequisites

### Required Software

1. **Visual Studio 2019 or 2022** (Community Edition is sufficient)
   - Install with "Desktop development with C++" workload
   - Ensure MSVC v143 toolset is installed

2. **CMake 3.20 or later**
   - Download from: https://cmake.org/download/
   - Add to system PATH during installation

3. **Qt 6.7.0 or later**
   - Download from: https://www.qt.io/download-qt-installer
   - Install the following components:
     - Qt 6.7.0 → MSVC 2019 64-bit
     - Qt 6.7.0 → Sources (optional, for debugging)
     - Developer and Designer Tools → CMake
     - Developer and Designer Tools → Ninja

### Optional Dependencies

4. **Whisper.cpp** (for transcription functionality)
   - Will be automatically downloaded as a git submodule
   - Or manually download from: https://github.com/ggerganov/whisper.cpp

5. **NSIS** (for creating installer)
   - Download from: https://nsis.sourceforge.io/
   - Add to system PATH

## Quick Start

### Option 1: Automated Setup (Recommended)

1. Open a **Visual Studio Developer Command Prompt**
2. Navigate to the project directory
3. Run the setup script:
   ```cmd
   cd scripts
   setup-dev-env.bat
   ```

### Option 2: Manual Setup

1. **Clone the repository:**
   ```cmd
   git clone <repository-url>
   cd WhisperApp
   ```

2. **Initialize submodules (optional):**
   ```cmd
   git submodule update --init --recursive
   ```

3. **Set environment variables:**
   ```cmd
   set QT_DIR=C:\Qt\6.7.0\msvc2019_64
   set PATH=%QT_DIR%\bin;%PATH%
   ```

## Building the Application

### Test Build (Debug)

To verify everything is set up correctly:

```cmd
cd scripts
test-build.bat
```

This will:
- Check for required tools
- Configure the project with CMake
- Build a debug version
- Verify the executable was created

### Release Build

For a production-ready build:

```cmd
cd scripts
build-release.bat
```

This will:
- Build the release version
- Deploy Qt dependencies
- Create a distributable package
- Generate checksums

### Manual Build

If you prefer to build manually:

```cmd
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH=C:\Qt\6.7.0\msvc2019_64 ^
    ..

# Build
cmake --build . --config Release --parallel
```

## Build Configuration Options

### CMake Options

- `BUILD_TESTS=ON/OFF` - Build unit tests (default: OFF)
- `BUILD_INSTALLER=ON/OFF` - Build installer package (default: OFF)
- `CMAKE_BUILD_TYPE` - Debug/Release/RelWithDebInfo/MinSizeRel

### Example with all options:

```cmd
cmake -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH=C:\Qt\6.7.0\msvc2019_64 ^
    -DBUILD_TESTS=ON ^
    -DBUILD_INSTALLER=ON ^
    ..
```

## Troubleshooting

### Common Issues

1. **Qt not found**
   - Verify Qt installation path
   - Update `CMAKE_PREFIX_PATH` to point to your Qt installation
   - Ensure Qt bin directory is in PATH

2. **Compiler not found**
   - Run from Visual Studio Developer Command Prompt
   - Verify Visual Studio C++ tools are installed

3. **CMake configuration fails**
   - Check CMake version (must be 3.20+)
   - Verify all paths are correct
   - Check for typos in environment variables

4. **Build fails with linker errors**
   - Ensure all dependencies are properly linked
   - Check that Qt libraries are found
   - Verify Windows SDK is installed

5. **Resource compilation fails**
   - Check that all files referenced in resources.qrc exist
   - Verify file paths are correct

### Getting Help

If you encounter issues:

1. Check the error messages carefully
2. Verify all prerequisites are installed
3. Try the test build first
4. Check the GitHub issues page
5. Run with verbose output: `cmake --build . --config Release --verbose`

## Project Structure

```
WhisperApp/
├── src/                    # Source code
│   ├── core/              # Core functionality
│   ├── ui/                # User interface
│   └── system/            # System integration
├── resources/             # Icons, sounds, styles
├── config/                # Configuration files
├── scripts/               # Build and setup scripts
├── tests/                 # Unit tests
├── third_party/           # External dependencies
└── CMakeLists.txt         # Main build configuration
```

## Next Steps

After a successful build:

1. **Test the application** - Run the executable to verify basic functionality
2. **Add Whisper.cpp** - For full transcription capabilities
3. **Run tests** - Build with `BUILD_TESTS=ON` and run the test suite
4. **Create installer** - Use `build-release.bat` for distribution

For development, see `DEVELOPMENT.md` for coding guidelines and contribution instructions.
