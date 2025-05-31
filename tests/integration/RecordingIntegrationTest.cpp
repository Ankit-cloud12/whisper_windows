/*
 * RecordingIntegrationTest.cpp
 * 
 * Integration tests for the recording workflow
 */

#include <gtest/gtest.h>
#include "core/AudioCapture.h"
#include "core/AudioConverter.h"
#include "core/DeviceManager.h"
#include "core/Settings.h"
#include "core/ErrorCodes.h"
#include "../TestUtils.h"
#include <thread>
#include <chrono>

class RecordingIntegrationTest : public ::testing::Test {
protected:
    Settings* settings;
    DeviceManager* deviceManager;
    AudioCapture* audioCapture;
    AudioConverter* audioConverter;
    std::string testDataDir;
    
    void SetUp() override {
        // Create test directory
        testDataDir = TestUtils::FileUtils::createTempDirectory();
        
        // Initialize components
        settings = new Settings(testDataDir);
        deviceManager = new DeviceManager();
        audioCapture = new AudioCapture();
        audioConverter = new AudioConverter();
        
        // Configure settings for testing
        settings->setSampleRate(16000);
        settings->setChannels(1);
        settings->setBitsPerSample(16);
        settings->setVadEnabled(true);
        settings->setVadThreshold(0.5f);
    }
    
    void TearDown() override {
        delete audioConverter;
        delete audioCapture;
        delete deviceManager;
        delete settings;
        TestUtils::FileUtils::cleanupTempDirectory(testDataDir);
    }
};

// Test device discovery and selection
TEST_F(RecordingIntegrationTest, DeviceDiscoveryAndSelection) {
    // Scan for devices
    auto result = deviceManager->scanDevices();
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Get input devices
    auto inputDevices = deviceManager->getInputDevices();
    
    // Should have at least one input device (might be virtual)
    if (inputDevices.empty()) {
        GTEST_SKIP() << "No input devices available";
    }
    
    // Select first device
    auto& device = inputDevices[0];
    settings->setInputDevice(device.id);
    
    // Verify device info
    EXPECT_FALSE(device.name.empty());
    EXPECT_FALSE(device.id.empty());
    EXPECT_GT(device.channels, 0);
    EXPECT_GT(device.sampleRate, 0);
}

// Test audio capture initialization
TEST_F(RecordingIntegrationTest, AudioCaptureInitialization) {
    // Get default device
    auto devices = deviceManager->getInputDevices();
    if (devices.empty()) {
        GTEST_SKIP() << "No input devices available";
    }
    
    // Initialize with settings
    AudioFormat format;
    format.sampleRate = settings->getSampleRate();
    format.channels = settings->getChannels();
    format.bitsPerSample = settings->getBitsPerSample();
    
    auto result = audioCapture->initialize(devices[0].id, format);
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Verify initialization
    EXPECT_TRUE(audioCapture->isInitialized());
    EXPECT_EQ(audioCapture->getFormat().sampleRate, format.sampleRate);
    EXPECT_EQ(audioCapture->getFormat().channels, format.channels);
    EXPECT_EQ(audioCapture->getFormat().bitsPerSample, format.bitsPerSample);
}

// Test recording workflow
TEST_F(RecordingIntegrationTest, RecordingWorkflow) {
    // Initialize audio capture
    auto devices = deviceManager->getInputDevices();
    if (devices.empty()) {
        GTEST_SKIP() << "No input devices available";
    }
    
    AudioFormat format;
    format.sampleRate = 16000;
    format.channels = 1;
    format.bitsPerSample = 16;
    
    audioCapture->initialize(devices[0].id, format);
    
    // Set up audio callback
    std::vector<float> capturedAudio;
    std::mutex audioMutex;
    
    audioCapture->setAudioCallback([&capturedAudio, &audioMutex](const float* data, size_t samples) {
        std::lock_guard<std::mutex> lock(audioMutex);
        capturedAudio.insert(capturedAudio.end(), data, data + samples);
    });
    
    // Start recording
    auto result = audioCapture->start();
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_TRUE(audioCapture->isRecording());
    
    // Record for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop recording
    result = audioCapture->stop();
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_FALSE(audioCapture->isRecording());
    
    // Check if we captured something (might be silence)
    std::lock_guard<std::mutex> lock(audioMutex);
    EXPECT_FALSE(capturedAudio.empty());
}

// Test VAD integration
TEST_F(RecordingIntegrationTest, VADIntegration) {
    // Create test audio with speech and silence
    auto silence1 = TestUtils::AudioGenerator::generateSilence(0.5f);
    auto speech = TestUtils::AudioGenerator::generateSineWave(440.0f, 1.0f);
    auto silence2 = TestUtils::AudioGenerator::generateSilence(0.5f);
    
    // Combine audio
    std::vector<float> testAudio;
    testAudio.insert(testAudio.end(), silence1.begin(), silence1.end());
    testAudio.insert(testAudio.end(), speech.begin(), speech.end());
    testAudio.insert(testAudio.end(), silence2.begin(), silence2.end());
    
    // Process with VAD
    bool speechDetected = false;
    size_t speechStart = 0;
    size_t speechEnd = 0;
    
    const size_t windowSize = 480; // 30ms at 16kHz
    for (size_t i = 0; i < testAudio.size() - windowSize; i += windowSize) {
        float energy = 0.0f;
        for (size_t j = 0; j < windowSize; ++j) {
            energy += testAudio[i + j] * testAudio[i + j];
        }
        energy = std::sqrt(energy / windowSize);
        
        bool isSpeech = energy > settings->getVadThreshold() * 0.1f;
        
        if (isSpeech && !speechDetected) {
            speechDetected = true;
            speechStart = i;
        } else if (!isSpeech && speechDetected) {
            speechEnd = i;
            break;
        }
    }
    
    // Verify VAD detected speech region
    EXPECT_TRUE(speechDetected);
    EXPECT_GT(speechEnd, speechStart);
    EXPECT_GE(speechStart, silence1.size() - windowSize);
    EXPECT_LE(speechEnd, silence1.size() + speech.size() + windowSize);
}

// Test audio format conversion
TEST_F(RecordingIntegrationTest, AudioFormatConversion) {
    // Create test audio
    std::vector<float> testAudio = TestUtils::AudioGenerator::generateSineWave(440.0f, 1.0f);
    
    // Convert to different formats
    AudioFormat sourceFormat;
    sourceFormat.sampleRate = 16000;
    sourceFormat.channels = 1;
    sourceFormat.bitsPerSample = 32;
    sourceFormat.isFloat = true;
    
    AudioFormat targetFormat;
    targetFormat.sampleRate = 16000;
    targetFormat.channels = 1;
    targetFormat.bitsPerSample = 16;
    targetFormat.isFloat = false;
    
    // Convert float to int16
    std::vector<int16_t> convertedAudio;
    auto result = audioConverter->convert(
        testAudio.data(),
        testAudio.size() * sizeof(float),
        sourceFormat,
        convertedAudio,
        targetFormat
    );
    
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_EQ(convertedAudio.size(), testAudio.size());
    
    // Verify conversion (check range)
    for (const auto& sample : convertedAudio) {
        EXPECT_GE(sample, -32768);
        EXPECT_LE(sample, 32767);
    }
}

// Test recording with file output
TEST_F(RecordingIntegrationTest, RecordingToFile) {
    // Create test audio
    std::vector<float> testAudio = TestUtils::AudioGenerator::generateSineWave(440.0f, 2.0f, 16000);
    
    // Save to WAV file
    std::string outputPath = testDataDir + "/test_recording.wav";
    
    AudioFormat format;
    format.sampleRate = 16000;
    format.channels = 1;
    format.bitsPerSample = 16;
    format.isFloat = false;
    
    // Convert to int16
    std::vector<int16_t> audioData;
    for (float sample : testAudio) {
        int16_t intSample = static_cast<int16_t>(sample * 32767.0f);
        audioData.push_back(intSample);
    }
    
    // Write WAV file
    auto result = audioConverter->saveToWav(audioData, format, outputPath);
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Verify file exists
    EXPECT_TRUE(std::filesystem::exists(outputPath));
    EXPECT_GT(std::filesystem::file_size(outputPath), 44); // WAV header + data
}

// Test multi-channel recording
TEST_F(RecordingIntegrationTest, MultiChannelRecording) {
    // Get devices that support stereo
    auto devices = deviceManager->getInputDevices();
    
    DeviceInfo* stereoDevice = nullptr;
    for (auto& device : devices) {
        if (device.channels >= 2) {
            stereoDevice = &device;
            break;
        }
    }
    
    if (!stereoDevice) {
        GTEST_SKIP() << "No stereo input devices available";
    }
    
    // Initialize with stereo format
    AudioFormat format;
    format.sampleRate = 16000;
    format.channels = 2;
    format.bitsPerSample = 16;
    
    auto result = audioCapture->initialize(stereoDevice->id, format);
    if (result != ErrorCode::Success) {
        GTEST_SKIP() << "Failed to initialize stereo recording";
    }
    
    // Verify stereo format
    EXPECT_EQ(audioCapture->getFormat().channels, 2);
}

// Test recording error handling
TEST_F(RecordingIntegrationTest, RecordingErrorHandling) {
    // Test with invalid device
    AudioFormat format;
    format.sampleRate = 16000;
    format.channels = 1;
    format.bitsPerSample = 16;
    
    auto result = audioCapture->initialize("invalid_device_id", format);
    EXPECT_NE(result, ErrorCode::Success);
    
    // Test starting without initialization
    AudioCapture uninitCapture;
    result = uninitCapture.start();
    EXPECT_NE(result, ErrorCode::Success);
    
    // Test invalid audio format
    auto devices = deviceManager->getInputDevices();
    if (!devices.empty()) {
        format.sampleRate = 0; // Invalid
        result = audioCapture->initialize(devices[0].id, format);
        EXPECT_NE(result, ErrorCode::Success);
    }
}

// Test recording performance
TEST_F(RecordingIntegrationTest, RecordingPerformance) {
    auto devices = deviceManager->getInputDevices();
    if (devices.empty()) {
        GTEST_SKIP() << "No input devices available";
    }
    
    AudioFormat format;
    format.sampleRate = 48000; // High sample rate
    format.channels = 1;
    format.bitsPerSample = 16;
    
    audioCapture->initialize(devices[0].id, format);
    
    // Measure callback latency
    std::vector<uint64_t> latencies;
    auto lastTime = std::chrono::steady_clock::now();
    
    audioCapture->setAudioCallback([&latencies, &lastTime](const float* data, size_t samples) {
        auto now = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count();
        latencies.push_back(latency);
        lastTime = now;
    });
    
    // Record for a short time
    audioCapture->start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    audioCapture->stop();
    
    // Analyze latencies
    if (!latencies.empty()) {
        uint64_t totalLatency = 0;
        uint64_t maxLatency = 0;
        
        for (auto latency : latencies) {
            totalLatency += latency;
            maxLatency = std::max(maxLatency, latency);
        }
        
        uint64_t avgLatency = totalLatency / latencies.size();
        
        // Latency should be reasonable (< 100ms)
        EXPECT_LT(avgLatency, 100000); // 100ms in microseconds
        EXPECT_LT(maxLatency, 200000); // 200ms max
    }
}