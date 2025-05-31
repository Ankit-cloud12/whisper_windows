cmake_minimum_required(VERSION 3.20)
project(WhisperApp VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Options
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_INSTALLER "Build installer package" OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Network)
find_package(Threads REQUIRED)

# Qt automoc and autouic
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add whisper.cpp as subdirectory (will be added when submodule is initialized)
# add_subdirectory(third_party/whisper.cpp)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/system
)

# Source files
set(SOURCES
    src/main.cpp
    src/core/WhisperEngine.cpp
    src/core/AudioCapture.cpp
    src/core/AudioConverter.cpp
    src/core/AudioUtils.cpp
    src/core/DeviceManager.cpp
    src/core/Logger.cpp
    src/core/ModelManager.cpp
    src/core/Settings.cpp
    src/ui/AboutDialog.cpp
    src/ui/AudioLevelWidget.cpp
    src/ui/HotkeyEditWidget.cpp
    src/ui/MainWindow.cpp
    src/ui/ModelDownloader.cpp
    src/ui/SettingsDialog.cpp
    src/ui/StatusBarWidget.cpp
    src/ui/TranscriptionHistoryWidget.cpp
    src/ui/TranscriptionWidget.cpp
    src/ui/TrayIcon.cpp
    src/ui/UIUtils.cpp
    src/system/GlobalHotkeys.cpp
    src/system/ClipboardManager.cpp
    src/system/WindowManager.cpp
)

# Header files
set(HEADERS
    src/core/WhisperEngine.h
    src/core/AudioCapture.h
    src/core/AudioConverter.h
    src/core/AudioUtils.h
    src/core/DeviceManager.h
    src/core/Logger.h
    src/core/ErrorCodes.h
    src/core/ModelManager.h
    src/core/Settings.h
    src/ui/AboutDialog.h
    src/ui/AudioLevelWidget.h
    src/ui/HotkeyEditWidget.h
    src/ui/MainWindow.h
    src/ui/ModelDownloader.h
    src/ui/SettingsDialog.h
    src/ui/StatusBarWidget.h
    src/ui/TranscriptionHistoryWidget.h
    src/ui/TranscriptionWidget.h
    src/ui/TrayIcon.h
    src/ui/UIUtils.h
    src/system/GlobalHotkeys.h
    src/system/ClipboardManager.h
    src/system/WindowManager.h
)

# Resource files
set(RESOURCES
    resources/resources.qrc
)

# Windows-specific settings
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE TRUE)
    
    # Windows libraries
    set(PLATFORM_LIBS
        winmm
        ole32
        user32
        advapi32
        shell32
        comdlg32
        uuid
        oleaut32
    )
    
    # Add Windows manifest for DPI awareness
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
        
        # Add manifest file if it exists
        # set(SOURCES ${SOURCES} resources/WhisperApp.manifest)
    endif()
endif()

# Create executable
add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    OUTPUT_NAME "WhisperApp"
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
    Threads::Threads
    ${PLATFORM_LIBS}
    # whisper  # Will be linked when whisper.cpp is added
)

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# Install targets
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install models directory structure
install(DIRECTORY ${CMAKE_SOURCE_DIR}/models/
    DESTINATION models
    FILES_MATCHING PATTERN ".gitkeep"
)

# Add tests if enabled
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# CPack configuration for creating installers
set(CPACK_PACKAGE_NAME "WhisperApp")
set(CPACK_PACKAGE_VENDOR "WhisperApp Development Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Windows Speech-to-Text Application")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "WhisperApp")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "WhisperApp")
    set(CPACK_NSIS_PACKAGE_NAME "WhisperApp")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/icons/WhisperApp.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/resources/icons/WhisperApp.ico")
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$DESKTOP\\\\WhisperApp.lnk' '$INSTDIR\\\\bin\\\\WhisperApp.exe'"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$DESKTOP\\\\WhisperApp.lnk'"
    )
endif()

include(CPack)

# Print configuration summary
message(STATUS "========================================")
message(STATUS "WhisperApp Configuration Summary:")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt Version: ${Qt6_VERSION}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")