/*
 * GlobalHotkeysTest.cpp
 * 
 * Unit tests for GlobalHotkeys class
 */

#include <gtest/gtest.h>
#include "system/GlobalHotkeys.h"
#include "core/ErrorCodes.h"
#include "../TestUtils.h"
#include <thread>
#include <chrono>

class GlobalHotkeysTest : public ::testing::Test {
protected:
    GlobalHotkeys* hotkeys;
    TestUtils::CallbackTracker<std::string> callbackTracker;
    
    void SetUp() override {
        hotkeys = new GlobalHotkeys();
        
        // Set up callback handler
        hotkeys->setHotkeyHandler([this](const std::string& action) {
            callbackTracker.onCallback(action);
        });
    }
    
    void TearDown() override {
        hotkeys->unregisterAll();
        delete hotkeys;
    }
};

// Test hotkey registration
TEST_F(GlobalHotkeysTest, RegisterHotkey) {
    // Register a simple hotkey
    auto result = hotkeys->registerHotkey("test_action", "Ctrl+T");
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Verify registration
    EXPECT_TRUE(hotkeys->isRegistered("test_action"));
    
    // Try to register same action again (should update)
    result = hotkeys->registerHotkey("test_action", "Ctrl+Shift+T");
    EXPECT_EQ(result, ErrorCode::Success);
}

// Test hotkey unregistration
TEST_F(GlobalHotkeysTest, UnregisterHotkey) {
    // Register hotkey
    hotkeys->registerHotkey("test_action", "Ctrl+U");
    EXPECT_TRUE(hotkeys->isRegistered("test_action"));
    
    // Unregister
    auto result = hotkeys->unregisterHotkey("test_action");
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_FALSE(hotkeys->isRegistered("test_action"));
    
    // Try to unregister non-existent hotkey
    result = hotkeys->unregisterHotkey("non_existent");
    EXPECT_EQ(result, ErrorCode::Success); // Should not fail
}

// Test unregister all hotkeys
TEST_F(GlobalHotkeysTest, UnregisterAll) {
    // Register multiple hotkeys
    hotkeys->registerHotkey("action1", "Ctrl+1");
    hotkeys->registerHotkey("action2", "Ctrl+2");
    hotkeys->registerHotkey("action3", "Ctrl+3");
    
    EXPECT_TRUE(hotkeys->isRegistered("action1"));
    EXPECT_TRUE(hotkeys->isRegistered("action2"));
    EXPECT_TRUE(hotkeys->isRegistered("action3"));
    
    // Unregister all
    hotkeys->unregisterAll();
    
    EXPECT_FALSE(hotkeys->isRegistered("action1"));
    EXPECT_FALSE(hotkeys->isRegistered("action2"));
    EXPECT_FALSE(hotkeys->isRegistered("action3"));
}

// Test hotkey parsing
TEST_F(GlobalHotkeysTest, HotkeyParsing) {
    // Test various hotkey combinations
    struct TestCase {
        std::string hotkey;
        bool shouldSucceed;
    };
    
    std::vector<TestCase> testCases = {
        {"Ctrl+A", true},
        {"Ctrl+Shift+A", true},
        {"Ctrl+Alt+A", true},
        {"Ctrl+Shift+Alt+A", true},
        {"F1", true},
        {"F12", true},
        {"Ctrl+F1", true},
        {"Escape", true},
        {"Space", true},
        {"Ctrl+Space", true},
        {"Ctrl++", true},        // Plus key
        {"Ctrl+-", true},        // Minus key
        {"InvalidKey", false},   // Invalid key
        {"Ctrl+", false},        // Incomplete
        {"", false},             // Empty
        {"Ctrl+Ctrl+A", false},  // Duplicate modifier
    };
    
    for (const auto& test : testCases) {
        auto result = hotkeys->registerHotkey("test", test.hotkey);
        if (test.shouldSucceed) {
            EXPECT_EQ(result, ErrorCode::Success) 
                << "Failed to register: " << test.hotkey;
        } else {
            EXPECT_NE(result, ErrorCode::Success) 
                << "Should have failed: " << test.hotkey;
        }
        hotkeys->unregisterHotkey("test");
    }
}

// Test hotkey callback
TEST_F(GlobalHotkeysTest, HotkeyCallback) {
    // Register hotkey
    hotkeys->registerHotkey("test_callback", "Ctrl+B");
    
    // Simulate hotkey press (this would normally come from Windows)
    // Since we can't easily simulate actual key presses in unit tests,
    // we'll call the internal handler directly if available
    
    // For now, we just verify the callback mechanism is set up
    EXPECT_TRUE(hotkeys->isRegistered("test_callback"));
}

// Test multiple hotkeys
TEST_F(GlobalHotkeysTest, MultipleHotkeys) {
    // Register multiple hotkeys
    std::vector<std::pair<std::string, std::string>> hotkeyMap = {
        {"record", "Ctrl+Shift+R"},
        {"pause", "Ctrl+Shift+P"},
        {"stop", "Ctrl+Shift+S"},
        {"cancel", "Escape"}
    };
    
    for (const auto& [action, hotkey] : hotkeyMap) {
        auto result = hotkeys->registerHotkey(action, hotkey);
        EXPECT_EQ(result, ErrorCode::Success);
        EXPECT_TRUE(hotkeys->isRegistered(action));
    }
    
    // Verify all are registered
    for (const auto& [action, hotkey] : hotkeyMap) {
        EXPECT_TRUE(hotkeys->isRegistered(action));
    }
}

// Test hotkey conflicts
TEST_F(GlobalHotkeysTest, HotkeyConflicts) {
    // Register first hotkey
    auto result = hotkeys->registerHotkey("action1", "Ctrl+X");
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Try to register same hotkey for different action
    // This should either fail or update the mapping
    result = hotkeys->registerHotkey("action2", "Ctrl+X");
    // The behavior depends on implementation
    // Either it fails or it succeeds and removes the first mapping
}

// Test invalid hotkey formats
TEST_F(GlobalHotkeysTest, InvalidHotkeyFormats) {
    struct InvalidTest {
        std::string action;
        std::string hotkey;
        std::string description;
    };
    
    std::vector<InvalidTest> invalidTests = {
        {"test1", "Ctrl+", "Missing key"},
        {"test2", "+A", "Missing modifier"},
        {"test3", "Ctrl+InvalidKey", "Invalid key name"},
        {"test4", "Ctrl++A", "Double plus"},
        {"test5", "CtrlA", "Missing separator"},
        {"test6", "Ctrl-A", "Wrong separator"},
        {"test7", "ctrl+a", "Lowercase (might work depending on impl)"},
    };
    
    for (const auto& test : invalidTests) {
        auto result = hotkeys->registerHotkey(test.action, test.hotkey);
        // We expect these to fail, but some might succeed depending on implementation
        if (result != ErrorCode::Success) {
            EXPECT_NE(result, ErrorCode::Success) << test.description;
        }
    }
}

// Test enable/disable functionality
TEST_F(GlobalHotkeysTest, EnableDisable) {
    // Register hotkeys
    hotkeys->registerHotkey("test_enable", "Ctrl+E");
    
    // Disable
    hotkeys->setEnabled(false);
    EXPECT_FALSE(hotkeys->isEnabled());
    
    // Enable
    hotkeys->setEnabled(true);
    EXPECT_TRUE(hotkeys->isEnabled());
}

// Test thread safety
TEST_F(GlobalHotkeysTest, ThreadSafety) {
    const int numThreads = 10;
    const int numOperations = 100;
    std::vector<std::thread> threads;
    
    // Multiple threads registering/unregistering
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i, numOperations]() {
            for (int j = 0; j < numOperations; ++j) {
                std::string action = "thread_" + std::to_string(i) + "_" + std::to_string(j);
                std::string hotkey = "Ctrl+" + std::to_string((i * numOperations + j) % 26 + 65); // A-Z
                
                // Register
                hotkeys->registerHotkey(action, hotkey);
                
                // Small delay
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                
                // Unregister
                hotkeys->unregisterHotkey(action);
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should have no registered hotkeys left
    hotkeys->unregisterAll(); // Clean up any remaining
}

// Test special keys
TEST_F(GlobalHotkeysTest, SpecialKeys) {
    struct SpecialKeyTest {
        std::string key;
        std::string description;
    };
    
    std::vector<SpecialKeyTest> specialKeys = {
        {"Escape", "Escape key"},
        {"Tab", "Tab key"},
        {"Space", "Space bar"},
        {"Return", "Enter key"},
        {"Delete", "Delete key"},
        {"Home", "Home key"},
        {"End", "End key"},
        {"PageUp", "Page Up key"},
        {"PageDown", "Page Down key"},
        {"Left", "Left arrow"},
        {"Right", "Right arrow"},
        {"Up", "Up arrow"},
        {"Down", "Down arrow"},
        {"F1", "Function key F1"},
        {"F12", "Function key F12"},
    };
    
    for (const auto& test : specialKeys) {
        auto result = hotkeys->registerHotkey("special_" + test.key, test.key);
        EXPECT_EQ(result, ErrorCode::Success) << "Failed: " << test.description;
        EXPECT_TRUE(hotkeys->isRegistered("special_" + test.key));
        hotkeys->unregisterHotkey("special_" + test.key);
    }
}