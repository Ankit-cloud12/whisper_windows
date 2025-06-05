cmake_minimum_required(VERSION 3.12)
project(whisper)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(WHISPER_BUILD_TESTS "Build tests" OFF)
option(WHISPER_BUILD_EXAMPLES "Build examples" OFF)
option(WHISPER_CUBLAS "Enable CUDA BLAS support" OFF)
option(WHISPER_OPENBLAS "Enable OpenBLAS support" OFF)
option(WHISPER_ACCELERATE "Enable Apple Accelerate framework" OFF)

# Compiler-specific settings
if(MSVC)
    add_compile_options(/W4)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wcast-qual -Wno-unused-function)
endif()

# CUDA support
if(WHISPER_CUBLAS)
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)
    add_compile_definitions(GGML_USE_CUBLAS)
    if(MSVC)
        add_compile_options(/arch:AVX2)
    else()
        add_compile_options(-mavx -mavx2 -mfma -mf16c)
    endif()
endif()

# Source files
set(WHISPER_SOURCES
    whisper.cpp
    ggml.c
    ggml-alloc.c
    ggml-backend.c
    ggml-quants.c
)

# Header files
set(WHISPER_HEADERS
    whisper.h
    ggml.h
    ggml-alloc.h
    ggml-backend.h
    ggml-impl.h
    ggml-quants.h
)

# CUDA sources
if(WHISPER_CUBLAS)
    list(APPEND WHISPER_SOURCES ggml-cuda.cu)
    list(APPEND WHISPER_HEADERS ggml-cuda.h)
endif()

# Create the whisper library
add_library(whisper STATIC ${WHISPER_SOURCES} ${WHISPER_HEADERS})

# Include directories
target_include_directories(whisper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries
if(WHISPER_CUBLAS)
    target_link_libraries(whisper PRIVATE CUDA::cudart CUDA::cublas)
endif()

if(WHISPER_OPENBLAS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(OPENBLAS REQUIRED openblas)
    target_link_libraries(whisper PRIVATE ${OPENBLAS_LIBRARIES})
    target_include_directories(whisper PRIVATE ${OPENBLAS_INCLUDE_DIRS})
    target_compile_definitions(whisper PRIVATE GGML_USE_OPENBLAS)
endif()

if(WHISPER_ACCELERATE)
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    if(ACCELERATE_FRAMEWORK)
        target_link_libraries(whisper PRIVATE ${ACCELERATE_FRAMEWORK})
        target_compile_definitions(whisper PRIVATE GGML_USE_ACCELERATE)
    endif()
endif()

# Platform-specific settings
if(WIN32)
    target_compile_definitions(whisper PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(whisper PRIVATE ws2_32)
endif()

# Threading
find_package(Threads REQUIRED)
target_link_libraries(whisper PRIVATE Threads::Threads)

# Math library on Unix
if(UNIX AND NOT APPLE)
    target_link_libraries(whisper PRIVATE m)
endif()

# Install targets
install(TARGETS whisper
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${WHISPER_HEADERS}
    DESTINATION include
)