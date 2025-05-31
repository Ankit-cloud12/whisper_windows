@echo off
REM Run WhisperApp tests with various options

echo ========================================
echo WhisperApp Test Runner
echo ========================================
echo.

set BUILD_DIR=%~dp0..\..\build

if not exist "%BUILD_DIR%\tests\WhisperAppTests.exe" (
    echo Error: Test executable not found!
    echo Please build the project with -DBUILD_TESTS=ON
    exit /b 1
)

if "%1"=="" goto :run_all

if "%1"=="all" goto :run_all
if "%1"=="unit" goto :run_unit
if "%1"=="integration" goto :run_integration
if "%1"=="system" goto :run_system
if "%1"=="coverage" goto :run_coverage
if "%1"=="help" goto :show_help

echo Invalid option: %1
goto :show_help

:run_all
echo Running all tests...
echo.
"%BUILD_DIR%\tests\WhisperAppTests.exe" --gtest_color=yes
goto :end

:run_unit
echo Running unit tests...
echo.
"%BUILD_DIR%\tests\WhisperAppTests.exe" --gtest_filter="*Test.*" --gtest_color=yes
goto :end

:run_integration
echo Running integration tests...
echo.
"%BUILD_DIR%\tests\WhisperAppTests.exe" --gtest_filter="*IntegrationTest.*" --gtest_color=yes
goto :end

:run_system
echo Running system tests...
echo.
"%BUILD_DIR%\tests\WhisperAppTests.exe" --gtest_filter="*SystemTest.*" --gtest_color=yes
goto :end

:run_coverage
echo Generating test coverage report...
echo.
if not exist "%BUILD_DIR%\coverage" mkdir "%BUILD_DIR%\coverage"

REM Run tests with coverage
"%BUILD_DIR%\tests\WhisperAppTests.exe" --gtest_output=xml:"%BUILD_DIR%\coverage\test_results.xml"

REM Generate coverage report (requires OpenCppCoverage)
if exist "C:\Program Files\OpenCppCoverage\OpenCppCoverage.exe" (
    "C:\Program Files\OpenCppCoverage\OpenCppCoverage.exe" ^
        --sources "%~dp0..\..\src" ^
        --export_type=html:"%BUILD_DIR%\coverage" ^
        -- "%BUILD_DIR%\tests\WhisperAppTests.exe"
    
    echo Coverage report generated in: %BUILD_DIR%\coverage
    start "" "%BUILD_DIR%\coverage\index.html"
) else (
    echo OpenCppCoverage not found. Please install it for coverage reports.
)
goto :end

:show_help
echo Usage: run-tests.bat [option]
echo.
echo Options:
echo   all         - Run all tests (default)
echo   unit        - Run unit tests only
echo   integration - Run integration tests only
echo   system      - Run system tests only
echo   coverage    - Run tests and generate coverage report
echo   help        - Show this help message
echo.
echo Examples:
echo   run-tests.bat
echo   run-tests.bat unit
echo   run-tests.bat coverage
goto :end

:end
echo.
echo ========================================
echo Test run completed
echo ========================================