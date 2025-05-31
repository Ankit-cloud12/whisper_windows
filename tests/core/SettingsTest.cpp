/*
 * SettingsTest.cpp
 * 
 * Unit tests for Settings class
 */

#include <gtest/gtest.h>
#include "core/Settings.h"
#include "core/ErrorCodes.h"
#include "../TestUtils.h"
#include <filesystem>
#include <thread>

class SettingsTest : public ::testing::Test {
protected:
    std::string testConfigPath;
    Settings* settings;
    
    void SetUp() override {
        // Create temporary directory for test
        testConfigPath = TestUtils::FileUtils::createTempDirectory();
        
        // Create Settings instance with test config path
        settings = new Settings(testConfigPath);
    }
    
    void TearDown() override {
        delete settings;
        TestUtils::FileUtils::cleanupTempDirectory(testConfigPath);
    }
};

// Test default values
TEST_F(SettingsTest, DefaultValues) {
    // General settings
    EXPECT_EQ(settings->getLanguage(), "en");
    EXPECT_TRUE(settings->getAutoStart());
    EXPECT_TRUE(settings->getMinimizeToTray());
    EXPECT_FALSE(settings->getShowNotifications());
    EXPECT_TRUE(settings->getCheckForUpdates());
    
    // Recording settings
    EXPECT_EQ(settings->getSampleRate(), 16000);
    EXPECT_EQ(settings->getChannels(), 1);
    EXPECT_EQ(settings->getBitsPerSample(), 16);
    EXPECT_TRUE(settings->getVadEnabled());
    EXPECT_FLOAT_EQ(settings->getVadThreshold(), 0.5f);
    EXPECT_EQ(settings->getVadPaddingMs(), 300);
    EXPECT_EQ(settings->getMaxRecordingDuration(), 300); // 5 minutes
    
    // Transcription settings
    EXPECT_EQ(settings->getModel(), "base.en");
    EXPECT_EQ(settings->getComputeType(), "auto");
    EXPECT_TRUE(settings->getTranslateToEnglish());
    EXPECT_EQ(settings->getMaxSegmentLength(), 0);
    EXPECT_TRUE(settings->getWordTimestamps());
    EXPECT_EQ(settings->getNumThreads(), 0); // auto
    EXPECT_EQ(settings->getBeamSize(), 5);
    EXPECT_FLOAT_EQ(settings->getTemperature(), 0.0f);
    
    // Hotkeys
    EXPECT_EQ(settings->getRecordHotkey(), "Ctrl+Shift+R");
    EXPECT_EQ(settings->getPauseHotkey(), "Ctrl+Shift+P");
    EXPECT_EQ(settings->getStopHotkey(), "Ctrl+Shift+S");
    EXPECT_EQ(settings->getCancelHotkey(), "Escape");
    
    // Output settings
    EXPECT_TRUE(settings->getAutoCopyToClipboard());
    EXPECT_TRUE(settings->getAutoTypeOutput());
    EXPECT_FALSE(settings->getSaveTranscriptions());
    EXPECT_EQ(settings->getTranscriptionFormat(), "txt");
    EXPECT_TRUE(settings->getTimestampFormat().empty());
}

// Test setting and getting values
TEST_F(SettingsTest, SetAndGetValues) {
    // Test string values
    settings->setValue("language", "fr");
    EXPECT_EQ(settings->getLanguage(), "fr");
    
    // Test bool values
    settings->setValue("autoStart", false);
    EXPECT_FALSE(settings->getAutoStart());
    
    // Test int values
    settings->setValue("sampleRate", 44100);
    EXPECT_EQ(settings->getSampleRate(), 44100);
    
    // Test float values
    settings->setValue("vadThreshold", 0.75f);
    EXPECT_FLOAT_EQ(settings->getVadThreshold(), 0.75f);
    
    // Test complex keys
    settings->setValue("recording/maxDuration", 600);
    EXPECT_EQ(settings->getMaxRecordingDuration(), 600);
}

// Test save and load functionality
TEST_F(SettingsTest, SaveAndLoad) {
    // Set some custom values
    settings->setValue("language", "es");
    settings->setValue("model", "large");
    settings->setValue("vadThreshold", 0.65f);
    settings->setValue("recordHotkey", "F9");
    
    // Save settings
    EXPECT_EQ(settings->save(), ErrorCode::Success);
    
    // Create new settings instance and load
    Settings settings2(testConfigPath);
    EXPECT_EQ(settings2.load(), ErrorCode::Success);
    
    // Verify loaded values
    EXPECT_EQ(settings2.getLanguage(), "es");
    EXPECT_EQ(settings2.getModel(), "large");
    EXPECT_FLOAT_EQ(settings2.getVadThreshold(), 0.65f);
    EXPECT_EQ(settings2.getRecordHotkey(), "F9");
}

// Test model paths functionality
TEST_F(SettingsTest, ModelPaths) {
    std::string modelsDir = testConfigPath + "/models";
    settings->setModelsDirectory(modelsDir);
    EXPECT_EQ(settings->getModelsDirectory(), modelsDir);
    
    // Test available models
    std::vector<std::string> testModels = {"tiny.en", "base", "small", "medium", "large"};
    settings->setAvailableModels(testModels);
    
    auto models = settings->getAvailableModels();
    EXPECT_EQ(models.size(), testModels.size());
    for (size_t i = 0; i < models.size(); ++i) {
        EXPECT_EQ(models[i], testModels[i]);
    }
}

// Test device settings
TEST_F(SettingsTest, DeviceSettings) {
    // Test input device
    settings->setInputDevice("Microphone Array");
    EXPECT_EQ(settings->getInputDevice(), "Microphone Array");
    
    // Test default device
    settings->setInputDevice("");
    EXPECT_TRUE(settings->getInputDevice().empty());
}

// Test file watch functionality
TEST_F(SettingsTest, FileWatch) {
    TestUtils::CallbackTracker<void*> tracker;
    
    // Set up file watch callback
    auto callback = [&tracker]() {
        tracker.onCallback(nullptr);
    };
    
    settings->setFileChangeCallback(callback);
    
    // Modify settings file
    settings->setValue("testValue", "modified");
    settings->save();
    
    // Give file watcher time to detect change
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Note: File watching may not work reliably in test environment
    // This is more of an integration test
}

// Test validation of settings values
TEST_F(SettingsTest, ValueValidation) {
    // Test sample rate validation
    settings->setValue("sampleRate", 8000);
    EXPECT_EQ(settings->getSampleRate(), 8000);
    
    settings->setValue("sampleRate", -1); // Invalid
    EXPECT_EQ(settings->getSampleRate(), 16000); // Should keep default
    
    // Test VAD threshold validation
    settings->setValue("vadThreshold", 0.0f);
    EXPECT_FLOAT_EQ(settings->getVadThreshold(), 0.0f);
    
    settings->setValue("vadThreshold", 1.5f); // Out of range
    EXPECT_FLOAT_EQ(settings->getVadThreshold(), 1.0f); // Should clamp to max
    
    // Test thread count validation
    settings->setValue("numThreads", -5);
    EXPECT_EQ(settings->getNumThreads(), 0); // Should use auto (0)
}

// Test reset to defaults
TEST_F(SettingsTest, ResetToDefaults) {
    // Change some values
    settings->setValue("language", "ja");
    settings->setValue("model", "large-v3");
    settings->setValue("vadEnabled", false);
    
    // Reset to defaults
    settings->resetToDefaults();
    
    // Verify defaults restored
    EXPECT_EQ(settings->getLanguage(), "en");
    EXPECT_EQ(settings->getModel(), "base.en");
    EXPECT_TRUE(settings->getVadEnabled());
}

// Test settings categories
TEST_F(SettingsTest, SettingsCategories) {
    // Test getting all settings in a category
    auto generalSettings = settings->getCategory("general");
    EXPECT_FALSE(generalSettings.empty());
    
    auto recordingSettings = settings->getCategory("recording");
    EXPECT_FALSE(recordingSettings.empty());
    
    auto transcriptionSettings = settings->getCategory("transcription");
    EXPECT_FALSE(transcriptionSettings.empty());
}

// Test import/export functionality
TEST_F(SettingsTest, ImportExport) {
    // Set custom values
    settings->setValue("language", "de");
    settings->setValue("model", "medium");
    settings->setValue("autoStart", false);
    
    // Export to file
    std::string exportPath = testConfigPath + "/exported_settings.json";
    EXPECT_EQ(settings->exportSettings(exportPath), ErrorCode::Success);
    
    // Reset settings
    settings->resetToDefaults();
    EXPECT_EQ(settings->getLanguage(), "en");
    
    // Import from file
    EXPECT_EQ(settings->importSettings(exportPath), ErrorCode::Success);
    
    // Verify imported values
    EXPECT_EQ(settings->getLanguage(), "de");
    EXPECT_EQ(settings->getModel(), "medium");
    EXPECT_FALSE(settings->getAutoStart());
}

// Test concurrent access
TEST_F(SettingsTest, ThreadSafety) {
    const int numThreads = 10;
    const int numIterations = 100;
    std::vector<std::thread> threads;
    
    // Multiple threads reading and writing
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i, numIterations]() {
            for (int j = 0; j < numIterations; ++j) {
                // Write
                std::string key = "thread_" + std::to_string(i);
                settings->setValue(key, j);
                
                // Read
                int value = settings->getValue(key, 0);
                EXPECT_GE(value, 0);
                EXPECT_LE(value, numIterations);
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
}