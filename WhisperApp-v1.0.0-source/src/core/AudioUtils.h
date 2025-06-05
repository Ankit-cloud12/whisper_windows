/*
 * AudioUtils.h
 * 
 * Audio utility functions for processing and analysis
 */

#ifndef AUDIOUTILS_H
#define AUDIOUTILS_H

#include <vector>
#include <string>
#include <cstdint>
#include <complex>

namespace AudioUtils {

/**
 * @brief Audio statistics structure
 */
struct AudioStats {
    float rms;          // Root Mean Square
    float peak;         // Peak amplitude
    float crest_factor; // Peak to RMS ratio
    float zero_crossings; // Zero crossing rate
};

/**
 * @brief Calculate RMS (Root Mean Square) level
 * @param samples Audio samples
 * @param count Number of samples
 * @return RMS value (0.0 to 1.0)
 */
float calculateRMS(const float* samples, size_t count);

/**
 * @brief Calculate peak level
 * @param samples Audio samples  
 * @param count Number of samples
 * @return Peak value (0.0 to 1.0)
 */
float calculatePeak(const float* samples, size_t count);

/**
 * @brief Calculate audio statistics
 * @param samples Audio samples
 * @param count Number of samples
 * @return Audio statistics
 */
AudioStats calculateStats(const float* samples, size_t count);

/**
 * @brief Detect silence in audio
 * @param samples Audio samples
 * @param count Number of samples
 * @param threshold Silence threshold (default 0.01)
 * @param min_duration Minimum silence duration in samples
 * @return true if silence detected
 */
bool detectSilence(const float* samples, size_t count, 
                   float threshold = 0.01f, size_t min_duration = 100);

/**
 * @brief Apply simple noise gate
 * @param samples Audio samples (modified in place)
 * @param count Number of samples
 * @param threshold Gate threshold
 * @param attack_time Attack time in samples
 * @param release_time Release time in samples
 */
void applyNoiseGate(float* samples, size_t count,
                    float threshold = 0.01f,
                    size_t attack_time = 10,
                    size_t release_time = 100);

/**
 * @brief Simple spectral subtraction noise reduction
 * @param samples Audio samples (modified in place)
 * @param count Number of samples
 * @param noise_floor Estimated noise floor
 * @param reduction_factor Noise reduction factor (0.0 to 1.0)
 */
void reduceNoise(float* samples, size_t count,
                 float noise_floor = 0.01f,
                 float reduction_factor = 0.5f);

/**
 * @brief Convert audio sample rate
 * @param input Input samples
 * @param input_count Number of input samples
 * @param input_rate Input sample rate
 * @param output_rate Output sample rate
 * @return Resampled audio
 */
std::vector<float> resample(const float* input, size_t input_count,
                           int input_rate, int output_rate);

/**
 * @brief Convert stereo to mono
 * @param stereo Stereo samples (interleaved L,R,L,R...)
 * @param sample_count Number of stereo samples (total samples / 2)
 * @return Mono audio
 */
std::vector<float> stereoToMono(const float* stereo, size_t sample_count);

/**
 * @brief Normalize audio to specified peak level
 * @param samples Audio samples (modified in place)
 * @param count Number of samples
 * @param target_peak Target peak level (default 0.95)
 */
void normalize(float* samples, size_t count, float target_peak = 0.95f);

/**
 * @brief Apply fade in effect
 * @param samples Audio samples (modified in place)
 * @param count Number of samples
 * @param fade_samples Number of samples for fade
 */
void fadeIn(float* samples, size_t count, size_t fade_samples);

/**
 * @brief Apply fade out effect
 * @param samples Audio samples (modified in place)
 * @param count Number of samples
 * @param fade_samples Number of samples for fade
 */
void fadeOut(float* samples, size_t count, size_t fade_samples);

/**
 * @brief WAV file header structure
 */
struct WavHeader {
    char riff[4];           // "RIFF"
    uint32_t file_size;     // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmt_size;      // Format chunk size (16)
    uint16_t audio_format;  // Audio format (1 = PCM)
    uint16_t channels;      // Number of channels
    uint32_t sample_rate;   // Sample rate
    uint32_t byte_rate;     // Byte rate
    uint16_t block_align;   // Block align
    uint16_t bits_per_sample; // Bits per sample
    char data[4];           // "data"
    uint32_t data_size;     // Data size
};

/**
 * @brief Create WAV file header
 * @param sample_rate Sample rate
 * @param channels Number of channels
 * @param bits_per_sample Bits per sample (16 or 32)
 * @param data_size Size of audio data in bytes
 * @return WAV header structure
 */
WavHeader createWavHeader(uint32_t sample_rate, uint16_t channels,
                         uint16_t bits_per_sample, uint32_t data_size);

/**
 * @brief Save audio to WAV file
 * @param filename Output filename
 * @param samples Audio samples
 * @param count Number of samples
 * @param sample_rate Sample rate
 * @param channels Number of channels
 * @return true if successful
 */
bool saveWav(const std::string& filename, const float* samples, size_t count,
             uint32_t sample_rate, uint16_t channels);

/**
 * @brief Load audio from WAV file
 * @param filename Input filename
 * @param sample_rate Output sample rate
 * @param channels Output channels
 * @return Audio samples (empty if failed)
 */
std::vector<float> loadWav(const std::string& filename, 
                          uint32_t& sample_rate, uint16_t& channels);

/**
 * @brief Apply pre-emphasis filter for speech
 * @param samples Audio samples (modified in place)
 * @param count Number of samples
 * @param coefficient Pre-emphasis coefficient (default 0.97)
 */
void preEmphasis(float* samples, size_t count, float coefficient = 0.97f);

/**
 * @brief Calculate zero crossing rate
 * @param samples Audio samples
 * @param count Number of samples
 * @return Zero crossing rate
 */
float calculateZeroCrossingRate(const float* samples, size_t count);

/**
 * @brief Simple energy-based voice activity detection
 * @param samples Audio samples
 * @param count Number of samples
 * @param frame_size Frame size for analysis
 * @param energy_threshold Energy threshold
 * @param zcr_threshold Zero crossing rate threshold
 * @return Vector of VAD decisions (true = voice, false = silence)
 */
std::vector<bool> detectVoiceActivity(const float* samples, size_t count,
                                     size_t frame_size = 256,
                                     float energy_threshold = 0.01f,
                                     float zcr_threshold = 0.1f);

/**
 * @brief Apply high-pass filter to remove DC offset
 * @param samples Audio samples (modified in place)
 * @param count Number of samples
 * @param cutoff_freq Cutoff frequency (Hz)
 * @param sample_rate Sample rate
 */
void removeDCOffset(float* samples, size_t count, 
                   float cutoff_freq = 80.0f, float sample_rate = 16000.0f);

/**
 * @brief Clip audio samples to prevent overflow
 * @param samples Audio samples (modified in place)
 * @param count Number of samples
 * @param max_value Maximum absolute value (default 1.0)
 * @return Number of clipped samples
 */
size_t clipAudio(float* samples, size_t count, float max_value = 1.0f);

} // namespace AudioUtils

#endif // AUDIOUTILS_H