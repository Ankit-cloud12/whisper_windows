/*
 * WindowManagerTest.cpp
 * 
 * Unit tests for WindowManager class
 */

#include <gtest/gtest.h>
#include "system/WindowManager.h"
#include "core/ErrorCodes.h"
#include "../TestUtils.h"
#include <thread>
#include <chrono>

class WindowManagerTest : public ::testing::Test {
protected:
    WindowManager* windowManager;
    
    void SetUp() override {
        windowManager = &WindowManager::getInstance();
    }
    
    void TearDown() override {
        // Nothing to tear down for singleton
    }
};

// Test singleton instance
TEST_F(WindowManagerTest, SingletonInstance) {
    auto& instance1 = WindowManager::getInstance();
    auto& instance2 = WindowManager::getInstance();
    
    // Should be the same instance
    EXPECT_EQ(&instance1, &instance2);
}

// Test getting foreground window
TEST_F(WindowManagerTest, GetForegroundWindow) {
    auto window = windowManager->getForegroundWindow();
    
    // Should return some window info (unless running headless)
    if (window.has_value()) {
        EXPECT_FALSE(window->title.empty() || window->className.empty());
        EXPECT_NE(window->handle, nullptr);
        EXPECT_GT(window->processId, 0);
    }
}

// Test window enumeration
TEST_F(WindowManagerTest, EnumerateWindows) {
    auto windows = windowManager->getAllWindows();
    
    // Should find at least some windows
    EXPECT_FALSE(windows.empty());
    
    // Verify window info
    for (const auto& window : windows) {
        EXPECT_NE(window.handle, nullptr);
        EXPECT_GT(window.processId, 0);
        // Title might be empty for some windows
    }
}

// Test finding window by title
TEST_F(WindowManagerTest, FindWindowByTitle) {
    // First get some existing windows
    auto windows = windowManager->getAllWindows();
    if (windows.empty()) {
        GTEST_SKIP() << "No windows available for testing";
    }
    
    // Find a window with non-empty title
    std::string testTitle;
    for (const auto& window : windows) {
        if (!window.title.empty()) {
            testTitle = window.title;
            break;
        }
    }
    
    if (testTitle.empty()) {
        GTEST_SKIP() << "No windows with titles found";
    }
    
    // Try to find the window
    auto found = windowManager->findWindowByTitle(testTitle);
    EXPECT_TRUE(found.has_value());
    EXPECT_EQ(found->title, testTitle);
    
    // Try partial match
    if (testTitle.length() > 3) {
        auto partial = windowManager->findWindowByTitle(testTitle.substr(0, 3), true);
        EXPECT_TRUE(partial.has_value());
    }
    
    // Try non-existent window
    auto notFound = windowManager->findWindowByTitle("NonExistentWindow12345");
    EXPECT_FALSE(notFound.has_value());
}

// Test finding window by class name
TEST_F(WindowManagerTest, FindWindowByClassName) {
    // Get some windows
    auto windows = windowManager->getAllWindows();
    if (windows.empty()) {
        GTEST_SKIP() << "No windows available for testing";
    }
    
    // Find a window with non-empty class name
    std::string testClassName;
    for (const auto& window : windows) {
        if (!window.className.empty()) {
            testClassName = window.className;
            break;
        }
    }
    
    if (testClassName.empty()) {
        GTEST_SKIP() << "No windows with class names found";
    }
    
    // Try to find the window
    auto found = windowManager->findWindowByClassName(testClassName);
    EXPECT_TRUE(found.has_value());
    EXPECT_EQ(found->className, testClassName);
}

// Test finding windows by process
TEST_F(WindowManagerTest, FindWindowsByProcess) {
    // Get current process windows
    DWORD currentPid = GetCurrentProcessId();
    auto windows = windowManager->getWindowsByProcessId(currentPid);
    
    // Should find at least the test runner window
    EXPECT_FALSE(windows.empty());
    
    // Verify all windows belong to current process
    for (const auto& window : windows) {
        EXPECT_EQ(window.processId, currentPid);
    }
}

// Test window activation
TEST_F(WindowManagerTest, ActivateWindow) {
    // Get current window
    auto currentWindow = windowManager->getForegroundWindow();
    if (!currentWindow.has_value()) {
        GTEST_SKIP() << "Cannot get current window";
    }
    
    // Try to activate it (should already be active)
    auto result = windowManager->activateWindow(currentWindow->handle);
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Try to activate null handle
    result = windowManager->activateWindow(nullptr);
    EXPECT_NE(result, ErrorCode::Success);
}

// Test window state
TEST_F(WindowManagerTest, WindowState) {
    auto window = windowManager->getForegroundWindow();
    if (!window.has_value()) {
        GTEST_SKIP() << "Cannot get foreground window";
    }
    
    // Check if visible
    EXPECT_TRUE(windowManager->isWindowVisible(window->handle));
    
    // Check if minimized
    bool isMinimized = windowManager->isWindowMinimized(window->handle);
    // Current window shouldn't be minimized
    EXPECT_FALSE(isMinimized);
    
    // Check if valid
    EXPECT_TRUE(windowManager->isValidWindow(window->handle));
    
    // Check invalid handle
    EXPECT_FALSE(windowManager->isValidWindow(nullptr));
    EXPECT_FALSE(windowManager->isValidWindow((HWND)0x12345678));
}

// Test window geometry
TEST_F(WindowManagerTest, WindowGeometry) {
    auto window = windowManager->getForegroundWindow();
    if (!window.has_value()) {
        GTEST_SKIP() << "Cannot get foreground window";
    }
    
    // Get window rect
    auto rect = windowManager->getWindowRect(window->handle);
    EXPECT_TRUE(rect.has_value());
    EXPECT_GT(rect->right - rect->left, 0); // Width > 0
    EXPECT_GT(rect->bottom - rect->top, 0); // Height > 0
    
    // Get client rect
    auto clientRect = windowManager->getClientRect(window->handle);
    EXPECT_TRUE(clientRect.has_value());
    EXPECT_GE(clientRect->right, 0);
    EXPECT_GE(clientRect->bottom, 0);
}

// Test process info
TEST_F(WindowManagerTest, ProcessInfo) {
    auto window = windowManager->getForegroundWindow();
    if (!window.has_value()) {
        GTEST_SKIP() << "Cannot get foreground window";
    }
    
    // Get process name
    auto processName = windowManager->getProcessName(window->processId);
    EXPECT_FALSE(processName.empty());
    
    // Get process path
    auto processPath = windowManager->getProcessPath(window->processId);
    EXPECT_FALSE(processPath.empty());
    EXPECT_TRUE(processPath.find(processName) != std::string::npos);
}

// Test window filtering
TEST_F(WindowManagerTest, WindowFiltering) {
    // Get visible windows only
    auto visibleWindows = windowManager->getVisibleWindows();
    EXPECT_FALSE(visibleWindows.empty());
    
    // Verify all are visible
    for (const auto& window : visibleWindows) {
        EXPECT_TRUE(windowManager->isWindowVisible(window.handle));
    }
    
    // Get top-level windows
    auto topLevelWindows = windowManager->getTopLevelWindows();
    EXPECT_FALSE(topLevelWindows.empty());
}

// Test window monitoring
TEST_F(WindowManagerTest, WindowMonitoring) {
    TestUtils::CallbackTracker<WindowManager::WindowInfo> tracker;
    
    // Set up callback
    windowManager->setForegroundWindowChangeCallback(
        [&tracker](const WindowManager::WindowInfo& info) {
            tracker.onCallback(info);
        }
    );
    
    // Start monitoring
    windowManager->startMonitoring();
    
    // Note: Actually changing foreground window in unit test is difficult
    // This test mainly verifies the monitoring setup doesn't crash
    
    // Stop monitoring
    windowManager->stopMonitoring();
}

// Test thread safety
TEST_F(WindowManagerTest, ThreadSafety) {
    const int numThreads = 10;
    const int numOperations = 50;
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);
    
    // Multiple threads querying windows
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, numOperations, &successCount]() {
            for (int j = 0; j < numOperations; ++j) {
                // Various operations
                auto windows = windowManager->getAllWindows();
                if (!windows.empty()) {
                    successCount++;
                }
                
                auto foreground = windowManager->getForegroundWindow();
                if (foreground.has_value()) {
                    successCount++;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should have completed many operations successfully
    EXPECT_GT(successCount, numThreads * numOperations);
}

// Test special window cases
TEST_F(WindowManagerTest, SpecialWindowCases) {
    // Test desktop window
    HWND desktop = GetDesktopWindow();
    EXPECT_TRUE(windowManager->isValidWindow(desktop));
    
    auto desktopInfo = windowManager->getWindowInfo(desktop);
    EXPECT_TRUE(desktopInfo.has_value());
    
    // Test shell window
    HWND shell = GetShellWindow();
    if (shell != nullptr) {
        EXPECT_TRUE(windowManager->isValidWindow(shell));
    }
}

// Test window class patterns
TEST_F(WindowManagerTest, WindowClassPatterns) {
    // Common window classes
    std::vector<std::string> commonClasses = {
        "Notepad",
        "Chrome_WidgetWin_1",
        "CabinetWClass",      // Explorer
        "ConsoleWindowClass", // Console
        "Shell_TrayWnd"       // Taskbar
    };
    
    // Try to find windows with these classes
    for (const auto& className : commonClasses) {
        auto windows = windowManager->findWindowsByClassName(className);
        // Some might not exist, which is fine
        if (!windows.empty()) {
            for (const auto& window : windows) {
                EXPECT_EQ(window.className, className);
            }
        }
    }
}

// Test window hierarchy
TEST_F(WindowManagerTest, WindowHierarchy) {
    auto window = windowManager->getForegroundWindow();
    if (!window.has_value()) {
        GTEST_SKIP() << "Cannot get foreground window";
    }
    
    // Get parent window
    HWND parent = GetParent(window->handle);
    if (parent != nullptr) {
        auto parentInfo = windowManager->getWindowInfo(parent);
        EXPECT_TRUE(parentInfo.has_value());
    }
    
    // Check if window has children
    auto hasChildren = windowManager->hasChildWindows(window->handle);
    // Result depends on the specific window
}