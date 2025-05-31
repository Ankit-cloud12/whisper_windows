/*
 * AudioConverterTest.cpp
 * 
 * Unit tests for AudioConverter class.
 */

#include <gtest/gtest.h>
#include "../TestUtils.h"
#include "core/AudioConverter.h"
#include "core/ErrorCodes.h"
#include <cmath>

using namespace WhisperApp;
using namespace TestUtils;

class AudioConverterTest : public ::testing::Test {
protected:
    std::unique_ptr<AudioConverter> converter;
    
    void SetUp() override {
        converter = std::make_unique<AudioConverter>();
    }
    
    void TearDown() override {
        converter.reset();
    }
    
    // Helper function to check if audio buffers are similar
    bool areBuffersSimilar(const std::vector<float>& a, const std::vector<float>& b, float tolerance = 0.01f) {
        if (a.size() != b.size()) return false;
        
        for (size_t i = 0; i < a.size(); ++i) {
            if (std::abs(a[i] - b[i]) > tolerance) {
                return false;
            }
        }
        return true;
    }
};

// Format conversion tests

TEST_F(AudioConverterTest, Float32ToInt16Conversion) {
    // Create test data
    std::vector<float> floatData = {0.0f, 0.5f, 1.0f, -0.5f, -1.0f};
    AudioFormat format(16000, 1, 16, false);
    
    // Convert to int16
    auto rawData = AudioConverter::fromFloat32(floatData, format);
    
    // Convert back to float32
    auto convertedBack = AudioConverter::toFloat32(rawData, format);
    
    // Check that values are preserved (with some tolerance for quantization)
    ASSERT_EQ(floatData.size(), convertedBack.size());
    for (size_t i = 0; i < floatData.size(); ++i) {
        EXPECT_NEAR(floatData[i], convertedBack[i], 1.0f / 32768.0f);
    }
}

TEST_F(AudioConverterTest, Float32ToInt24Conversion) {
    // Create test data
    std::vector<float> floatData = {0.0f, 0.25f, 0.75f, -0.25f, -0.75f};
    AudioFormat format(16000, 1, 24, false);
    
    // Convert to int24
    auto rawData = AudioConverter::fromFloat32(floatData, format);
    
    // Check size
    EXPECT_EQ(rawData.size(), floatData.size() * 3);  // 3 bytes per sample
    
    // Convert back to float32
    auto convertedBack = AudioConverter::toFloat32(rawData, format);
    
    // Check that values are preserved
    ASSERT_EQ(floatData.size(), convertedBack.size());
    for (size_t i = 0; i < floatData.size(); ++i) {
        EXPECT_NEAR(floatData[i], convertedBack[i], 1.0f / 8388608.0f);
    }
}

TEST_F(AudioConverterTest, Float32ToUInt8Conversion) {
    // Create test data
    std::vector<float> floatData = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
    AudioFormat format(16000, 1, 8, false);
    
    // Convert to uint8
    auto rawData = AudioConverter::fromFloat32(floatData, format);
    
    // Convert back to float32
    auto convertedBack = AudioConverter::toFloat32(rawData, format);
    
    // Check that values are preserved (with tolerance for 8-bit quantization)
    ASSERT_EQ(floatData.size(), convertedBack.size());
    for (size_t i = 0; i < floatData.size(); ++i) {
        EXPECT_NEAR(floatData[i], convertedBack[i], 1.0f / 128.0f);
    }
}

// Channel conversion tests

TEST_F(AudioConverterTest, StereoToMonoConversion) {
    // Create stereo signal with different content in each channel
    std::vector<float> stereo;
    for (int i = 0; i < 100; ++i) {
        stereo.push_back(0.5f);   // Left channel
        stereo.push_back(-0.5f);  // Right channel
    }
    
    // Convert to mono
    auto mono = AudioConverter::stereoToMono(stereo);
    
    // Check size
    EXPECT_EQ(mono.size(), stereo.size() / 2);
    
    // Check that mono is average of stereo channels
    for (size_t i = 0; i < mono.size(); ++i) {
        float expected = (stereo[i * 2] + stereo[i * 2 + 1]) / 2.0f;
        EXPECT_FLOAT_EQ(mono[i], expected);
    }
}

TEST_F(AudioConverterTest, MonoToStereoConversion) {
    // Create mono signal
    std::vector<float> mono = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
    
    // Convert to stereo
    auto stereo = AudioConverter::monoToStereo(mono);
    
    // Check size
    EXPECT_EQ(stereo.size(), mono.size() * 2);
    
    // Check that both channels have same content
    for (size_t i = 0; i < mono.size(); ++i) {
        EXPECT_FLOAT_EQ(stereo[i * 2], mono[i]);      // Left
        EXPECT_FLOAT_EQ(stereo[i * 2 + 1], mono[i]);  // Right
    }
}

// Resampling tests

TEST_F(AudioConverterTest, UpsamplingTest) {
    // Create a simple signal at 8kHz
    int inputRate = 8000;
    int outputRate = 16000;
    std::vector<float> input = AudioGenerator::generateSineWave(440.0f, 0.1f, inputRate);
    
    // Upsample to 16kHz
    auto output = AudioConverter::resample(input, inputRate, outputRate);
    
    // Check that output has approximately double the samples
    float expectedRatio = static_cast<float>(outputRate) / inputRate;
    float actualRatio = static_cast<float>(output.size()) / input.size();
    EXPECT_NEAR(actualRatio, expectedRatio, 0.01f);
}

TEST_F(AudioConverterTest, DownsamplingTest) {
    // Create a signal at 48kHz
    int inputRate = 48000;
    int outputRate = 16000;
    std::vector<float> input = AudioGenerator::generateSineWave(440.0f, 0.1f, inputRate);
    
    // Downsample to 16kHz
    auto output = AudioConverter::resample(input, inputRate, outputRate);
    
    // Check that output has approximately 1/3 the samples
    float expectedRatio = static_cast<float>(outputRate) / inputRate;
    float actualRatio = static_cast<float>(output.size()) / input.size();
    EXPECT_NEAR(actualRatio, expectedRatio, 0.01f);
}

TEST_F(AudioConverterTest, NoResamplingNeeded) {
    // Create a signal at 16kHz
    int sampleRate = 16000;
    std::vector<float> input = AudioGenerator::generateWhiteNoise(0.1f, sampleRate);
    
    // "Resample" to same rate
    auto output = AudioConverter::resample(input, sampleRate, sampleRate);
    
    // Should return the same data
    EXPECT_EQ(output.size(), input.size());
    EXPECT_TRUE(areBuffersSimilar(input, output));
}

// Audio processing tests

TEST_F(AudioConverterTest, NormalizationTest) {
    // Create a quiet signal
    std::vector<float> quiet = {0.1f, -0.1f, 0.05f, -0.05f, 0.15f};
    
    // Normalize
    auto normalized = AudioConverter::normalize(quiet);
    
    // Find peak in normalized signal
    float peak = 0.0f;
    for (float sample : normalized) {
        peak = std::max(peak, std::abs(sample));
    }
    
    // Peak should be close to target (0.95)
    EXPECT_NEAR(peak, 0.95f, 0.001f);
}

TEST_F(AudioConverterTest, DCOffsetRemoval) {
    // Create signal with DC offset
    std::vector<float> withDC;
    float dcOffset = 0.3f;
    for (int i = 0; i < 100; ++i) {
        withDC.push_back(std::sin(2.0f * M_PI * i / 20.0f) + dcOffset);
    }
    
    // Remove DC offset
    auto withoutDC = AudioConverter::removeDCOffset(withDC);
    
    // Calculate average (should be close to 0)
    float average = 0.0f;
    for (float sample : withoutDC) {
        average += sample;
    }
    average /= withoutDC.size();
    
    EXPECT_NEAR(average, 0.0f, 0.001f);
}

TEST_F(AudioConverterTest, DitheringTest) {
    // Create a smooth signal
    std::vector<float> smooth;
    for (int i = 0; i < 1000; ++i) {
        smooth.push_back(std::sin(2.0f * M_PI * i / 100.0f) * 0.5f);
    }
    
    // Apply dithering for 16-bit
    auto dithered = AudioConverter::applyDithering(smooth, 16);
    
    // Check that dithering added small noise
    ASSERT_EQ(smooth.size(), dithered.size());
    
    bool foundDifference = false;
    for (size_t i = 0; i < smooth.size(); ++i) {
        float diff = std::abs(smooth[i] - dithered[i]);
        if (diff > 0.0f) {
            foundDifference = true;
            // Dither should be small
            EXPECT_LT(diff, 1.0f / 32768.0f);
        }
    }
    
    EXPECT_TRUE(foundDifference);  // Dithering should have changed something
}

// Full conversion pipeline tests

TEST_F(AudioConverterTest, CompleteConversionPipeline) {
    // Create input buffer: 44.1kHz stereo 16-bit
    AudioBuffer input;
    input.format = AudioFormat(44100, 2, 16, false);
    
    // Generate stereo test signal
    auto leftChannel = AudioGenerator::generateSineWave(440.0f, 0.5f, 44100);
    auto rightChannel = AudioGenerator::generateSineWave(880.0f, 0.5f, 44100);
    
    // Interleave channels
    for (size_t i = 0; i < leftChannel.size(); ++i) {
        input.data.push_back(leftChannel[i]);
        input.data.push_back(rightChannel[i]);
    }
    
    // Convert to whisper format: 16kHz mono float32
    AudioConverter::ConversionParams params;
    params.targetFormat = AudioFormat(16000, 1, 32, true);
    params.normalizeAudio = true;
    params.removeDCOffset = true;
    
    AudioConverter::ConversionStats stats;
    auto output = converter->convert(input, params, &stats);
    
    // Check output format
    EXPECT_EQ(output.format.sampleRate, 16000);
    EXPECT_EQ(output.format.channels, 1);
    EXPECT_EQ(output.format.bitsPerSample, 32);
    EXPECT_TRUE(output.format.isFloat);
    
    // Check size (approximately)
    float expectedSizeRatio = (16000.0f / 44100.0f) * 0.5f;  // Downsample and mono
    float actualSizeRatio = static_cast<float>(output.data.size()) / input.data.size();
    EXPECT_NEAR(actualSizeRatio, expectedSizeRatio, 0.1f);
    
    // Check statistics
    EXPECT_GT(stats.peakLevel, 0.0f);
    EXPECT_GT(stats.averageLevel, 0.0f);
    EXPECT_GT(stats.processingTimeMs, 0);
}

// Audio splitting and merging tests

TEST_F(AudioConverterTest, SplitIntoChunks) {
    // Create 5 seconds of audio at 16kHz
    AudioBuffer buffer;
    buffer.format = AudioFormat(16000, 1, 32, true);
    buffer.data = AudioGenerator::generateWhiteNoise(5.0f, 16000);
    
    // Split into 1-second chunks with 0.1s overlap
    auto chunks = AudioConverter::splitIntoChunks(buffer, 1000, 100);
    
    // Check number of chunks
    EXPECT_EQ(chunks.size(), 6);  // 5 chunks + 1 partial
    
    // Check chunk sizes
    for (size_t i = 0; i < chunks.size() - 1; ++i) {
        EXPECT_EQ(chunks[i].data.size(), 16000);  // 1 second at 16kHz
    }
    
    // Check timestamps
    for (size_t i = 0; i < chunks.size(); ++i) {
        uint64_t expectedTimestamp = i * 900;  // 900ms stride (1000ms - 100ms overlap)
        EXPECT_EQ(chunks[i].timestamp_ms, expectedTimestamp);
    }
}

TEST_F(AudioConverterTest, MergeChunks) {
    // Create chunks
    std::vector<AudioBuffer> chunks;
    AudioFormat format(16000, 1, 32, true);
    
    for (int i = 0; i < 3; ++i) {
        AudioBuffer chunk;
        chunk.format = format;
        chunk.data = AudioGenerator::generateSineWave(440.0f + i * 100, 1.0f, 16000);
        chunk.timestamp_ms = i * 900;  // 100ms overlap
        chunks.push_back(chunk);
    }
    
    // Merge with 100ms overlap removal
    auto merged = AudioConverter::mergeChunks(chunks, 100);
    
    // Check size (3 seconds - 2 * 0.1s overlap)
    size_t expectedSize = 3 * 16000 - 2 * 1600;
    EXPECT_NEAR(merged.data.size(), expectedSize, 100);  // Allow small tolerance
}

// Error handling tests

TEST_F(AudioConverterTest, ConvertEmptyBuffer) {
    AudioBuffer empty;
    empty.format = AudioFormat(16000, 1, 16, false);
    
    // Should throw exception
    EXPECT_THROW({
        converter->convert(empty);
    }, AudioException);
}

TEST_F(AudioConverterTest, InvalidChannelConversion) {
    AudioBuffer input;
    input.format = AudioFormat(16000, 5, 16, false);  // 5 channels
    input.data.resize(16000 * 5);
    
    AudioConverter::ConversionParams params;
    params.targetFormat = AudioFormat(16000, 1, 16, false);
    
    // Should throw exception for unsupported channel count
    EXPECT_THROW({
        converter->convert(input, params);
    }, AudioException);
}

// File I/O tests (mock implementations)

TEST_F(AudioConverterTest, LoadFromFile) {
    // Test mock file loading
    auto buffer = AudioConverter::loadFromFile("test.wav");
    
    // Should return non-empty buffer
    EXPECT_FALSE(buffer.empty());
    EXPECT_GT(buffer.data.size(), 0);
}

TEST_F(AudioConverterTest, SaveToFile) {
    // Create test buffer
    AudioBuffer buffer;
    buffer.format = AudioFormat(16000, 1, 16, false);
    buffer.data = AudioGenerator::generateSineWave(440.0f, 1.0f, 16000);
    
    // Should not throw
    EXPECT_NO_THROW({
        AudioConverter::saveToFile(buffer, "output.wav");
    });
}

TEST_F(AudioConverterTest, SaveEmptyBuffer) {
    AudioBuffer empty;
    
    // Should throw exception
    EXPECT_THROW({
        AudioConverter::saveToFile(empty, "empty.wav");
    }, AudioException);
}

// Extension support tests

TEST_F(AudioConverterTest, SupportedExtensions) {
    auto extensions = AudioConverter::getSupportedExtensions();
    
    // Should support common formats
    EXPECT_NE(std::find(extensions.begin(), extensions.end(), "wav"), extensions.end());
    EXPECT_NE(std::find(extensions.begin(), extensions.end(), "mp3"), extensions.end());
    EXPECT_NE(std::find(extensions.begin(), extensions.end(), "flac"), extensions.end());
}

TEST_F(AudioConverterTest, ExtensionCheck) {
    EXPECT_TRUE(AudioConverter::isExtensionSupported("wav"));
    EXPECT_TRUE(AudioConverter::isExtensionSupported("WAV"));  // Case insensitive
    EXPECT_TRUE(AudioConverter::isExtensionSupported("mp3"));
    EXPECT_FALSE(AudioConverter::isExtensionSupported("xyz"));
}

// Format detection tests

TEST_F(AudioConverterTest, DetectFormat) {
    // Mock implementation returns standard format
    auto format = AudioConverter::detectFormat("test.wav");
    
    EXPECT_EQ(format.sampleRate, 44100);
    EXPECT_EQ(format.channels, 2);
    EXPECT_EQ(format.bitsPerSample, 16);
    EXPECT_FALSE(format.isFloat);
}

// Performance tests

TEST_F(AudioConverterTest, ConversionPerformance) {
    // Create large buffer: 30 seconds at 44.1kHz stereo
    AudioBuffer input;
    input.format = AudioFormat(44100, 2, 16, false);
    input.data.resize(44100 * 2 * 30);  // 30 seconds
    
    // Fill with random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (auto& sample : input.data) {
        sample = dist(gen);
    }
    
    // Measure conversion time
    PerformanceUtils::Timer timer("30s audio conversion");
    
    AudioConverter::ConversionParams params;
    params.targetFormat = AudioFormat(16000, 1, 32, true);
    
    auto output = converter->convert(input, params);
    
    // Conversion should complete (timer will print time)
    EXPECT_FALSE(output.empty());
}

// Main function for running tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}