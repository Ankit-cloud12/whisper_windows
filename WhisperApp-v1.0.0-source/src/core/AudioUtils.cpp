/*
 * AudioUtils.cpp
 * 
 * Implementation of audio utility functions
 */

#include "AudioUtils.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <cstring>

namespace AudioUtils {

float calculateRMS(const float* samples, size_t count) {
    if (count == 0) return 0.0f;
    
    float sum = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        sum += samples[i] * samples[i];
    }
    
    return std::sqrt(sum / count);
}

float calculatePeak(const float* samples, size_t count) {
    if (count == 0) return 0.0f;
    
    float peak = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        peak = std::max(peak, std::abs(samples[i]));
    }
    
    return peak;
}

AudioStats calculateStats(const float* samples, size_t count) {
    AudioStats stats = {0};
    
    if (count == 0) return stats;
    
    stats.rms = calculateRMS(samples, count);
    stats.peak = calculatePeak(samples, count);
    stats.crest_factor = (stats.rms > 0) ? (stats.peak / stats.rms) : 0.0f;
    stats.zero_crossings = calculateZeroCrossingRate(samples, count);
    
    return stats;
}

bool detectSilence(const float* samples, size_t count, float threshold, size_t min_duration) {
    if (count < min_duration) return false;
    
    size_t silence_count = 0;
    
    for (size_t i = 0; i < count; ++i) {
        if (std::abs(samples[i]) < threshold) {
            silence_count++;
            if (silence_count >= min_duration) {
                return true;
            }
        } else {
            silence_count = 0;
        }
    }
    
    return false;
}

void applyNoiseGate(float* samples, size_t count, float threshold, 
                    size_t attack_time, size_t release_time) {
    bool gate_open = false;
    size_t hold_time = 0;
    
    for (size_t i = 0; i < count; ++i) {
        float level = std::abs(samples[i]);
        
        if (level >= threshold) {
            gate_open = true;
            hold_time = release_time;
        } else if (hold_time > 0) {
            hold_time--;
        } else {
            gate_open = false;
        }
        
        if (!gate_open) {
            // Apply smooth fade to avoid clicks
            float fade = (hold_time > 0) ? float(hold_time) / release_time : 0.0f;
            samples[i] *= fade;
        }
    }
}

void reduceNoise(float* samples, size_t count, float noise_floor, float reduction_factor) {
    // Simple spectral subtraction approximation
    // In a real implementation, this would use FFT
    
    for (size_t i = 0; i < count; ++i) {
        float magnitude = std::abs(samples[i]);
        
        if (magnitude > noise_floor) {
            float reduced = magnitude - (noise_floor * reduction_factor);
            float ratio = reduced / magnitude;
            samples[i] *= ratio;
        } else {
            samples[i] *= (1.0f - reduction_factor);
        }
    }
}

std::vector<float> resample(const float* input, size_t input_count,
                           int input_rate, int output_rate) {
    if (input_rate == output_rate) {
        return std::vector<float>(input, input + input_count);
    }
    
    // Simple linear interpolation resampling
    double ratio = double(input_rate) / output_rate;
    size_t output_count = size_t(input_count / ratio);
    std::vector<float> output(output_count);
    
    for (size_t i = 0; i < output_count; ++i) {
        double src_pos = i * ratio;
        size_t src_idx = size_t(src_pos);
        double frac = src_pos - src_idx;
        
        if (src_idx + 1 < input_count) {
            output[i] = float(input[src_idx] * (1.0 - frac) + 
                            input[src_idx + 1] * frac);
        } else {
            output[i] = input[src_idx];
        }
    }
    
    return output;
}

std::vector<float> stereoToMono(const float* stereo, size_t sample_count) {
    std::vector<float> mono(sample_count);
    
    for (size_t i = 0; i < sample_count; ++i) {
        mono[i] = (stereo[i * 2] + stereo[i * 2 + 1]) * 0.5f;
    }
    
    return mono;
}

void normalize(float* samples, size_t count, float target_peak) {
    float current_peak = calculatePeak(samples, count);
    
    if (current_peak > 0.0f && current_peak != target_peak) {
        float scale = target_peak / current_peak;
        
        for (size_t i = 0; i < count; ++i) {
            samples[i] *= scale;
        }
    }
}

void fadeIn(float* samples, size_t count, size_t fade_samples) {
    fade_samples = std::min(fade_samples, count);
    
    for (size_t i = 0; i < fade_samples; ++i) {
        float factor = float(i) / fade_samples;
        samples[i] *= factor;
    }
}

void fadeOut(float* samples, size_t count, size_t fade_samples) {
    fade_samples = std::min(fade_samples, count);
    size_t start = count - fade_samples;
    
    for (size_t i = 0; i < fade_samples; ++i) {
        float factor = 1.0f - (float(i) / fade_samples);
        samples[start + i] *= factor;
    }
}

WavHeader createWavHeader(uint32_t sample_rate, uint16_t channels,
                         uint16_t bits_per_sample, uint32_t data_size) {
    WavHeader header;
    
    std::memcpy(header.riff, "RIFF", 4);
    header.file_size = 36 + data_size;
    std::memcpy(header.wave, "WAVE", 4);
    std::memcpy(header.fmt, "fmt ", 4);
    header.fmt_size = 16;
    header.audio_format = 1; // PCM
    header.channels = channels;
    header.sample_rate = sample_rate;
    header.byte_rate = sample_rate * channels * (bits_per_sample / 8);
    header.block_align = channels * (bits_per_sample / 8);
    header.bits_per_sample = bits_per_sample;
    std::memcpy(header.data, "data", 4);
    header.data_size = data_size;
    
    return header;
}

bool saveWav(const std::string& filename, const float* samples, size_t count,
             uint32_t sample_rate, uint16_t channels) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;
    
    // Convert float samples to 16-bit PCM
    std::vector<int16_t> pcm_data(count);
    for (size_t i = 0; i < count; ++i) {
        float sample = std::max(-1.0f, std::min(1.0f, samples[i]));
        pcm_data[i] = static_cast<int16_t>(sample * 32767.0f);
    }
    
    uint32_t data_size = count * sizeof(int16_t);
    WavHeader header = createWavHeader(sample_rate, channels, 16, data_size);
    
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(pcm_data.data()), data_size);
    
    return file.good();
}

std::vector<float> loadWav(const std::string& filename, 
                          uint32_t& sample_rate, uint16_t& channels) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return {};
    
    WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (std::memcmp(header.riff, "RIFF", 4) != 0 ||
        std::memcmp(header.wave, "WAVE", 4) != 0) {
        return {};
    }
    
    sample_rate = header.sample_rate;
    channels = header.channels;
    
    size_t sample_count = header.data_size / (header.bits_per_sample / 8);
    std::vector<float> samples;
    
    if (header.bits_per_sample == 16) {
        std::vector<int16_t> pcm_data(sample_count);
        file.read(reinterpret_cast<char*>(pcm_data.data()), header.data_size);
        
        samples.resize(sample_count);
        for (size_t i = 0; i < sample_count; ++i) {
            samples[i] = pcm_data[i] / 32768.0f;
        }
    } else if (header.bits_per_sample == 32) {
        samples.resize(sample_count);
        file.read(reinterpret_cast<char*>(samples.data()), header.data_size);
    }
    
    return samples;
}

void preEmphasis(float* samples, size_t count, float coefficient) {
    if (count == 0) return;
    
    // Process from end to beginning to avoid overwriting
    for (size_t i = count - 1; i > 0; --i) {
        samples[i] = samples[i] - coefficient * samples[i - 1];
    }
}

float calculateZeroCrossingRate(const float* samples, size_t count) {
    if (count < 2) return 0.0f;
    
    size_t crossings = 0;
    for (size_t i = 1; i < count; ++i) {
        if ((samples[i] >= 0) != (samples[i - 1] >= 0)) {
            crossings++;
        }
    }
    
    return float(crossings) / (count - 1);
}

std::vector<bool> detectVoiceActivity(const float* samples, size_t count,
                                     size_t frame_size, float energy_threshold,
                                     float zcr_threshold) {
    size_t num_frames = count / frame_size;
    std::vector<bool> vad_results(num_frames, false);
    
    for (size_t frame = 0; frame < num_frames; ++frame) {
        const float* frame_start = samples + (frame * frame_size);
        
        float energy = calculateRMS(frame_start, frame_size);
        float zcr = calculateZeroCrossingRate(frame_start, frame_size);
        
        // Simple VAD: high energy and moderate ZCR indicates voice
        vad_results[frame] = (energy > energy_threshold) && 
                           (zcr < zcr_threshold);
    }
    
    return vad_results;
}

void removeDCOffset(float* samples, size_t count, float cutoff_freq, float sample_rate) {
    if (count == 0) return;
    
    // Simple high-pass filter
    float alpha = 1.0f / (1.0f + 2.0f * 3.14159f * cutoff_freq / sample_rate);
    float prev_input = 0.0f;
    float prev_output = 0.0f;
    
    for (size_t i = 0; i < count; ++i) {
        float input = samples[i];
        samples[i] = alpha * (prev_output + input - prev_input);
        prev_input = input;
        prev_output = samples[i];
    }
}

size_t clipAudio(float* samples, size_t count, float max_value) {
    size_t clipped_count = 0;
    
    for (size_t i = 0; i < count; ++i) {
        if (samples[i] > max_value) {
            samples[i] = max_value;
            clipped_count++;
        } else if (samples[i] < -max_value) {
            samples[i] = -max_value;
            clipped_count++;
        }
    }
    
    return clipped_count;
}

} // namespace AudioUtils