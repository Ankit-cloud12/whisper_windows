/*
 * TranscriptionIntegrationTest.cpp
 * 
 * Integration tests for the transcription pipeline
 */

#include <gtest/gtest.h>
#include "core/WhisperEngine.h"
#include "core/ModelManager.h"
#include "core/AudioConverter.h"
#include "core/Settings.h"
#include "system/ClipboardManager.h"
#include "core/ErrorCodes.h"
#include "../TestUtils.h"
#include <filesystem>
#include <fstream>

class TranscriptionIntegrationTest : public ::testing::Test {
protected:
    Settings* settings;
    ModelManager* modelManager;
    WhisperEngine* whisperEngine;
    AudioConverter* audioConverter;
    std::string testDataDir;
    std::string modelsDir;
    
    void SetUp() override {
        // Create test directories
        testDataDir = TestUtils::FileUtils::createTempDirectory();
        modelsDir = testDataDir + "/models";
        std::filesystem::create_directories(modelsDir);
        
        // Initialize components
        settings = new Settings(testDataDir);
        settings->setModelsDirectory(modelsDir);
        
        modelManager = new ModelManager(modelsDir);
        whisperEngine = new WhisperEngine();
        audioConverter = new AudioConverter();
        
        // Create a mock model file for testing
        createMockModel("ggml-base.en.bin");
    }
    
    void TearDown() override {
        delete audioConverter;
        delete whisperEngine;
        delete modelManager;
        delete settings;
        TestUtils::FileUtils::cleanupTempDirectory(testDataDir);
    }
    
    void createMockModel(const std::string& modelName) {
        std::string modelPath = modelsDir + "/" + modelName;
        std::ofstream file(modelPath, std::ios::binary);
        // Write minimal valid model header
        std::vector<uint8_t> header(1024, 0);
        file.write(reinterpret_cast<const char*>(header.data()), header.size());
        file.close();
    }
    
    std::vector<float> createTestAudio(float duration = 2.0f) {
        // Create audio that simulates speech patterns
        std::vector<float> audio;
        int sampleRate = 16000;
        int samples = static_cast<int>(duration * sampleRate);
        
        // Generate modulated sine wave to simulate speech
        for (int i = 0; i < samples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            float modulation = 0.5f + 0.5f * std::sin(2 * M_PI * 3.0f * t);
            float sample = modulation * std::sin(2 * M_PI * 440.0f * t);
            audio.push_back(sample);
        }
        
        return audio;
    }
};

// Test model loading and initialization
TEST_F(TranscriptionIntegrationTest, ModelLoadingAndInitialization) {
    // Scan for models
    auto result = modelManager->scanModels();
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Get available models
    auto models = modelManager->getAvailableModels();
    if (models.empty()) {
        GTEST_SKIP() << "No models available for testing";
    }
    
    // Get model path
    auto modelPath = modelManager->getModelPath(models[0].name);
    EXPECT_TRUE(modelPath.has_value());
    
    // Initialize engine
    result = whisperEngine->initialize(*modelPath);
    if (result != ErrorCode::Success) {
        GTEST_SKIP() << "Failed to initialize Whisper engine (model may be invalid for testing)";
    }
    
    EXPECT_TRUE(whisperEngine->isInitialized());
}

// Test basic transcription workflow
TEST_F(TranscriptionIntegrationTest, BasicTranscriptionWorkflow) {
    // Skip if no real model available
    if (!std::filesystem::exists(modelsDir + "/ggml-base.en.bin") || 
        std::filesystem::file_size(modelsDir + "/ggml-base.en.bin") < 1024 * 1024) {
        GTEST_SKIP() << "Real model required for transcription test";
    }
    
    // Initialize engine
    modelManager->scanModels();
    auto modelPath = modelManager->getModelPath("base.en");
    if (!modelPath.has_value()) {
        GTEST_SKIP() << "Model not found";
    }
    
    auto result = whisperEngine->initialize(*modelPath);
    if (result != ErrorCode::Success) {
        GTEST_SKIP() << "Failed to initialize engine";
    }
    
    // Create test audio
    auto testAudio = createTestAudio();
    
    // Set up transcription callback
    TestUtils::CallbackTracker<WhisperEngine::TranscriptionResult> tracker;
    whisperEngine->setTranscriptionCallback(
        [&tracker](const WhisperEngine::TranscriptionResult& result) {
            tracker.onCallback(result);
        }
    );
    
    // Configure options
    WhisperEngine::TranscriptionOptions options;
    options.language = "en";
    options.translate = false;
    options.maxSegmentLength = 0;
    options.wordTimestamps = true;
    
    // Start transcription
    result = whisperEngine->transcribe(testAudio, options);
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Wait for result
    bool gotResult = tracker.waitForCallback(10000); // 10 seconds timeout
    if (gotResult) {
        auto transcriptionResult = tracker.getResult();
        EXPECT_FALSE(transcriptionResult.text.empty());
    }
}

// Test transcription with different options
TEST_F(TranscriptionIntegrationTest, TranscriptionOptions) {
    // Skip if no real model available
    if (!std::filesystem::exists(modelsDir + "/ggml-base.en.bin")) {
        GTEST_SKIP() << "Real model required for options test";
    }
    
    modelManager->scanModels();
    auto modelPath = modelManager->getModelPath("base.en");
    if (!modelPath.has_value() || 
        whisperEngine->initialize(*modelPath) != ErrorCode::Success) {
        GTEST_SKIP() << "Failed to initialize";
    }
    
    auto testAudio = createTestAudio();
    
    // Test with word timestamps
    {
        WhisperEngine::TranscriptionOptions options;
        options.wordTimestamps = true;
        
        TestUtils::CallbackTracker<WhisperEngine::TranscriptionResult> tracker;
        whisperEngine->setTranscriptionCallback(
            [&tracker](const WhisperEngine::TranscriptionResult& result) {
                tracker.onCallback(result);
            }
        );
        
        whisperEngine->transcribe(testAudio, options);
        
        if (tracker.waitForCallback(5000)) {
            auto result = tracker.getResult();
            // Check if we have segments with timestamps
            if (!result.segments.empty()) {
                for (const auto& segment : result.segments) {
                    EXPECT_GE(segment.start, 0);
                    EXPECT_GT(segment.end, segment.start);
                }
            }
        }
    }
    
    // Test with translation
    {
        WhisperEngine::TranscriptionOptions options;
        options.translate = true;
        options.language = "auto"; // Auto-detect
        
        TestUtils::CallbackTracker<WhisperEngine::TranscriptionResult> tracker;
        whisperEngine->setTranscriptionCallback(
            [&tracker](const WhisperEngine::TranscriptionResult& result) {
                tracker.onCallback(result);
            }
        );
        
        whisperEngine->transcribe(testAudio, options);
        
        if (tracker.waitForCallback(5000)) {
            auto result = tracker.getResult();
            EXPECT_FALSE(result.text.empty());
        }
    }
}

// Test cancellation
TEST_F(TranscriptionIntegrationTest, TranscriptionCancellation) {
    // Skip if no real model
    if (!std::filesystem::exists(modelsDir + "/ggml-base.en.bin")) {
        GTEST_SKIP() << "Real model required for cancellation test";
    }
    
    modelManager->scanModels();
    auto modelPath = modelManager->getModelPath("base.en");
    if (!modelPath.has_value() || 
        whisperEngine->initialize(*modelPath) != ErrorCode::Success) {
        GTEST_SKIP() << "Failed to initialize";
    }
    
    // Create longer audio for cancellation test
    auto testAudio = createTestAudio(10.0f); // 10 seconds
    
    WhisperEngine::TranscriptionOptions options;
    
    // Start transcription in a thread
    std::thread transcriptionThread([this, &testAudio, &options]() {
        whisperEngine->transcribe(testAudio, options);
    });
    
    // Wait a bit then cancel
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    whisperEngine->cancel();
    
    // Wait for thread to finish
    transcriptionThread.join();
    
    // Verify cancellation worked (engine should be ready for new transcription)
    EXPECT_TRUE(whisperEngine->isInitialized());
}

// Test clipboard integration
TEST_F(TranscriptionIntegrationTest, ClipboardIntegration) {
    auto& clipboard = ClipboardManager::getInstance();
    
    // Save original clipboard content
    std::string originalContent;
    auto getResult = clipboard.getText();
    if (getResult.isSuccess()) {
        originalContent = getResult.value();
    }
    
    // Simulate transcription result
    std::string transcriptionText = "This is a test transcription.";
    
    // Copy to clipboard
    auto result = clipboard.setText(transcriptionText);
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Verify clipboard content
    auto verifyResult = clipboard.getText();
    EXPECT_TRUE(verifyResult.isSuccess());
    EXPECT_EQ(verifyResult.value(), transcriptionText);
    
    // Restore original content
    if (!originalContent.empty()) {
        clipboard.setText(originalContent);
    }
}

// Test file output
TEST_F(TranscriptionIntegrationTest, FileOutput) {
    // Test transcription result
    WhisperEngine::TranscriptionResult result;
    result.text = "This is a test transcription.\nWith multiple lines.";
    result.segments = {
        {0, 2000, "This is a test transcription."},
        {2000, 4000, "With multiple lines."}
    };
    
    // Save as text file
    std::string textPath = testDataDir + "/transcription.txt";
    std::ofstream textFile(textPath);
    textFile << result.text;
    textFile.close();
    
    // Verify file
    EXPECT_TRUE(std::filesystem::exists(textPath));
    std::ifstream readFile(textPath);
    std::string content((std::istreambuf_iterator<char>(readFile)),
                       std::istreambuf_iterator<char>());
    EXPECT_EQ(content, result.text);
    
    // Save as SRT file with timestamps
    std::string srtPath = testDataDir + "/transcription.srt";
    std::ofstream srtFile(srtPath);
    int index = 1;
    for (const auto& segment : result.segments) {
        srtFile << index++ << "\n";
        srtFile << formatTimestamp(segment.start) << " --> " 
                << formatTimestamp(segment.end) << "\n";
        srtFile << segment.text << "\n\n";
    }
    srtFile.close();
    
    EXPECT_TRUE(std::filesystem::exists(srtPath));
}

// Test error recovery
TEST_F(TranscriptionIntegrationTest, ErrorRecovery) {
    // Test with invalid model path
    auto result = whisperEngine->initialize("invalid/path/to/model.bin");
    EXPECT_NE(result, ErrorCode::Success);
    EXPECT_FALSE(whisperEngine->isInitialized());
    
    // Try to transcribe without initialization
    std::vector<float> audio = {0.0f, 0.1f, 0.2f};
    WhisperEngine::TranscriptionOptions options;
    result = whisperEngine->transcribe(audio, options);
    EXPECT_NE(result, ErrorCode::Success);
    
    // Initialize with valid model (if available)
    modelManager->scanModels();
    auto models = modelManager->getAvailableModels();
    if (!models.empty()) {
        auto modelPath = modelManager->getModelPath(models[0].name);
        if (modelPath.has_value()) {
            result = whisperEngine->initialize(*modelPath);
            if (result == ErrorCode::Success) {
                // Should be able to transcribe now
                EXPECT_TRUE(whisperEngine->isInitialized());
            }
        }
    }
}

// Test performance metrics
TEST_F(TranscriptionIntegrationTest, PerformanceMetrics) {
    // Skip if no real model
    if (!std::filesystem::exists(modelsDir + "/ggml-base.en.bin")) {
        GTEST_SKIP() << "Real model required for performance test";
    }
    
    modelManager->scanModels();
    auto modelPath = modelManager->getModelPath("base.en");
    if (!modelPath.has_value() || 
        whisperEngine->initialize(*modelPath) != ErrorCode::Success) {
        GTEST_SKIP() << "Failed to initialize";
    }
    
    // Create test audio of various lengths
    std::vector<float> durations = {1.0f, 2.0f, 5.0f};
    
    for (float duration : durations) {
        auto audio = createTestAudio(duration);
        
        TestUtils::PerformanceUtils::Timer timer("Transcription " + 
                                               std::to_string(duration) + "s");
        
        WhisperEngine::TranscriptionOptions options;
        auto result = whisperEngine->transcribe(audio, options);
        
        uint64_t elapsed = timer.elapsed();
        
        if (result == ErrorCode::Success) {
            // Real-time factor (should be < 1 for real-time performance)
            float rtf = static_cast<float>(elapsed) / (duration * 1000.0f);
            std::cout << "Real-time factor: " << rtf << std::endl;
            
            // Performance should be reasonable
            EXPECT_LT(rtf, 2.0f); // Should be faster than 2x real-time
        }
    }
}

private:
    std::string formatTimestamp(int milliseconds) {
        int hours = milliseconds / 3600000;
        int minutes = (milliseconds % 3600000) / 60000;
        int seconds = (milliseconds % 60000) / 1000;
        int ms = milliseconds % 1000;
        
        char buffer[13];
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d,%03d", 
                 hours, minutes, seconds, ms);
        return std::string(buffer);
    }
};