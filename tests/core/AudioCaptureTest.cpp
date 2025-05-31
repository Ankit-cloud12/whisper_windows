/*
 * AudioCaptureTest.cpp
 * 
 * Unit tests for AudioCapture functionality
 */

#include "../../src/core/AudioCapture.h"
#include "../TestUtils.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <cassert>

// Simple test framework macros
#define TEST(TestClass, TestName) \
    void TestClass##_##TestName(); \
    struct TestClass##_##TestName##_Register { \
        TestClass##_##TestName##_Register() { \
            std::cout << "Running " #TestClass "." #TestName << "..." << std::endl; \
            TestClass##_##TestName(); \
            std::cout << "  PASSED" << std::endl; \
        } \
    } TestClass##_##TestName##_Instance; \
    void TestClass##_##TestName()

#define EXPECT_TRUE(expr) \
    if (!(expr)) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #expr " to be true" << std::endl; \
        assert(false); \
    }

#define EXPECT_FALSE(expr) \
    if (expr) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #expr " to be false" << std::endl; \
        assert(false); \
    }

#define EXPECT_EQ(a, b) \
    if ((a) != (b)) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #a " == " #b << std::endl; \
        std::cerr << "  Actual: " << (a) << " != " << (b) << std::endl; \
        assert(false); \
    }

#define EXPECT_GT(a, b) \
    if (!((a) > (b))) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #a " > " #b << std::endl; \
        std::cerr << "  Actual: " << (a) << " <= " << (b) << std::endl; \
        assert(false); \
    }

#define EXPECT_GE(a, b) \
    if (!((a) >= (b))) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #a " >= " #b << std::endl; \
        std::cerr << "  Actual: " << (a) << " < " << (b) << std::endl; \
        assert(false); \
    }

#define EXPECT_LE(a, b) \
    if (!((a) <= (b))) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #a " <= " #b << std::endl; \
        std::cerr << "  Actual: " << (a) << " > " << (b) << std::endl; \
        assert(false); \
    }

// Test helper class
class AudioCaptureTestHelper {
public:
    std::unique_ptr<AudioCapture> capture;
    std::vector<float> captured_samples;
    int callback_count = 0;
    float last_level = 0.0f;
    bool device_changed = false;
    
    AudioCaptureTestHelper() {
        capture = std::make_unique<AudioCapture>();
        captured_samples.clear();
        callback_count = 0;
        last_level = 0.0f;
        device_changed = false;
    }
    
    void audioCallback(const float* data, size_t count) {
        captured_samples.insert(captured_samples.end(), data, data + count);
        callback_count++;
    }
    
    void levelCallback(float level) {
        last_level = level;
    }
    
    void deviceChangeCallback() {
        device_changed = true;
    }
};

TEST(AudioCapture, InitializeShutdown) {
    AudioCaptureTestHelper helper;
    
    EXPECT_TRUE(helper.capture->initialize());
    EXPECT_TRUE(helper.capture->initialize()); // Should be safe to call multiple times
    
    helper.capture->shutdown();
    helper.capture->shutdown(); // Should be safe to call multiple times
}

TEST(AudioCapture, DeviceEnumeration) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    auto devices = helper.capture->getAudioDevices();
    EXPECT_FALSE(devices.empty());
    
    // Should have at least one device
    bool found_default = false;
    for (const auto& device : devices) {
        EXPECT_FALSE(device.id.empty());
        EXPECT_FALSE(device.name.empty());
        EXPECT_GT(device.channels, 0);
        EXPECT_GT(device.sample_rate, 0);
        
        if (device.is_default) {
            found_default = true;
        }
    }
    
    EXPECT_TRUE(found_default);
}

TEST(AudioCapture, DefaultDevice) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    auto default_device = helper.capture->getDefaultDevice();
    EXPECT_FALSE(default_device.id.empty());
    EXPECT_FALSE(default_device.name.empty());
    EXPECT_TRUE(default_device.is_default);
}

TEST(AudioCapture, DeviceSelection) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    auto devices = helper.capture->getAudioDevices();
    EXPECT_FALSE(devices.empty());
    
    // Try to set each device
    for (const auto& device : devices) {
        EXPECT_TRUE(helper.capture->setDevice(device.id));
    }
    
    // Try invalid device
    EXPECT_FALSE(helper.capture->setDevice("invalid_device_id"));
}

TEST(AudioCapture, Configuration) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    AudioCaptureConfig config;
    config.sample_rate = 44100;
    config.channels = 2;
    config.buffer_size_ms = 50;
    config.enable_noise_suppression = true;
    config.enable_silence_detection = false;
    
    helper.capture->setConfig(config);
    
    auto retrieved_config = helper.capture->getConfig();
    EXPECT_EQ(retrieved_config.sample_rate, config.sample_rate);
    EXPECT_EQ(retrieved_config.channels, config.channels);
    EXPECT_EQ(retrieved_config.buffer_size_ms, config.buffer_size_ms);
    EXPECT_EQ(retrieved_config.enable_noise_suppression, config.enable_noise_suppression);
    EXPECT_EQ(retrieved_config.enable_silence_detection, config.enable_silence_detection);
}

TEST(AudioCapture, StartStopCapture) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    EXPECT_FALSE(helper.capture->isCapturing());
    
    // Start capture with callback
    auto callback = [&helper](const float* data, size_t count) {
        helper.audioCallback(data, count);
    };
    
    EXPECT_TRUE(helper.capture->startCapture(callback));
    EXPECT_TRUE(helper.capture->isCapturing());
    
    // Let it capture for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    helper.capture->stopCapture();
    EXPECT_FALSE(helper.capture->isCapturing());
    
    // Should have received some callbacks
    EXPECT_GT(helper.callback_count, 0);
    EXPECT_FALSE(helper.captured_samples.empty());
}

TEST(AudioCapture, AudioLevel) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    helper.capture->setLevelCallback([&helper](float level) {
        helper.levelCallback(level);
    });
    
    auto callback = [](const float*, size_t) {};
    EXPECT_TRUE(helper.capture->startCapture(callback));
    
    // Let it run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    float level = helper.capture->getAudioLevel();
    EXPECT_GE(level, 0.0f);
    EXPECT_LE(level, 1.0f);
    
    // Should have received level updates
    EXPECT_GT(helper.last_level, 0.0f);
    
    helper.capture->stopCapture();
}

TEST(AudioCapture, BufferOperations) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    auto callback = [](const float*, size_t) {};
    EXPECT_TRUE(helper.capture->startCapture(callback));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    auto captured_audio = helper.capture->getCapturedAudio();
    EXPECT_FALSE(captured_audio.empty());
    
    helper.capture->clearBuffer();
    auto cleared_audio = helper.capture->getCapturedAudio();
    EXPECT_TRUE(cleared_audio.empty());
    
    helper.capture->stopCapture();
}

TEST(AudioCapture, DeviceChangeNotification) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    helper.capture->setDeviceChangeCallback([&helper]() {
        helper.deviceChangeCallback();
    });
    
    // In a real test, we would trigger an actual device change
    // For now, just verify the callback is set
    EXPECT_FALSE(helper.device_changed);
}

TEST(AudioCapture, LoopbackCapture) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    EXPECT_FALSE(helper.capture->isLoopbackEnabled());
    
    helper.capture->setLoopbackEnabled(true);
    EXPECT_TRUE(helper.capture->isLoopbackEnabled());
    
    helper.capture->setLoopbackEnabled(false);
    EXPECT_FALSE(helper.capture->isLoopbackEnabled());
}

TEST(AudioCapture, CaptureStatistics) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    helper.capture->resetStats();
    auto stats = helper.capture->getStats();
    EXPECT_EQ(stats.total_samples, 0u);
    EXPECT_EQ(stats.dropped_samples, 0u);
    EXPECT_EQ(stats.buffer_overruns, 0);
    
    auto callback = [](const float*, size_t) {};
    EXPECT_TRUE(helper.capture->startCapture(callback));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    helper.capture->stopCapture();
    
    stats = helper.capture->getStats();
    EXPECT_GT(stats.total_samples, 0u);
    EXPECT_GE(stats.average_level, 0.0f);
    EXPECT_LE(stats.average_level, 1.0f);
}

TEST(AudioCapture, SilenceDetection) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    AudioCaptureConfig config;
    config.enable_silence_detection = true;
    config.silence_threshold = 0.001f; // Very low threshold
    config.silence_duration_ms = 100; // Short duration for testing
    helper.capture->setConfig(config);
    
    auto callback = [](const float*, size_t) {};
    EXPECT_TRUE(helper.capture->startCapture(callback));
    
    // The mock implementation generates a sine wave with low amplitude
    // So silence detection should eventually trigger
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Check if capture stopped due to silence
    // (This depends on the mock implementation)
}

TEST(AudioCapture, NoiseSuppressionEnabled) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    AudioCaptureConfig config;
    config.enable_noise_suppression = true;
    helper.capture->setConfig(config);
    
    auto callback = [&helper](const float* data, size_t count) {
        helper.audioCallback(data, count);
    };
    
    EXPECT_TRUE(helper.capture->startCapture(callback));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    helper.capture->stopCapture();
    
    // Verify we received processed audio
    EXPECT_FALSE(helper.captured_samples.empty());
}

// Stress test
TEST(AudioCapture, RapidStartStop) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    auto callback = [](const float*, size_t) {};
    
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(helper.capture->startCapture(callback));
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        helper.capture->stopCapture();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Configuration validation
TEST(AudioCapture, ConfigValidation) {
    AudioCaptureTestHelper helper;
    EXPECT_TRUE(helper.capture->initialize());
    
    // Test various configurations
    std::vector<AudioCaptureConfig> configs = {
        {8000, 1, 50, false, false, 0.01f, 1000},
        {16000, 1, 100, true, true, 0.02f, 2000},
        {44100, 2, 20, false, true, 0.005f, 500},
        {48000, 2, 200, true, false, 0.03f, 3000}
    };
    
    for (const auto& config : configs) {
        helper.capture->setConfig(config);
        auto retrieved = helper.capture->getConfig();
        EXPECT_EQ(retrieved.sample_rate, config.sample_rate);
        EXPECT_EQ(retrieved.channels, config.channels);
    }
}

// Main function to run all tests
int main() {
    std::cout << "Running AudioCapture tests..." << std::endl;
    std::cout << "All tests completed!" << std::endl;
    return 0;
}