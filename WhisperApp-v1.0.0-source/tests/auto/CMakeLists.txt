cmake_minimum_required(VERSION 3.20)
project(WhisperAppAutoTests)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Core Test Widgets Gui) # Add Gui for QWindow related tests if needed by MainWindow

# Include directories
# Assumes this CMakeLists.txt is in tests/auto, so ../../src points to WhisperApp-v1.0.0-source/src
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src)

# Test sources
set(TEST_SOURCES
    main_auto_tests.cpp
    test_settings.cpp
    test_mainwindow_logic.cpp
    # .h files are usually not added to add_executable directly unless specific moc processing needed
)

add_executable(whisper_auto_tests ${TEST_SOURCES})

# Link Qt libraries and application libraries/objects
target_link_libraries(whisper_auto_tests
    PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Widgets
    Qt6::Gui
    WhisperApp # Link against the main application target
)

# Add test to CTest
include(CTest)
add_test(NAME QtAutoTests COMMAND whisper_auto_tests)
