# WhisperApp Testing Guide

This document provides comprehensive testing procedures for WhisperApp, including automated unit tests, integration tests, and manual testing procedures.

## Table of Contents

1. [Automated Testing](#automated-testing)
2. [Manual Testing Procedures](#manual-testing-procedures)
3. [Performance Testing](#performance-testing)
4. [Edge Cases and Error Scenarios](#edge-cases-and-error-scenarios)
5. [Test Data](#test-data)

## Automated Testing

### Building and Running Tests

```bash
# Configure with tests enabled
cmake -B build -DBUILD_TESTS=ON

# Build tests
cmake --build build --target WhisperAppTests

# Run all tests
cd build
ctest --verbose

# Or run test executable directly
./tests/WhisperAppTests

# Run specific test suite
./tests/WhisperAppTests --gtest_filter=SettingsTest.*

# Run with detailed output
./tests/WhisperAppTests --gtest_print_time=1 --gtest_color=yes
```

### Test Categories

#### Unit Tests
- **SettingsTest**: Configuration management, persistence, validation
- **ModelManagerTest**: Model discovery, validation, paths
- **ErrorCodesTest**: Error handling, codes, messages
- **AudioCaptureTest**: Audio device capture functionality
- **AudioConverterTest**: Audio format conversions
- **AudioUtilsTest**: Audio utility functions
- **DeviceManagerTest**: Audio device enumeration

#### System Tests
- **GlobalHotkeysTest**: Hotkey registration and handling
- **ClipboardManagerTest**: Clipboard operations
- **WindowManagerTest**: Window detection and management

#### Integration Tests
- **RecordingIntegrationTest**: End-to-end recording workflow
- **TranscriptionIntegrationTest**: Complete transcription pipeline

## Manual Testing Procedures

### 1. Installation and Setup

#### Test: First-time Installation
1. Install WhisperApp using the installer
2. **Expected**: Installation completes without errors
3. **Verify**: 
   - Application appears in Start Menu
   - Desktop shortcut created (if selected)
   - Application launches successfully

#### Test: Model Download
1. Launch application for the first time
2. Open Settings → Models
3. Click "Download Models"
4. **Expected**: Model downloader appears
5. Select "base.en" model
6. Click Download
7. **Verify**:
   - Progress bar shows download progress
   - Model installs successfully
   - Model appears in available models list

### 2. Basic Recording Functionality

#### Test: Default Recording
1. Press default hotkey (Ctrl+Shift+R)
2. **Expected**: Recording starts, tray icon changes
3. Speak clearly for 5-10 seconds
4. Press stop hotkey (Ctrl+Shift+S)
5. **Verify**:
   - Recording stops
   - Transcription appears in main window
   - Text is accurate

#### Test: Device Selection
1. Open Settings → Audio
2. Change input device
3. Apply settings
4. Record audio
5. **Verify**: Audio captured from selected device

#### Test: VAD (Voice Activity Detection)
1. Enable VAD in settings
2. Start recording
3. Remain silent for 3 seconds
4. Speak for 5 seconds
5. Remain silent for 3 seconds
6. **Verify**: Only speech segments are transcribed

### 3. Transcription Features

#### Test: Model Selection
1. Download multiple models (tiny, base, small)
2. Select each model in settings
3. Record same audio with each model
4. **Compare**:
   - Transcription accuracy
   - Processing speed
   - Resource usage

#### Test: Language Detection
1. Set language to "auto"
2. Record in English
3. Record in another language (if multilingual model)
4. **Verify**: Correct language detected

#### Test: Translation
1. Enable "Translate to English"
2. Record in non-English language
3. **Verify**: Output is translated to English

### 4. Output Features

#### Test: Clipboard Integration
1. Enable "Auto-copy to clipboard"
2. Complete a transcription
3. **Verify**: Text automatically copied
4. Paste in another application
5. **Verify**: Correct text pasted

#### Test: Auto-type Feature
1. Open a text editor
2. Enable "Auto-type output"
3. Complete a transcription
4. **Verify**: Text automatically typed in editor

#### Test: File Output
1. Enable "Save transcriptions"
2. Set output directory
3. Complete several transcriptions
4. **Verify**:
   - Files created in specified directory
   - Correct format (txt/srt)
   - Timestamps included (if enabled)

### 5. UI Features

#### Test: System Tray
1. Minimize to system tray
2. **Verify**: Application minimizes to tray
3. Right-click tray icon
4. **Verify**: Context menu appears with options
5. Double-click tray icon
6. **Verify**: Main window restored

#### Test: Transcription History
1. Complete multiple transcriptions
2. **Verify**: All appear in history widget
3. Click on a history item
4. **Verify**: Full text displayed
5. Right-click for context menu
6. **Verify**: Copy/delete options work

#### Test: Settings Dialog
1. Open each settings tab
2. Modify various settings
3. Click Apply
4. Close and reopen settings
5. **Verify**: All changes persisted

### 6. Hotkey Features

#### Test: Custom Hotkeys
1. Open Settings → Hotkeys
2. Change each hotkey
3. Apply settings
4. Test each new hotkey
5. **Verify**: All hotkeys work correctly

#### Test: Hotkey Conflicts
1. Try to set same hotkey for multiple actions
2. **Verify**: Warning or prevention of conflict
3. Set system-wide used hotkey (e.g., Ctrl+C)
4. **Verify**: Appropriate handling

### 7. Window Integration

#### Test: Active Window Detection
1. Open a text editor
2. Start recording with focus on editor
3. Complete transcription
4. **Verify**: Text inserted at cursor position

#### Test: Window Switching
1. Start recording
2. Switch to different application
3. Complete recording
4. **Verify**: Transcription completes normally

## Performance Testing

### Test: Long Recording
1. Set max recording duration to 5 minutes
2. Record continuously for 5 minutes
3. **Monitor**:
   - Memory usage
   - CPU usage
   - Disk usage
4. **Verify**:
   - Recording stops at limit
   - Transcription completes
   - No memory leaks

### Test: Rapid Recording
1. Make 10 quick recordings (5 seconds each)
2. **Verify**:
   - All recordings processed
   - No crashes or hangs
   - UI remains responsive

### Test: Large Model Performance
1. Download and select large model
2. Record 1-minute audio
3. **Measure**:
   - Transcription time
   - Memory usage
   - Accuracy

## Edge Cases and Error Scenarios

### Test: No Microphone
1. Disable all audio input devices
2. Try to start recording
3. **Expected**: Clear error message
4. **Verify**: Application doesn't crash

### Test: Model Missing
1. Delete model file manually
2. Try to transcribe
3. **Expected**: Error message about missing model
4. **Verify**: Prompt to download model

### Test: Disk Full
1. Fill disk to near capacity
2. Try to save transcription
3. **Expected**: Appropriate error message
4. **Verify**: No data corruption

### Test: Network Issues
1. Disconnect network
2. Try to download model
3. **Expected**: Network error message
4. Reconnect and retry
5. **Verify**: Download resumes or restarts

### Test: Invalid Audio Format
1. Select incompatible audio settings
2. Try to record
3. **Expected**: Format error or automatic adjustment
4. **Verify**: Fallback to supported format

### Test: Simultaneous Operations
1. Start recording
2. Try to start another recording
3. **Expected**: Second recording blocked or queued
4. Open settings while recording
5. **Verify**: Settings accessible but relevant options disabled

## Test Data

### Audio Samples
Location: `tests/data/audio/`

1. **silence.wav** - 10 seconds of silence
2. **speech_english.wav** - Clear English speech
3. **speech_noisy.wav** - Speech with background noise
4. **music.wav** - Music without speech
5. **mixed_content.wav** - Speech with music background

### Expected Outputs
Location: `tests/data/expected/`

1. **speech_english.txt** - Expected transcription
2. **speech_english.srt** - Expected SRT format
3. **speech_translated.txt** - Expected translation

### Test Configurations
Location: `tests/data/configs/`

1. **minimal.json** - Minimal valid configuration
2. **full.json** - All options configured
3. **invalid.json** - Invalid configuration for error testing

## Debugging Failed Tests

### Enable Debug Logging
```bash
# Set environment variable
export WHISPER_LOG_LEVEL=DEBUG

# Run tests with debug output
./tests/WhisperAppTests --gtest_catch_exceptions=0
```

### Common Issues

1. **Audio Tests Fail**: Check audio permissions and devices
2. **Model Tests Fail**: Ensure test models are present
3. **Hotkey Tests Fail**: May need elevated permissions
4. **Clipboard Tests Fail**: Check clipboard access permissions

## Continuous Integration

### GitHub Actions Workflow
```yaml
name: Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v2
      - name: Configure
        run: cmake -B build -DBUILD_TESTS=ON
      - name: Build
        run: cmake --build build
      - name: Test
        run: cd build && ctest --verbose
```

## Test Coverage

### Generate Coverage Report
```bash
# Build with coverage flags
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON

# Run tests
cd build && ctest

# Generate report
gcov src/core/*.cpp
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage-report
```

### Coverage Goals
- Core functionality: >90%
- Audio processing: >85%
- UI components: >70%
- Error handling: >95%

## Regression Testing

### Before Each Release
1. Run full automated test suite
2. Complete manual test checklist
3. Test on multiple Windows versions (10, 11)
4. Test with different audio hardware
5. Verify installer/uninstaller
6. Check for memory leaks with tools like Valgrind

### Test Report Template
```
WhisperApp Test Report
Version: X.Y.Z
Date: YYYY-MM-DD
Tester: Name

Automated Tests:
- Total: XXX
- Passed: XXX
- Failed: X
- Skipped: X

Manual Tests:
- [ ] Installation
- [ ] Basic Recording
- [ ] Transcription
- [ ] Settings
- [ ] Hotkeys
- [ ] Performance

Issues Found:
1. Issue description...

Recommendations:
- ...