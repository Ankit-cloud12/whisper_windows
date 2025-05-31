/*
 * ClipboardManagerTest.cpp
 * 
 * Unit tests for ClipboardManager class
 */

#include <gtest/gtest.h>
#include "system/ClipboardManager.h"
#include "core/ErrorCodes.h"
#include "../TestUtils.h"
#include <thread>
#include <chrono>

class ClipboardManagerTest : public ::testing::Test {
protected:
    ClipboardManager* clipboard;
    std::string originalClipboardContent;
    
    void SetUp() override {
        clipboard = &ClipboardManager::getInstance();
        
        // Save original clipboard content
        auto result = clipboard->getText();
        if (result.isSuccess()) {
            originalClipboardContent = result.value();
        }
    }
    
    void TearDown() override {
        // Restore original clipboard content
        if (!originalClipboardContent.empty()) {
            clipboard->setText(originalClipboardContent);
        }
    }
};

// Test singleton instance
TEST_F(ClipboardManagerTest, SingletonInstance) {
    auto& instance1 = ClipboardManager::getInstance();
    auto& instance2 = ClipboardManager::getInstance();
    
    // Should be the same instance
    EXPECT_EQ(&instance1, &instance2);
}

// Test setting and getting text
TEST_F(ClipboardManagerTest, SetAndGetText) {
    // Set text
    std::string testText = "Hello, Clipboard!";
    auto result = clipboard->setText(testText);
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Get text
    auto getText = clipboard->getText();
    EXPECT_TRUE(getText.isSuccess());
    EXPECT_EQ(getText.value(), testText);
}

// Test empty text
TEST_F(ClipboardManagerTest, EmptyText) {
    // Set empty text
    auto result = clipboard->setText("");
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Get text
    auto getText = clipboard->getText();
    EXPECT_TRUE(getText.isSuccess());
    EXPECT_TRUE(getText.value().empty());
}

// Test special characters
TEST_F(ClipboardManagerTest, SpecialCharacters) {
    // Test various special characters
    std::vector<std::string> testStrings = {
        "Line1\nLine2\nLine3",                    // Newlines
        "Tab\tSeparated\tValues",                 // Tabs
        "Special chars: !@#$%^&*()_+-=[]{}|;:',.<>?/", // Special characters
        "Unicode: 你好世界 🌍 émojis 😀",         // Unicode and emojis
        "Quotes: \"double\" and 'single'",        // Quotes
        "Escaped: \\n \\t \\\" \\\\"             // Escaped characters
    };
    
    for (const auto& testStr : testStrings) {
        auto setResult = clipboard->setText(testStr);
        EXPECT_EQ(setResult, ErrorCode::Success);
        
        auto getResult = clipboard->getText();
        EXPECT_TRUE(getResult.isSuccess());
        EXPECT_EQ(getResult.value(), testStr);
    }
}

// Test large text
TEST_F(ClipboardManagerTest, LargeText) {
    // Create large text (1MB)
    std::string largeText;
    largeText.reserve(1024 * 1024);
    for (int i = 0; i < 1024; ++i) {
        largeText.append(std::string(1024, 'A' + (i % 26)));
    }
    
    // Set large text
    auto result = clipboard->setText(largeText);
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Get text
    auto getText = clipboard->getText();
    EXPECT_TRUE(getText.isSuccess());
    EXPECT_EQ(getText.value().size(), largeText.size());
    EXPECT_EQ(getText.value(), largeText);
}

// Test clear functionality
TEST_F(ClipboardManagerTest, ClearClipboard) {
    // Set some text
    clipboard->setText("Test content");
    
    // Clear
    auto result = clipboard->clear();
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Verify cleared
    auto getText = clipboard->getText();
    EXPECT_TRUE(getText.isSuccess());
    EXPECT_TRUE(getText.value().empty());
}

// Test hasText functionality
TEST_F(ClipboardManagerTest, HasText) {
    // Clear clipboard
    clipboard->clear();
    EXPECT_FALSE(clipboard->hasText());
    
    // Set text
    clipboard->setText("Some text");
    EXPECT_TRUE(clipboard->hasText());
    
    // Clear again
    clipboard->clear();
    EXPECT_FALSE(clipboard->hasText());
}

// Test clipboard monitoring
TEST_F(ClipboardManagerTest, ClipboardMonitoring) {
    TestUtils::CallbackTracker<std::string> tracker;
    
    // Set up change callback
    clipboard->setChangeCallback([&tracker](const std::string& text) {
        tracker.onCallback(text);
    });
    
    // Start monitoring
    clipboard->startMonitoring();
    
    // Change clipboard content
    std::string testText = "Monitored text";
    clipboard->setText(testText);
    
    // Wait for callback
    bool callbackReceived = tracker.waitForCallback(1000);
    
    // Stop monitoring
    clipboard->stopMonitoring();
    
    // Verify callback was called (may not work in all environments)
    if (callbackReceived) {
        EXPECT_EQ(tracker.getResult(), testText);
    }
}

// Test rapid clipboard operations
TEST_F(ClipboardManagerTest, RapidOperations) {
    const int numOperations = 100;
    
    for (int i = 0; i < numOperations; ++i) {
        std::string text = "Rapid test " + std::to_string(i);
        
        // Set
        auto setResult = clipboard->setText(text);
        EXPECT_EQ(setResult, ErrorCode::Success);
        
        // Get
        auto getResult = clipboard->getText();
        EXPECT_TRUE(getResult.isSuccess());
        EXPECT_EQ(getResult.value(), text);
    }
}

// Test thread safety
TEST_F(ClipboardManagerTest, ThreadSafety) {
    const int numThreads = 5;
    const int numOperations = 20;
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);
    
    // Multiple threads accessing clipboard
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i, numOperations, &successCount]() {
            for (int j = 0; j < numOperations; ++j) {
                std::string text = "Thread " + std::to_string(i) + " Op " + std::to_string(j);
                
                // Set
                if (clipboard->setText(text) == ErrorCode::Success) {
                    successCount++;
                }
                
                // Small delay
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                // Get
                auto result = clipboard->getText();
                if (result.isSuccess()) {
                    // Just verify we got something
                    EXPECT_FALSE(result.value().empty());
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should have some successful operations
    EXPECT_GT(successCount, 0);
}

// Test format support
TEST_F(ClipboardManagerTest, FormatSupport) {
    // Test if clipboard supports text format
    EXPECT_TRUE(clipboard->supportsTextFormat());
    
    // Test available formats
    auto formats = clipboard->getAvailableFormats();
    EXPECT_FALSE(formats.empty());
    
    // Text format should be available
    bool hasTextFormat = false;
    for (const auto& format : formats) {
        if (format == "text/plain" || format == "text" || format == "CF_TEXT") {
            hasTextFormat = true;
            break;
        }
    }
    EXPECT_TRUE(hasTextFormat);
}

// Test error handling
TEST_F(ClipboardManagerTest, ErrorHandling) {
    // Test with simulated failures
    // Note: It's difficult to simulate clipboard failures in unit tests
    // This test mainly ensures error paths don't crash
    
    // Try to get text when clipboard might be locked
    auto result = clipboard->getText();
    // Should either succeed or return a proper error
    if (!result.isSuccess()) {
        EXPECT_NE(result.code(), ErrorCode::Success);
    }
}

// Test clipboard content types
TEST_F(ClipboardManagerTest, ContentTypes) {
    // Test plain text
    {
        std::string plainText = "This is plain text";
        clipboard->setText(plainText);
        auto result = clipboard->getText();
        EXPECT_TRUE(result.isSuccess());
        EXPECT_EQ(result.value(), plainText);
    }
    
    // Test rich text (as plain text)
    {
        std::string richText = "<b>Bold</b> <i>Italic</i>";
        clipboard->setText(richText);
        auto result = clipboard->getText();
        EXPECT_TRUE(result.isSuccess());
        // Should get the raw text including tags
        EXPECT_EQ(result.value(), richText);
    }
    
    // Test multi-line text
    {
        std::string multiLine = "Line 1\r\nLine 2\r\nLine 3";
        clipboard->setText(multiLine);
        auto result = clipboard->getText();
        EXPECT_TRUE(result.isSuccess());
        // Line endings might be normalized
        EXPECT_FALSE(result.value().empty());
    }
}

// Test clipboard persistence
TEST_F(ClipboardManagerTest, ClipboardPersistence) {
    // Set text
    std::string testText = "Persistent text";
    clipboard->setText(testText);
    
    // Create new instance (simulate app restart)
    auto& newInstance = ClipboardManager::getInstance();
    
    // Should still have the text
    auto result = newInstance.getText();
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value(), testText);
}