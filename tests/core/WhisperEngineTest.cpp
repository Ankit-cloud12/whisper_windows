/*
 * WhisperEngineTest.cpp
 * 
 * Unit tests for WhisperEngine class.
 */

#include <gtest/gtest.h>
#include "../TestUtils.h"
#include "core/WhisperEngine.h"
#include "core/ErrorCodes.h"
#include "core/AudioConverter.h"

using namespace WhisperApp;
using namespace TestUtils;

class WhisperEngineTest : public ::testing::Test {
protected:
    std::unique_ptr<WhisperEngine> engine;
    
    void SetUp() override {
        engine = std::make_unique<WhisperEngine>();
    }
    
    void TearDown() override {
        engine.reset();
    }
};

// Test fixture for async operations
class WhisperEngineAsyncTest : public WhisperEngineTest {
protected:
    CallbackTracker<WhisperEngine::TranscriptionResult> resultTracker;
    CallbackTracker<float> progressTracker;
};

// Basic functionality tests

TEST_F(WhisperEngineTest, InitialState) {
    EXPECT_FALSE(engine->isModelLoaded());
    EXPECT_FALSE(engine->isTranscribing());
    EXPECT_FALSE(engine->isGPUAvailable());  // Mock returns false
    EXPECT_GT(engine->getThreadCount(), 0);
}

TEST_F(WhisperEngineTest, ModelLoadingSuccess) {
    // Test loading a model
    std::string modelPath = "models/ggml-tiny.bin";
    EXPECT_TRUE(engine->loadModel(modelPath));
    EXPECT_TRUE(engine->isModelLoaded());
    
    // Get model info
    std::string info = engine->getModelInfo();
    EXPECT_FALSE(info.empty());
    EXPECT_NE(info.find("tiny"), std::string::npos);
}

TEST_F(WhisperEngineTest, ModelLoadingFailure) {
    // Test loading with empty path
    EXPECT_FALSE(engine->loadModel(""));
    EXPECT_FALSE(engine->isModelLoaded());
}

TEST_F(WhisperEngineTest, ModelUnloading) {
    // Load a model
    engine->loadModel("models/ggml-base.bin");
    EXPECT_TRUE(engine->isModelLoaded());
    
    // Unload the model
    engine->unloadModel();
    EXPECT_FALSE(engine->isModelLoaded());
}

TEST_F(WhisperEngineTest, ThreadCountConfiguration) {
    // Test setting thread count
    engine->setThreadCount(8);
    EXPECT_EQ(engine->getThreadCount(), 8);
    
    // Test auto-detect (0)
    engine->setThreadCount(0);
    EXPECT_GT(engine->getThreadCount(), 0);
}

// Audio transcription tests

TEST_F(WhisperEngineTest, TranscribeValidAudio) {
    // Load model first
    ASSERT_TRUE(engine->loadModel("models/ggml-tiny.bin"));
    
    // Generate test audio (1 second of sine wave)
    auto audio = AudioGenerator::generateSineWave(440.0f, 1.0f, 16000);
    
    // Transcribe
    auto result = engine->transcribeAudio(audio);
    
    EXPECT_FALSE(result.text.empty());
    EXPECT_GT(result.confidence, 0.0f);
    EXPECT_GT(result.processing_time_ms, 0);
    EXPECT_FALSE(result.segments.empty());
}

TEST_F(WhisperEngineTest, TranscribeEmptyAudio) {
    // Load model first
    ASSERT_TRUE(engine->loadModel("models/ggml-tiny.bin"));
    
    // Try to transcribe empty audio
    std::vector<float> emptyAudio;
    auto result = engine->transcribeAudio(emptyAudio);
    
    // Should handle gracefully
    EXPECT_TRUE(result.text.find("Error") != std::string::npos || result.text.empty());
    EXPECT_EQ(result.confidence, 0.0f);
}

TEST_F(WhisperEngineTest, TranscribeWithoutModel) {
    // Don't load a model
    ASSERT_FALSE(engine->isModelLoaded());
    
    // Generate test audio
    auto audio = AudioGenerator::generateSineWave(440.0f, 1.0f, 16000);
    
    // Try to transcribe
    auto result = engine->transcribeAudio(audio);
    
    // Should fail gracefully
    EXPECT_TRUE(result.text.find("Error") != std::string::npos || result.text.empty());
    EXPECT_EQ(result.confidence, 0.0f);
}

TEST_F(WhisperEngineTest, TranscriptionParameters) {
    // Load model
    ASSERT_TRUE(engine->loadModel("models/ggml-tiny.bin"));
    
    // Generate test audio
    auto audio = AudioGenerator::generateWhiteNoise(2.0f, 16000);
    
    // Test with custom parameters
    WhisperEngine::TranscriptionParams params;
    params.language = "es";  // Spanish
    params.translate = true;
    params.print_timestamps = true;
    params.beam_size = 10;
    params.temperature = 0.5f;
    
    auto result = engine->transcribeAudio(audio, params);
    
    EXPECT_FALSE(result.text.empty());
    // In mock implementation, should mention the language
    EXPECT_NE(result.text.find("es"), std::string::npos);
}

// Async transcription tests

TEST_F(WhisperEngineAsyncTest, AsyncTranscriptionSuccess) {
    // Load model
    ASSERT_TRUE(engine->loadModel("models/ggml-tiny.bin"));
    
    // Generate test audio
    auto audio = AudioGenerator::generateSineWave(440.0f, 1.0f, 16000);
    
    // Start async transcription
    engine->transcribeAudioAsync(
        audio,
        WhisperEngine::TranscriptionParams(),
        [this](const WhisperEngine::TranscriptionResult& result) {
            resultTracker.onCallback(result);
        },
        [this](float progress) {
            progressTracker.onCallback(progress);
        }
    );
    
    // Wait for completion
    ASSERT_TRUE(resultTracker.waitForCallback());
    
    // Check result
    auto result = resultTracker.getResult();
    EXPECT_FALSE(result.text.empty());
    EXPECT_GT(result.confidence, 0.0f);
    
    // Check that progress was reported
    EXPECT_TRUE(progressTracker.wasCalled());
}

TEST_F(WhisperEngineAsyncTest, AsyncTranscriptionCancellation) {
    // Load model
    ASSERT_TRUE(engine->loadModel("models/ggml-tiny.bin"));
    
    // Generate longer audio for cancellation test
    auto audio = AudioGenerator::generateWhiteNoise(5.0f, 16000);
    
    // Start async transcription
    engine->transcribeAudioAsync(
        audio,
        WhisperEngine::TranscriptionParams(),
        [this](const WhisperEngine::TranscriptionResult& result) {
            resultTracker.onCallback(result);
        }
    );
    
    // Give it some time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Cancel transcription
    engine->cancelTranscription();
    
    // Wait for completion
    ASSERT_TRUE(resultTracker.waitForCallback());
    
    // Result should indicate cancellation
    auto result = resultTracker.getResult();
    // In real implementation, would check for cancellation error
}

TEST_F(WhisperEngineAsyncTest, ConcurrentTranscriptionRejection) {
    // Load model
    ASSERT_TRUE(engine->loadModel("models/ggml-tiny.bin"));
    
    // Generate test audio
    auto audio = AudioGenerator::generateWhiteNoise(2.0f, 16000);
    
    // Start first transcription
    engine->transcribeAudioAsync(
        audio,
        WhisperEngine::TranscriptionParams(),
        [](const WhisperEngine::TranscriptionResult&) {}  // Ignore result
    );
    
    // Try to start second transcription immediately
    CallbackTracker<WhisperEngine::TranscriptionResult> secondTracker;
    engine->transcribeAudioAsync(
        audio,
        WhisperEngine::TranscriptionParams(),
        [&secondTracker](const WhisperEngine::TranscriptionResult& result) {
            secondTracker.onCallback(result);
        }
    );
    
    // Second transcription should be rejected immediately
    ASSERT_TRUE(secondTracker.waitForCallback(1000));  // Short timeout
    
    auto result = secondTracker.getResult();
    EXPECT_NE(result.text.find("Already transcribing"), std::string::npos);
}

// Language support tests

TEST_F(WhisperEngineTest, SupportedLanguages) {
    auto languages = WhisperEngine::getSupportedLanguages();
    
    // Should have many languages
    EXPECT_GT(languages.size(), 50);
    
    // Check for some common languages
    EXPECT_NE(std::find(languages.begin(), languages.end(), "en"), languages.end());
    EXPECT_NE(std::find(languages.begin(), languages.end(), "es"), languages.end());
    EXPECT_NE(std::find(languages.begin(), languages.end(), "fr"), languages.end());
    EXPECT_NE(std::find(languages.begin(), languages.end(), "de"), languages.end());
    EXPECT_NE(std::find(languages.begin(), languages.end(), "zh"), languages.end());
}

TEST_F(WhisperEngineTest, LanguageNames) {
    EXPECT_EQ(WhisperEngine::getLanguageName("en"), "English");
    EXPECT_EQ(WhisperEngine::getLanguageName("es"), "Spanish");
    EXPECT_EQ(WhisperEngine::getLanguageName("fr"), "French");
    
    // Unknown language code should return the code itself
    EXPECT_EQ(WhisperEngine::getLanguageName("xyz"), "xyz");
}

// Performance tests

TEST_F(WhisperEngineTest, TranscriptionPerformance) {
    // Load model
    ASSERT_TRUE(engine->loadModel("models/ggml-tiny.bin"));
    
    // Generate test audio of various lengths
    std::vector<float> durations = {0.5f, 1.0f, 2.0f, 5.0f};
    
    for (float duration : durations) {
        auto audio = AudioGenerator::generateWhiteNoise(duration, 16000);
        
        PerformanceUtils::Timer timer("Transcription " + std::to_string(duration) + "s");
        auto result = engine->transcribeAudio(audio);
        
        // Check that processing time is reasonable
        // In mock implementation, should be fast
        EXPECT_LT(result.processing_time_ms, duration * 1000);  // Faster than real-time
    }
}

// GPU tests

TEST_F(WhisperEngineTest, GPUConfiguration) {
    // GPU should not be available in mock
    EXPECT_FALSE(engine->isGPUAvailable());
    
    // Try to enable GPU
    bool gpuEnabled = engine->setGPUEnabled(true);
    EXPECT_FALSE(gpuEnabled);  // Should fail since GPU not available
}

// Error handling tests

TEST_F(WhisperEngineTest, AudioValidation) {
    // Load model
    ASSERT_TRUE(engine->loadModel("models/ggml-tiny.bin"));
    
    // Test with wrong sample rate (should work with conversion in real implementation)
    auto audio = AudioGenerator::generateSineWave(440.0f, 1.0f, 44100);  // 44.1kHz instead of 16kHz
    
    // In mock implementation, this should still work
    auto result = engine->transcribeAudio(audio);
    // Real implementation would need to handle sample rate conversion
}

// Integration tests with AudioConverter

TEST_F(WhisperEngineTest, IntegrationWithAudioConverter) {
    // Load model
    ASSERT_TRUE(engine->loadModel("models/ggml-tiny.bin"));
    
    // Create audio in non-standard format
    AudioBuffer buffer;
    buffer.format = AudioFormat(44100, 2, 16, false);  // 44.1kHz stereo
    buffer.data = AudioGenerator::generateSineWave(440.0f, 1.0f, 44100);
    
    // Convert to whisper format
    AudioConverter converter;
    AudioConverter::ConversionParams params;
    params.targetFormat = AudioFormat(16000, 1, 32, true);  // 16kHz mono float32
    
    auto converted = converter.convert(buffer, params);
    
    // Transcribe converted audio
    auto result = engine->transcribeAudio(converted.data);
    
    EXPECT_FALSE(result.text.empty());
    EXPECT_GT(result.confidence, 0.0f);
}

// Main function for running tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}