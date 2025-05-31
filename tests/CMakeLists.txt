# CMakeLists.txt for WhisperApp Tests
#
# Unit test configuration using Google Test framework

cmake_minimum_required(VERSION 3.16)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/system
    ${CMAKE_SOURCE_DIR}/src/ui
    ${GTEST_INCLUDE_DIRS}
)

# Test sources
set(TEST_SOURCES
    # Core tests
    core/SettingsTest.cpp
    core/ModelManagerTest.cpp
    core/ErrorCodesTest.cpp
    core/WhisperEngineTest.cpp
    core/AudioConverterTest.cpp
    core/AudioCaptureTest.cpp
    core/AudioUtilsTest.cpp
    core/DeviceManagerTest.cpp
    
    # System tests
    system/GlobalHotkeysTest.cpp
    system/ClipboardManagerTest.cpp
    system/WindowManagerTest.cpp
    
    # Integration tests
    integration/RecordingIntegrationTest.cpp
    integration/TranscriptionIntegrationTest.cpp
)

# Test utilities
set(TEST_UTILS
    TestUtils.h
)

# Create test executable
add_executable(WhisperAppTests
    ${TEST_SOURCES}
    ${TEST_UTILS}
)

# Link with Google Test and project libraries
target_link_libraries(WhisperAppTests
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread  # Required for Google Test on some platforms
    # Link with project libraries when available
    # WhisperEngine
    # AudioConverter
    # etc.
)

# Add compile definitions for testing
target_compile_definitions(WhisperAppTests PRIVATE
    TESTING_ENABLED
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

# Add tests to CTest
add_test(NAME WhisperAppTests COMMAND WhisperAppTests)

# Custom target to run tests with verbose output
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS WhisperAppTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Copy test data files if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Generate test coverage report (optional)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_COMPILER_IS_GNUCXX)
    include(CodeCoverage)
    setup_target_for_coverage(
        NAME coverage
        EXECUTABLE WhisperAppTests
        DEPENDENCIES WhisperAppTests
    )
endif()