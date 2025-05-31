/*
 * AudioConverter.cpp
 * 
 * Implementation of the AudioConverter class.
 */

#include "AudioConverter.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>
#include <fstream>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace WhisperApp {

/**
 * @brief Private implementation class
 */
class AudioConverter::Impl {
public:
    std::mt19937 rng;  // Random number generator for dithering
    
    Impl() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}
    
    // Linear interpolation for resampling
    float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }
    
    // Simple sinc interpolation kernel
    float sinc(float x) {
        if (std::abs(x) < 0.001f) return 1.0f;
        float pi_x = M_PI * x;
        return std::sin(pi_x) / pi_x;
    }
    
    // Lanczos window function
    float lanczos(float x, int a) {
        if (std::abs(x) >= a) return 0.0f;
        if (std::abs(x) < 0.001f) return 1.0f;
        return sinc(x) * sinc(x / a);
    }
};

// Constructor
AudioConverter::AudioConverter() : pImpl(std::make_unique<Impl>()) {
    LOG_DEBUG("AudioConverter", "Audio converter initialized");
}

// Destructor
AudioConverter::~AudioConverter() = default;

// Main conversion function
AudioBuffer AudioConverter::convert(const AudioBuffer& input,
                                  const ConversionParams& params,
                                  ConversionStats* stats) {
    LOG_TIMER("AudioConverter", "Audio conversion");
    
    if (input.empty()) {
        throw AudioException(ErrorCode::AudioDataEmpty, "Input audio buffer is empty");
    }
    
    auto start_time = std::chrono::steady_clock::now();
    AudioBuffer output;
    output.format = params.targetFormat;
    output.timestamp_ms = input.timestamp_ms;
    
    // Start with the input data
    std::vector<float> working = input.data;
    AudioFormat currentFormat = input.format;
    
    // Step 1: Remove DC offset if requested
    if (params.removeDCOffset) {
        LOG_DEBUG("AudioConverter", "Removing DC offset");
        working = removeDCOffset(working);
    }
    
    // Step 2: Convert channels if needed
    if (currentFormat.channels != params.targetFormat.channels) {
        LOG_DEBUG("AudioConverter", "Converting channels: " + 
                  std::to_string(currentFormat.channels) + " -> " + 
                  std::to_string(params.targetFormat.channels));
        
        if (currentFormat.channels == 2 && params.targetFormat.channels == 1) {
            working = stereoToMono(working);
        } else if (currentFormat.channels == 1 && params.targetFormat.channels == 2) {
            working = monoToStereo(working);
        } else {
            throw AudioException(ErrorCode::AudioChannelCountInvalid,
                               "Unsupported channel conversion");
        }
        currentFormat.channels = params.targetFormat.channels;
    }
    
    // Step 3: Resample if needed
    if (currentFormat.sampleRate != params.targetFormat.sampleRate) {
        LOG_DEBUG("AudioConverter", "Resampling: " + 
                  std::to_string(currentFormat.sampleRate) + " Hz -> " + 
                  std::to_string(params.targetFormat.sampleRate) + " Hz");
        
        working = resample(working, currentFormat.sampleRate, 
                          params.targetFormat.sampleRate, params.quality);
        currentFormat.sampleRate = params.targetFormat.sampleRate;
    }
    
    // Step 4: Normalize if requested
    if (params.normalizeAudio) {
        LOG_DEBUG("AudioConverter", "Normalizing audio");
        working = normalize(working);
    }
    
    // Step 5: Apply dithering if converting to lower bit depth
    if (params.applyDithering && 
        params.targetFormat.bitsPerSample < 32) {
        LOG_DEBUG("AudioConverter", "Applying dithering for " + 
                  std::to_string(params.targetFormat.bitsPerSample) + "-bit output");
        working = applyDithering(working, params.targetFormat.bitsPerSample);
    }
    
    // Calculate statistics if requested
    if (stats) {
        *stats = calculateStats(working);
        auto end_time = std::chrono::steady_clock::now();
        stats->processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
    }
    
    output.data = std::move(working);
    
    LOG_INFO("AudioConverter", "Conversion completed: " + 
             std::to_string(input.data.size()) + " -> " + 
             std::to_string(output.data.size()) + " samples");
    
    return output;
}

// Convert raw data to float32
std::vector<float> AudioConverter::toFloat32(const std::vector<uint8_t>& data,
                                            const AudioFormat& format) {
    std::vector<float> result;
    size_t sampleCount = data.size() / (format.bitsPerSample / 8);
    result.reserve(sampleCount);
    
    const uint8_t* ptr = data.data();
    
    for (size_t i = 0; i < sampleCount; ++i) {
        float sample = 0.0f;
        
        if (format.bitsPerSample == 8) {
            // 8-bit unsigned
            uint8_t val = *ptr++;
            sample = (val - 128) / 128.0f;
        } else if (format.bitsPerSample == 16) {
            // 16-bit signed
            int16_t val = *reinterpret_cast<const int16_t*>(ptr);
            ptr += 2;
            sample = val / 32768.0f;
        } else if (format.bitsPerSample == 24) {
            // 24-bit signed
            int32_t val = 0;
            std::memcpy(&val, ptr, 3);
            if (val & 0x800000) {  // Sign extend
                val |= 0xFF000000;
            }
            ptr += 3;
            sample = val / 8388608.0f;
        } else if (format.bitsPerSample == 32) {
            if (format.isFloat) {
                // 32-bit float
                sample = *reinterpret_cast<const float*>(ptr);
                ptr += 4;
            } else {
                // 32-bit signed integer
                int32_t val = *reinterpret_cast<const int32_t*>(ptr);
                ptr += 4;
                sample = val / 2147483648.0f;
            }
        }
        
        result.push_back(sample);
    }
    
    return result;
}

// Convert float32 to raw data
std::vector<uint8_t> AudioConverter::fromFloat32(const std::vector<float>& samples,
                                                const AudioFormat& format) {
    std::vector<uint8_t> result;
    size_t byteSize = samples.size() * (format.bitsPerSample / 8);
    result.resize(byteSize);
    
    uint8_t* ptr = result.data();
    
    for (float sample : samples) {
        // Clamp to valid range
        sample = std::clamp(sample, -1.0f, 1.0f);
        
        if (format.bitsPerSample == 8) {
            // 8-bit unsigned
            uint8_t val = static_cast<uint8_t>((sample + 1.0f) * 128.0f);
            *ptr++ = val;
        } else if (format.bitsPerSample == 16) {
            // 16-bit signed
            int16_t val = static_cast<int16_t>(sample * 32767.0f);
            *reinterpret_cast<int16_t*>(ptr) = val;
            ptr += 2;
        } else if (format.bitsPerSample == 24) {
            // 24-bit signed
            int32_t val = static_cast<int32_t>(sample * 8388607.0f);
            std::memcpy(ptr, &val, 3);
            ptr += 3;
        } else if (format.bitsPerSample == 32) {
            if (format.isFloat) {
                // 32-bit float
                *reinterpret_cast<float*>(ptr) = sample;
                ptr += 4;
            } else {
                // 32-bit signed integer
                int32_t val = static_cast<int32_t>(sample * 2147483647.0f);
                *reinterpret_cast<int32_t*>(ptr) = val;
                ptr += 4;
            }
        }
    }
    
    return result;
}

// Resample audio
std::vector<float> AudioConverter::resample(const std::vector<float>& input,
                                           int inputRate,
                                           int outputRate,
                                           ConversionQuality quality) {
    if (inputRate == outputRate) {
        return input;
    }
    
    LOG_DEBUG("AudioConverter", "Resampling from " + std::to_string(inputRate) + 
              " to " + std::to_string(outputRate));
    
    double ratio = static_cast<double>(outputRate) / inputRate;
    size_t outputSize = static_cast<size_t>(input.size() * ratio);
    std::vector<float> output;
    output.reserve(outputSize);
    
    // Simple linear interpolation for now
    // TODO: Implement better resampling algorithms based on quality setting
    for (size_t i = 0; i < outputSize; ++i) {
        double srcIdx = i / ratio;
        size_t idx0 = static_cast<size_t>(srcIdx);
        size_t idx1 = std::min(idx0 + 1, input.size() - 1);
        double frac = srcIdx - idx0;
        
        float sample = input[idx0] * (1.0 - frac) + input[idx1] * frac;
        output.push_back(sample);
    }
    
    return output;
}

// Convert stereo to mono
std::vector<float> AudioConverter::stereoToMono(const std::vector<float>& stereo) {
    std::vector<float> mono;
    mono.reserve(stereo.size() / 2);
    
    for (size_t i = 0; i < stereo.size(); i += 2) {
        float left = stereo[i];
        float right = (i + 1 < stereo.size()) ? stereo[i + 1] : 0.0f;
        mono.push_back((left + right) * 0.5f);
    }
    
    return mono;
}

// Convert mono to stereo
std::vector<float> AudioConverter::monoToStereo(const std::vector<float>& mono) {
    std::vector<float> stereo;
    stereo.reserve(mono.size() * 2);
    
    for (float sample : mono) {
        stereo.push_back(sample);  // Left
        stereo.push_back(sample);  // Right
    }
    
    return stereo;
}

// Normalize audio
std::vector<float> AudioConverter::normalize(const std::vector<float>& samples,
                                            float targetPeak) {
    if (samples.empty()) {
        return samples;
    }
    
    // Find peak absolute value
    float peak = 0.0f;
    for (float sample : samples) {
        peak = std::max(peak, std::abs(sample));
    }
    
    if (peak == 0.0f || peak == targetPeak) {
        return samples;
    }
    
    // Calculate scaling factor
    float scale = targetPeak / peak;
    
    // Apply scaling
    std::vector<float> normalized;
    normalized.reserve(samples.size());
    
    for (float sample : samples) {
        normalized.push_back(sample * scale);
    }
    
    return normalized;
}

// Remove DC offset
std::vector<float> AudioConverter::removeDCOffset(const std::vector<float>& samples) {
    if (samples.empty()) {
        return samples;
    }
    
    // Calculate average (DC offset)
    double sum = 0.0;
    for (float sample : samples) {
        sum += sample;
    }
    float dcOffset = static_cast<float>(sum / samples.size());
    
    // Remove DC offset
    std::vector<float> result;
    result.reserve(samples.size());
    
    for (float sample : samples) {
        result.push_back(sample - dcOffset);
    }
    
    return result;
}

// Apply dithering
std::vector<float> AudioConverter::applyDithering(const std::vector<float>& samples,
                                                 int targetBits) {
    std::vector<float> dithered;
    dithered.reserve(samples.size());
    
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    float quantizationStep = 1.0f / (1 << (targetBits - 1));
    
    AudioConverter converter;
    
    for (float sample : samples) {
        // Add triangular dither
        float dither = (dist(converter.pImpl->rng) + dist(converter.pImpl->rng)) * 0.5f;
        dither *= quantizationStep * 0.5f;
        
        float ditheredSample = sample + dither;
        dithered.push_back(ditheredSample);
    }
    
    return dithered;
}

// Calculate statistics
AudioConverter::ConversionStats AudioConverter::calculateStats(const std::vector<float>& samples) {
    ConversionStats stats;
    
    if (samples.empty()) {
        return stats;
    }
    
    double sum = 0.0;
    double sumSquares = 0.0;
    
    for (float sample : samples) {
        float absSample = std::abs(sample);
        stats.peakLevel = std::max(stats.peakLevel, absSample);
        
        if (absSample > 1.0f) {
            stats.clippedSamples++;
        }
        
        sum += sample;
        sumSquares += sample * sample;
    }
    
    stats.dcOffset = static_cast<float>(sum / samples.size());
    stats.averageLevel = static_cast<float>(std::sqrt(sumSquares / samples.size()));
    
    return stats;
}

// Load audio from file (mock implementation)
AudioBuffer AudioConverter::loadFromFile(const std::string& filePath) {
    LOG_INFO("AudioConverter", "Loading audio from: " + filePath);
    
    // Mock implementation - would use actual audio loading library
    AudioBuffer buffer;
    buffer.format = AudioFormat(44100, 2, 16, false);  // Assume CD quality
    
    // Generate some test data
    buffer.data.resize(44100);  // 1 second of silence
    
    LOG_INFO("AudioConverter", "Loaded " + std::to_string(buffer.data.size()) + " samples");
    return buffer;
}

// Save audio to file (mock implementation)
void AudioConverter::saveToFile(const AudioBuffer& buffer,
                              const std::string& filePath,
                              const AudioFormat& format) {
    LOG_INFO("AudioConverter", "Saving audio to: " + filePath);
    
    // Mock implementation - would use actual audio saving library
    if (buffer.empty()) {
        throw AudioException(ErrorCode::AudioDataEmpty, "Cannot save empty audio buffer");
    }
    
    // In real implementation, would write to file
    LOG_INFO("AudioConverter", "Saved " + std::to_string(buffer.data.size()) + " samples");
}

// Get supported extensions
std::vector<std::string> AudioConverter::getSupportedExtensions() {
    return {"wav", "mp3", "ogg", "flac", "aac", "m4a", "wma"};
}

// Check if extension is supported
bool AudioConverter::isExtensionSupported(const std::string& extension) {
    auto supported = getSupportedExtensions();
    std::string lowerExt = extension;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);
    
    return std::find(supported.begin(), supported.end(), lowerExt) != supported.end();
}

// Detect format from file (mock implementation)
AudioFormat AudioConverter::detectFormat(const std::string& filePath) {
    LOG_DEBUG("AudioConverter", "Detecting format for: " + filePath);
    
    // Mock implementation - would use actual format detection
    // For now, return a common format
    return AudioFormat(44100, 2, 16, false);
}

// Split audio into chunks
std::vector<AudioBuffer> AudioConverter::splitIntoChunks(const AudioBuffer& buffer,
                                                        uint64_t chunkDurationMs,
                                                        uint64_t overlapMs) {
    std::vector<AudioBuffer> chunks;
    
    if (buffer.empty() || chunkDurationMs == 0) {
        return chunks;
    }
    
    size_t samplesPerChunk = (buffer.format.sampleRate * chunkDurationMs) / 1000;
    size_t samplesOverlap = (buffer.format.sampleRate * overlapMs) / 1000;
    size_t stride = samplesPerChunk - samplesOverlap;
    
    for (size_t i = 0; i < buffer.data.size(); i += stride) {
        AudioBuffer chunk;
        chunk.format = buffer.format;
        chunk.timestamp_ms = buffer.timestamp_ms + (i * 1000) / buffer.format.sampleRate;
        
        size_t end = std::min(i + samplesPerChunk, buffer.data.size());
        chunk.data.assign(buffer.data.begin() + i, buffer.data.begin() + end);
        
        chunks.push_back(chunk);
        
        if (end >= buffer.data.size()) {
            break;
        }
    }
    
    LOG_DEBUG("AudioConverter", "Split audio into " + std::to_string(chunks.size()) + " chunks");
    return chunks;
}

// Merge audio chunks
AudioBuffer AudioConverter::mergeChunks(const std::vector<AudioBuffer>& chunks,
                                       uint64_t overlapMs) {
    AudioBuffer merged;
    
    if (chunks.empty()) {
        return merged;
    }
    
    merged.format = chunks[0].format;
    merged.timestamp_ms = chunks[0].timestamp_ms;
    
    size_t samplesOverlap = (merged.format.sampleRate * overlapMs) / 1000;
    
    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& chunk = chunks[i];
        
        if (i == 0) {
            // First chunk - add everything
            merged.data.insert(merged.data.end(), chunk.data.begin(), chunk.data.end());
        } else {
            // Subsequent chunks - skip overlap
            size_t skipSamples = std::min(samplesOverlap, chunk.data.size());
            merged.data.insert(merged.data.end(), 
                             chunk.data.begin() + skipSamples, 
                             chunk.data.end());
        }
    }
    
    LOG_DEBUG("AudioConverter", "Merged " + std::to_string(chunks.size()) + 
              " chunks into " + std::to_string(merged.data.size()) + " samples");
    return merged;
}

} // namespace WhisperApp