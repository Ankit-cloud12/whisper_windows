/*
 * AudioUtilsTest.cpp
 * 
 * Unit tests for AudioUtils functionality
 */

#include "../../src/core/AudioUtils.h"
#include "../TestUtils.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <algorithm>

// Simple test framework macros (same as AudioCaptureTest)
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

#define EXPECT_NEAR(a, b, tolerance) \
    if (std::abs((a) - (b)) > (tolerance)) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #a " near " #b << " (tolerance: " << tolerance << ")" << std::endl; \
        std::cerr << "  Actual: " << (a) << " vs " << (b) << std::endl; \
        assert(false); \
    }

TEST(AudioUtils, CalculateRMS) {
    // Test with silence
    std::vector<float> silence(1000, 0.0f);
    float rms = AudioUtils::calculateRMS(silence.data(), silence.size());
    EXPECT_NEAR(rms, 0.0f, 0.001f);
    
    // Test with sine wave
    auto sine = TestUtils::AudioGenerator::generateSineWave(440.0f, 0.1f, 16000, 0.5f);
    rms = AudioUtils::calculateRMS(sine.data(), sine.size());
    // RMS of sine wave with amplitude 0.5 should be approximately 0.5 / sqrt(2)
    EXPECT_NEAR(rms, 0.5f / std::sqrt(2.0f), 0.01f);
    
    // Test with empty buffer
    rms = AudioUtils::calculateRMS(nullptr, 0);
    EXPECT_EQ(rms, 0.0f);
}

TEST(AudioUtils, CalculatePeak) {
    // Test with silence
    std::vector<float> silence(1000, 0.0f);
    float peak = AudioUtils::calculatePeak(silence.data(), silence.size());
    EXPECT_EQ(peak, 0.0f);
    
    // Test with known values
    std::vector<float> samples = {0.1f, -0.5f, 0.3f, -0.8f, 0.2f};
    peak = AudioUtils::calculatePeak(samples.data(), samples.size());
    EXPECT_EQ(peak, 0.8f);
    
    // Test with sine wave
    auto sine = TestUtils::AudioGenerator::generateSineWave(440.0f, 0.1f, 16000, 0.7f);
    peak = AudioUtils::calculatePeak(sine.data(), sine.size());
    EXPECT_NEAR(peak, 0.7f, 0.01f);
}

TEST(AudioUtils, CalculateStats) {
    auto sine = TestUtils::AudioGenerator::generateSineWave(440.0f, 0.1f, 16000, 0.5f);
    auto stats = AudioUtils::calculateStats(sine.data(), sine.size());
    
    // Check RMS
    EXPECT_NEAR(stats.rms, 0.5f / std::sqrt(2.0f), 0.01f);
    
    // Check peak
    EXPECT_NEAR(stats.peak, 0.5f, 0.01f);
    
    // Check crest factor (peak / RMS)
    float expected_crest = std::sqrt(2.0f);
    EXPECT_NEAR(stats.crest_factor, expected_crest, 0.1f);
    
    // Check zero crossing rate (should be around 2 * frequency / sample_rate)
    float expected_zcr = 2.0f * 440.0f / 16000.0f;
    EXPECT_NEAR(stats.zero_crossings, expected_zcr, 0.05f);
}

TEST(AudioUtils, DetectSilence) {
    // Test actual silence
    std::vector<float> silence(1000, 0.0f);
    bool is_silent = AudioUtils::detectSilence(silence.data(), silence.size(), 0.01f, 100);
    EXPECT_TRUE(is_silent);
    
    // Test loud signal
    auto loud = TestUtils::AudioGenerator::generateSineWave(440.0f, 0.1f, 16000, 0.8f);
    is_silent = AudioUtils::detectSilence(loud.data(), loud.size(), 0.01f, 100);
    EXPECT_FALSE(is_silent);
    
    // Test quiet signal below threshold
    auto quiet = TestUtils::AudioGenerator::generateSineWave(440.0f, 0.1f, 16000, 0.005f);
    is_silent = AudioUtils::detectSilence(quiet.data(), quiet.size(), 0.01f, 100);
    EXPECT_TRUE(is_silent);
}

TEST(AudioUtils, ApplyNoiseGate) {
    // Generate test signal with quiet and loud parts
    std::vector<float> signal;
    
    // Add quiet noise
    auto noise = TestUtils::AudioGenerator::generateWhiteNoise(0.1f, 16000, 0.005f);
    signal.insert(signal.end(), noise.begin(), noise.end());
    
    // Add loud signal
    auto loud = TestUtils::AudioGenerator::generateSineWave(440.0f, 0.1f, 16000, 0.5f);
    signal.insert(signal.end(), loud.begin(), loud.end());
    
    // Add quiet noise again
    signal.insert(signal.end(), noise.begin(), noise.end());
    
    // Apply noise gate
    AudioUtils::applyNoiseGate(signal.data(), signal.size(), 0.01f, 10, 100);
    
    // Check that quiet parts are reduced
    float quiet_rms = AudioUtils::calculateRMS(signal.data(), noise.size());
    EXPECT_TRUE(quiet_rms < 0.005f);
}

TEST(AudioUtils, ReduceNoise) {
    // Generate noisy signal
    auto clean = TestUtils::AudioGenerator::generateSineWave(440.0f, 0.5f, 16000, 0.5f);
    auto noise = TestUtils::AudioGenerator::generateWhiteNoise(0.5f, 16000, 0.1f);
    
    // Mix signals manually
    std::vector<float> noisy(clean.size());
    for (size_t i = 0; i < clean.size(); ++i) {
        noisy[i] = clean[i] + noise[i];
    }
    
    // Apply noise reduction
    AudioUtils::reduceNoise(noisy.data(), noisy.size(), 0.05f, 0.5f);
    
    // Signal should be cleaner (lower RMS difference from original)
    float original_rms = AudioUtils::calculateRMS(clean.data(), clean.size());
    float processed_rms = AudioUtils::calculateRMS(noisy.data(), noisy.size());
    
    // Processed signal should be closer to original amplitude
    EXPECT_TRUE(std::abs(processed_rms - original_rms) < 0.2f);
}

TEST(AudioUtils, Resample) {
    // Test upsampling
    auto original = TestUtils::AudioGenerator::generateSineWave(100.0f, 0.1f, 8000, 0.5f);
    auto upsampled = AudioUtils::resample(original.data(), original.size(), 8000, 16000);
    
    // Should have approximately twice as many samples
    EXPECT_EQ(upsampled.size(), original.size() * 2);
    
    // Test downsampling
    auto downsampled = AudioUtils::resample(original.data(), original.size(), 8000, 4000);
    
    // Should have approximately half as many samples
    EXPECT_EQ(downsampled.size(), original.size() / 2);
    
    // Test same sample rate (should return copy)
    auto same = AudioUtils::resample(original.data(), original.size(), 8000, 8000);
    EXPECT_EQ(same.size(), original.size());
}

TEST(AudioUtils, StereoToMono) {
    // Create stereo signal
    std::vector<float> stereo;
    for (int i = 0; i < 100; ++i) {
        stereo.push_back(0.5f);  // Left channel
        stereo.push_back(0.3f);  // Right channel
    }
    
    auto mono = AudioUtils::stereoToMono(stereo.data(), 100);
    
    EXPECT_EQ(mono.size(), 100u);
    
    // Each mono sample should be average of L and R
    for (size_t i = 0; i < mono.size(); ++i) {
        EXPECT_NEAR(mono[i], 0.4f, 0.001f);  // (0.5 + 0.3) / 2
    }
}

TEST(AudioUtils, Normalize) {
    std::vector<float> signal = {0.1f, -0.2f, 0.3f, -0.4f, 0.5f};
    
    AudioUtils::normalize(signal.data(), signal.size(), 0.95f);
    
    // Peak should now be 0.95
    float peak = AudioUtils::calculatePeak(signal.data(), signal.size());
    EXPECT_NEAR(peak, 0.95f, 0.001f);
    
    // Test with already normalized signal
    AudioUtils::normalize(signal.data(), signal.size(), 0.95f);
    peak = AudioUtils::calculatePeak(signal.data(), signal.size());
    EXPECT_NEAR(peak, 0.95f, 0.001f);
}

TEST(AudioUtils, FadeInOut) {
    // Test fade in
    std::vector<float> signal(1000, 0.5f);
    AudioUtils::fadeIn(signal.data(), signal.size(), 100);
    
    // First sample should be zero
    EXPECT_NEAR(signal[0], 0.0f, 0.001f);
    
    // Sample at fade end should be original value
    EXPECT_NEAR(signal[100], 0.5f, 0.05f);
    
    // Test fade out
    std::fill(signal.begin(), signal.end(), 0.5f);
    AudioUtils::fadeOut(signal.data(), signal.size(), 100);
    
    // Last sample should be zero
    EXPECT_NEAR(signal[999], 0.0f, 0.001f);
    
    // Sample before fade should be original value
    EXPECT_NEAR(signal[899], 0.5f, 0.05f);
}

TEST(AudioUtils, WavHeader) {
    auto header = AudioUtils::createWavHeader(44100, 2, 16, 88200);
    
    EXPECT_EQ(std::string(header.riff, 4), "RIFF");
    EXPECT_EQ(std::string(header.wave, 4), "WAVE");
    EXPECT_EQ(std::string(header.fmt, 4), "fmt ");
    EXPECT_EQ(std::string(header.data, 4), "data");
    
    EXPECT_EQ(header.sample_rate, 44100u);
    EXPECT_EQ(header.channels, 2);
    EXPECT_EQ(header.bits_per_sample, 16);
    EXPECT_EQ(header.data_size, 88200u);
    EXPECT_EQ(header.byte_rate, 44100u * 2 * 2);  // sample_rate * channels * bytes_per_sample
}

TEST(AudioUtils, SaveLoadWav) {
    // Generate test audio
    auto original = TestUtils::AudioGenerator::generateSineWave(440.0f, 0.5f, 16000, 0.5f);
    
    // Save to temporary file
    std::string filename = TestUtils::FileUtils::createTempDirectory() + "/test.wav";
    bool saved = AudioUtils::saveWav(filename, original.data(), original.size(), 16000, 1);
    EXPECT_TRUE(saved);
    
    // Load back
    uint32_t sample_rate;
    uint16_t channels;
    auto loaded = AudioUtils::loadWav(filename, sample_rate, channels);
    
    EXPECT_EQ(sample_rate, 16000u);
    EXPECT_EQ(channels, 1);
    EXPECT_EQ(loaded.size(), original.size());
    
    // Clean up
    TestUtils::FileUtils::cleanupTempDirectory(filename);
}

TEST(AudioUtils, PreEmphasis) {
    std::vector<float> signal = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
    std::vector<float> original = signal;  // Copy for comparison
    
    AudioUtils::preEmphasis(signal.data(), signal.size(), 0.97f);
    
    // Check pre-emphasis formula: y[n] = x[n] - coeff * x[n-1]
    for (size_t i = 1; i < signal.size(); ++i) {
        float expected = original[i] - 0.97f * original[i-1];
        EXPECT_NEAR(signal[i], expected, 0.001f);
    }
}

TEST(AudioUtils, ZeroCrossingRate) {
    // Test with DC signal (no crossings)
    std::vector<float> dc(100, 0.5f);
    float zcr = AudioUtils::calculateZeroCrossingRate(dc.data(), dc.size());
    EXPECT_EQ(zcr, 0.0f);
    
    // Test with alternating signal (maximum crossings)
    std::vector<float> alternating;
    for (int i = 0; i < 100; ++i) {
        alternating.push_back(i % 2 == 0 ? 0.5f : -0.5f);
    }
    zcr = AudioUtils::calculateZeroCrossingRate(alternating.data(), alternating.size());
    EXPECT_NEAR(zcr, 1.0f, 0.01f);  // Nearly every sample crosses zero
    
    // Test with sine wave
    auto sine = TestUtils::AudioGenerator::generateSineWave(100.0f, 0.1f, 16000, 0.5f);
    zcr = AudioUtils::calculateZeroCrossingRate(sine.data(), sine.size());
    // Expected ZCR for sine wave: 2 * frequency / sample_rate
    float expected_zcr = 2.0f * 100.0f / 16000.0f;
    EXPECT_NEAR(zcr, expected_zcr, 0.01f);
}

TEST(AudioUtils, VoiceActivityDetection) {
    // Create test signal with speech-like and silence sections
    std::vector<float> signal;
    
    // Add silence
    auto silence = TestUtils::AudioGenerator::generateSilence(0.1f, 16000);
    signal.insert(signal.end(), silence.begin(), silence.end());
    
    // Add "speech" (sine wave with varying frequency)
    auto speech = TestUtils::AudioGenerator::generateSineWave(300.0f, 0.2f, 16000, 0.3f);
    signal.insert(signal.end(), speech.begin(), speech.end());
    
    // Add more silence
    signal.insert(signal.end(), silence.begin(), silence.end());
    
    auto vad = AudioUtils::detectVoiceActivity(signal.data(), signal.size(), 256, 0.01f, 0.5f);
    
    // Should detect silence at beginning and end, voice in middle
    EXPECT_FALSE(vad.empty());
    
    // First few frames should be silence
    EXPECT_FALSE(vad[0]);
    
    // Middle frames should have voice activity
    bool found_voice = false;
    for (size_t i = vad.size() / 3; i < 2 * vad.size() / 3; ++i) {
        if (vad[i]) {
            found_voice = true;
            break;
        }
    }
    EXPECT_TRUE(found_voice);
}

TEST(AudioUtils, RemoveDCOffset) {
    // Create signal with DC offset
    std::vector<float> signal;
    for (int i = 0; i < 1000; ++i) {
        signal.push_back(0.5f + 0.1f * std::sin(2.0f * M_PI * 100.0f * i / 16000.0f));
    }
    
    // Calculate DC before
    float dc_before = 0.0f;
    for (float s : signal) {
        dc_before += s;
    }
    dc_before /= signal.size();
    EXPECT_NEAR(dc_before, 0.5f, 0.01f);
    
    // Remove DC offset
    AudioUtils::removeDCOffset(signal.data(), signal.size(), 80.0f, 16000.0f);
    
    // Calculate DC after
    float dc_after = 0.0f;
    for (float s : signal) {
        dc_after += s;
    }
    dc_after /= signal.size();
    
    // DC offset should be significantly reduced
    EXPECT_TRUE(std::abs(dc_after) < std::abs(dc_before) * 0.1f);
}

TEST(AudioUtils, ClipAudio) {
    std::vector<float> signal = {0.5f, 1.5f, -0.8f, -1.2f, 0.9f, 2.0f};
    
    size_t clipped = AudioUtils::clipAudio(signal.data(), signal.size(), 1.0f);
    
    // Should have clipped 3 samples (1.5, -1.2, 2.0)
    EXPECT_EQ(clipped, 3u);
    
    // Check all values are within range
    for (float s : signal) {
        EXPECT_TRUE(s >= -1.0f && s <= 1.0f);
    }
    
    // Check specific clipped values
    EXPECT_EQ(signal[1], 1.0f);   // Was 1.5
    EXPECT_EQ(signal[3], -1.0f);  // Was -1.2
    EXPECT_EQ(signal[5], 1.0f);   // Was 2.0
}

// Main function
int main() {
    std::cout << "Running AudioUtils tests..." << std::endl;
    std::cout << "All tests completed!" << std::endl;
    return 0;
}