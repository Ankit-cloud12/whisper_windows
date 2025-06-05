/*
 * AudioCapture.h
 * 
 * Audio capture functionality using Windows Audio Session API (WASAPI).
 * This class handles real-time audio recording from system audio devices.
 * 
 * Features:
 * - Device enumeration and selection
 * - Real-time audio capture with low latency
 * - Audio format conversion (to 16kHz mono float32 for Whisper)
 * - Silence detection and automatic recording
 * - Audio level monitoring
 */

#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>

// Forward declarations for Windows types
struct IMMDeviceEnumerator;
struct IMMDevice;
struct IAudioClient;
struct IAudioCaptureClient;

/**
 * @brief Audio device information
 */
struct AudioDevice {
    std::string id;                 // Unique device ID
    std::string name;               // Friendly device name
    bool is_default = false;        // Is this the default device
    bool is_loopback = false;       // Is this a loopback device
    int channels = 0;               // Number of channels
    int sample_rate = 0;            // Native sample rate
};

/**
 * @brief Audio capture configuration
 */
struct AudioCaptureConfig {
    int sample_rate = 16000;        // Target sample rate (16kHz for Whisper)
    int channels = 1;               // Target channels (1 = mono)
    int buffer_size_ms = 100;       // Buffer size in milliseconds
    bool enable_noise_suppression = false;  // Basic noise suppression
    bool enable_silence_detection = true;   // Automatic silence detection
    float silence_threshold = 0.01f;        // Silence detection threshold
    int silence_duration_ms = 2000;         // Silence duration before stop
};

/**
 * @brief Main class for audio capture functionality
 */
class AudioCapture {
public:
    /**
     * @brief Audio data callback function type
     * @param audio_data Buffer containing audio samples
     * @param sample_count Number of samples in the buffer
     */
    using AudioCallback = std::function<void(const float* audio_data, size_t sample_count)>;
    
    /**
     * @brief Audio level callback function type
     * @param level Current audio level (0.0 to 1.0)
     */
    using LevelCallback = std::function<void(float level)>;
    
    /**
     * @brief Device change callback function type
     */
    using DeviceChangeCallback = std::function<void()>;

public:
    AudioCapture();
    ~AudioCapture();

    // Prevent copying
    AudioCapture(const AudioCapture&) = delete;
    AudioCapture& operator=(const AudioCapture&) = delete;

    /**
     * @brief Initialize the audio capture system
     * @return true if successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Shutdown the audio capture system
     */
    void shutdown();

    /**
     * @brief Get list of available audio devices
     * @return Vector of audio devices
     */
    std::vector<AudioDevice> getAudioDevices() const;

    /**
     * @brief Get the default audio device
     * @return Default audio device info
     */
    AudioDevice getDefaultDevice() const;

    /**
     * @brief Set the audio device to use
     * @param device_id Device ID from getAudioDevices()
     * @return true if successful, false otherwise
     */
    bool setDevice(const std::string& device_id);

    /**
     * @brief Get the current device ID
     * @return Current device ID
     */
    std::string getCurrentDeviceId() const;

    /**
     * @brief Set capture configuration
     * @param config Audio capture configuration
     */
    void setConfig(const AudioCaptureConfig& config);

    /**
     * @brief Get current capture configuration
     * @return Current configuration
     */
    AudioCaptureConfig getConfig() const;

    /**
     * @brief Start audio capture
     * @param callback Callback for audio data
     * @return true if successful, false otherwise
     */
    bool startCapture(AudioCallback callback);

    /**
     * @brief Stop audio capture
     */
    void stopCapture();

    /**
     * @brief Check if capture is active
     * @return true if capturing, false otherwise
     */
    bool isCapturing() const;

    /**
     * @brief Get captured audio data (alternative to callback)
     * @return Vector of audio samples
     */
    std::vector<float> getCapturedAudio() const;

    /**
     * @brief Clear captured audio buffer
     */
    void clearBuffer();

    /**
     * @brief Get current audio level
     * @return Audio level (0.0 to 1.0)
     */
    float getAudioLevel() const;

    /**
     * @brief Set audio level monitoring callback
     * @param callback Level callback function
     */
    void setLevelCallback(LevelCallback callback);

    /**
     * @brief Set device change notification callback
     * @param callback Device change callback
     */
    void setDeviceChangeCallback(DeviceChangeCallback callback);

    /**
     * @brief Enable or disable loopback capture (system audio)
     * @param enable true to enable loopback, false for microphone
     */
    void setLoopbackEnabled(bool enable);

    /**
     * @brief Check if loopback capture is enabled
     * @return true if loopback is enabled
     */
    bool isLoopbackEnabled() const;

    /**
     * @brief Get capture statistics
     */
    struct CaptureStats {
        uint64_t total_samples = 0;     // Total samples captured
        uint64_t dropped_samples = 0;   // Samples dropped due to buffer overflow
        float average_level = 0.0f;     // Average audio level
        int buffer_overruns = 0;        // Number of buffer overruns
    };
    CaptureStats getStats() const;

    /**
     * @brief Reset capture statistics
     */
    void resetStats();

private:
    // Private implementation (PIMPL idiom)
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // AUDIOCAPTURE_H