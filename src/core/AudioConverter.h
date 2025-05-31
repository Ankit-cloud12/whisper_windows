/*
 * AudioConverter.h
 * 
 * Audio format conversion utilities for WhisperApp.
 * Handles resampling, channel conversion, and format conversion
 * to prepare audio for Whisper transcription.
 */

#ifndef AUDIOCONVERTER_H
#define AUDIOCONVERTER_H

#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include "ErrorCodes.h"

namespace WhisperApp {

/**
 * @brief Audio format information
 */
struct AudioFormat {
    int sampleRate;      // Sample rate in Hz
    int channels;        // Number of channels (1 = mono, 2 = stereo)
    int bitsPerSample;   // Bits per sample (8, 16, 24, 32)
    bool isFloat;        // True if floating point format
    
    AudioFormat(int sr = 16000, int ch = 1, int bps = 16, bool fp = false)
        : sampleRate(sr), channels(ch), bitsPerSample(bps), isFloat(fp) {}
    
    bool operator==(const AudioFormat& other) const {
        return sampleRate == other.sampleRate &&
               channels == other.channels &&
               bitsPerSample == other.bitsPerSample &&
               isFloat == other.isFloat;
    }
    
    bool operator!=(const AudioFormat& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Audio buffer with format information
 */
struct AudioBuffer {
    std::vector<float> data;     // Audio samples in float32 format
    AudioFormat format;          // Original format information
    uint64_t timestamp_ms = 0;   // Timestamp in milliseconds
    
    // Get duration in milliseconds
    uint64_t getDurationMs() const {
        if (format.sampleRate == 0) return 0;
        return (data.size() * 1000) / format.sampleRate;
    }
    
    // Get size in bytes (original format)
    size_t getSizeBytes() const {
        return data.size() * (format.bitsPerSample / 8);
    }
    
    void clear() {
        data.clear();
        timestamp_ms = 0;
    }
    
    bool empty() const {
        return data.empty();
    }
};

/**
 * @brief Conversion quality settings
 */
enum class ConversionQuality {
    Low,      // Fast, lower quality
    Medium,   // Balanced
    High,     // Slow, higher quality
    Best      // Slowest, best quality
};

/**
 * @brief Audio converter class
 */
class AudioConverter {
public:
    /**
     * @brief Conversion parameters
     */
    struct ConversionParams {
        AudioFormat targetFormat{16000, 1, 32, true};  // Default: 16kHz, mono, float32
        ConversionQuality quality = ConversionQuality::Medium;
        bool normalizeAudio = true;      // Normalize to [-1, 1] range
        float targetLoudness = -16.0f;   // Target loudness in LUFS
        bool removeDCOffset = true;      // Remove DC offset
        bool applyDithering = true;      // Apply dithering for bit depth reduction
    };
    
    /**
     * @brief Conversion statistics
     */
    struct ConversionStats {
        float peakLevel = 0.0f;          // Peak audio level
        float averageLevel = 0.0f;       // Average audio level
        float dcOffset = 0.0f;           // DC offset detected
        uint64_t clippedSamples = 0;     // Number of clipped samples
        uint64_t processingTimeMs = 0;   // Processing time
    };

public:
    AudioConverter();
    ~AudioConverter();
    
    /**
     * @brief Convert audio buffer to target format
     * @param input Input audio buffer
     * @param params Conversion parameters
     * @param stats Optional conversion statistics output
     * @return Converted audio buffer
     * @throws AudioException on conversion failure
     */
    AudioBuffer convert(const AudioBuffer& input, 
                       const ConversionParams& params = ConversionParams(),
                       ConversionStats* stats = nullptr);
    
    /**
     * @brief Convert raw audio data to float32 format
     * @param data Raw audio data
     * @param format Input format
     * @return Float32 samples
     */
    static std::vector<float> toFloat32(const std::vector<uint8_t>& data,
                                       const AudioFormat& format);
    
    /**
     * @brief Convert float32 to raw audio data
     * @param samples Float32 samples
     * @param format Target format
     * @return Raw audio data
     */
    static std::vector<uint8_t> fromFloat32(const std::vector<float>& samples,
                                           const AudioFormat& format);
    
    /**
     * @brief Resample audio to target sample rate
     * @param input Input samples
     * @param inputRate Input sample rate
     * @param outputRate Output sample rate
     * @param quality Resampling quality
     * @return Resampled audio
     */
    static std::vector<float> resample(const std::vector<float>& input,
                                      int inputRate,
                                      int outputRate,
                                      ConversionQuality quality = ConversionQuality::Medium);
    
    /**
     * @brief Convert stereo to mono
     * @param stereo Stereo samples (interleaved)
     * @return Mono samples
     */
    static std::vector<float> stereoToMono(const std::vector<float>& stereo);
    
    /**
     * @brief Convert mono to stereo
     * @param mono Mono samples
     * @return Stereo samples (interleaved)
     */
    static std::vector<float> monoToStereo(const std::vector<float>& mono);
    
    /**
     * @brief Normalize audio to specified range
     * @param samples Audio samples
     * @param targetPeak Target peak level (default: 0.95)
     * @return Normalized samples
     */
    static std::vector<float> normalize(const std::vector<float>& samples,
                                       float targetPeak = 0.95f);
    
    /**
     * @brief Remove DC offset from audio
     * @param samples Audio samples
     * @return Samples with DC offset removed
     */
    static std::vector<float> removeDCOffset(const std::vector<float>& samples);
    
    /**
     * @brief Apply dithering to audio
     * @param samples Audio samples
     * @param targetBits Target bit depth
     * @return Dithered samples
     */
    static std::vector<float> applyDithering(const std::vector<float>& samples,
                                            int targetBits);
    
    /**
     * @brief Calculate audio statistics
     * @param samples Audio samples
     * @return Audio statistics
     */
    static ConversionStats calculateStats(const std::vector<float>& samples);
    
    /**
     * @brief Load audio from file
     * @param filePath Path to audio file
     * @return Audio buffer
     * @throws AudioException on load failure
     */
    static AudioBuffer loadFromFile(const std::string& filePath);
    
    /**
     * @brief Save audio to file
     * @param buffer Audio buffer
     * @param filePath Path to save file
     * @param format Target format (if different from buffer format)
     * @throws AudioException on save failure
     */
    static void saveToFile(const AudioBuffer& buffer,
                          const std::string& filePath,
                          const AudioFormat& format = AudioFormat());
    
    /**
     * @brief Get supported audio file extensions
     * @return Vector of supported extensions (e.g., "wav", "mp3")
     */
    static std::vector<std::string> getSupportedExtensions();
    
    /**
     * @brief Check if file extension is supported
     * @param extension File extension (without dot)
     * @return True if supported
     */
    static bool isExtensionSupported(const std::string& extension);
    
    /**
     * @brief Detect audio format from file
     * @param filePath Path to audio file
     * @return Detected audio format
     * @throws AudioException if format cannot be detected
     */
    static AudioFormat detectFormat(const std::string& filePath);
    
    /**
     * @brief Split audio into chunks
     * @param buffer Audio buffer
     * @param chunkDurationMs Chunk duration in milliseconds
     * @param overlapMs Overlap between chunks in milliseconds
     * @return Vector of audio chunks
     */
    static std::vector<AudioBuffer> splitIntoChunks(const AudioBuffer& buffer,
                                                   uint64_t chunkDurationMs,
                                                   uint64_t overlapMs = 0);
    
    /**
     * @brief Merge audio chunks
     * @param chunks Vector of audio chunks
     * @param overlapMs Overlap to remove in milliseconds
     * @return Merged audio buffer
     */
    static AudioBuffer mergeChunks(const std::vector<AudioBuffer>& chunks,
                                  uint64_t overlapMs = 0);

private:
    // Private implementation
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace WhisperApp

#endif // AUDIOCONVERTER_H