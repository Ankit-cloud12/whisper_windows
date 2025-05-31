/*
 * DeviceManager.h
 * 
 * Windows audio device enumeration and management
 */

#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

/**
 * @brief Audio format information
 */
struct AudioFormat {
    int sample_rate;
    int channels;
    int bits_per_sample;
    std::string format_tag;  // e.g., "PCM", "IEEE_FLOAT"
    
    bool operator==(const AudioFormat& other) const {
        return sample_rate == other.sample_rate &&
               channels == other.channels &&
               bits_per_sample == other.bits_per_sample &&
               format_tag == other.format_tag;
    }
};

/**
 * @brief Device capabilities
 */
struct DeviceCapabilities {
    std::vector<AudioFormat> supported_formats;
    int min_channels;
    int max_channels;
    int min_sample_rate;
    int max_sample_rate;
    bool supports_exclusive_mode;
    bool supports_loopback;
    int min_buffer_size_ms;
    int max_buffer_size_ms;
};

/**
 * @brief Extended device information
 */
struct DeviceInfo {
    std::string id;
    std::string friendly_name;
    std::string description;
    std::string manufacturer;
    std::string driver_version;
    bool is_default;
    bool is_default_communications;
    bool is_enabled;
    bool is_present;
    DeviceCapabilities capabilities;
};

/**
 * @brief Device state
 */
enum class DeviceState {
    ACTIVE,
    DISABLED,
    NOT_PRESENT,
    UNPLUGGED
};

/**
 * @brief Audio device type
 */
enum class DeviceType {
    CAPTURE,     // Microphone/Line-in
    RENDER,      // Speakers/Headphones
    LOOPBACK     // System audio capture
};

/**
 * @brief Device manager for Windows audio devices
 */
class DeviceManager {
public:
    /**
     * @brief Device change callback
     * @param device_id Changed device ID
     * @param state New device state
     */
    using DeviceChangeCallback = std::function<void(const std::string& device_id, DeviceState state)>;
    
    /**
     * @brief Default device change callback
     * @param device_type Type of default device that changed
     * @param device_id New default device ID
     */
    using DefaultDeviceChangeCallback = std::function<void(DeviceType device_type, const std::string& device_id)>;

public:
    DeviceManager();
    ~DeviceManager();
    
    // Prevent copying
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;
    
    /**
     * @brief Initialize device manager
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown device manager
     */
    void shutdown();
    
    /**
     * @brief Get all audio devices of specified type
     * @param type Device type to enumerate
     * @return Vector of device information
     */
    std::vector<DeviceInfo> getDevices(DeviceType type) const;
    
    /**
     * @brief Get specific device information
     * @param device_id Device ID
     * @return Device information (empty if not found)
     */
    DeviceInfo getDeviceInfo(const std::string& device_id) const;
    
    /**
     * @brief Get default device for specified type
     * @param type Device type
     * @return Default device info
     */
    DeviceInfo getDefaultDevice(DeviceType type) const;
    
    /**
     * @brief Get default communications device
     * @param type Device type
     * @return Default communications device info
     */
    DeviceInfo getDefaultCommunicationsDevice(DeviceType type) const;
    
    /**
     * @brief Check if device supports specific format
     * @param device_id Device ID
     * @param format Audio format to check
     * @return true if format is supported
     */
    bool isFormatSupported(const std::string& device_id, const AudioFormat& format) const;
    
    /**
     * @brief Get best matching format for device
     * @param device_id Device ID
     * @param preferred_format Preferred format
     * @return Best matching supported format
     */
    AudioFormat getBestMatchingFormat(const std::string& device_id, 
                                     const AudioFormat& preferred_format) const;
    
    /**
     * @brief Test device functionality
     * @param device_id Device ID
     * @param duration_ms Test duration in milliseconds
     * @return true if device is working
     */
    bool testDevice(const std::string& device_id, int duration_ms = 1000);
    
    /**
     * @brief Get device state
     * @param device_id Device ID
     * @return Current device state
     */
    DeviceState getDeviceState(const std::string& device_id) const;
    
    /**
     * @brief Enable device state monitoring
     * @param enable true to enable monitoring
     */
    void enableMonitoring(bool enable);
    
    /**
     * @brief Set device change callback
     * @param callback Callback function
     */
    void setDeviceChangeCallback(DeviceChangeCallback callback);
    
    /**
     * @brief Set default device change callback
     * @param callback Callback function
     */
    void setDefaultDeviceChangeCallback(DefaultDeviceChangeCallback callback);
    
    /**
     * @brief Refresh device list
     */
    void refreshDevices();
    
    /**
     * @brief Get device volume
     * @param device_id Device ID
     * @return Volume level (0.0 to 1.0), -1 if error
     */
    float getDeviceVolume(const std::string& device_id) const;
    
    /**
     * @brief Set device volume
     * @param device_id Device ID
     * @param volume Volume level (0.0 to 1.0)
     * @return true if successful
     */
    bool setDeviceVolume(const std::string& device_id, float volume);
    
    /**
     * @brief Check if device is muted
     * @param device_id Device ID
     * @return true if muted
     */
    bool isDeviceMuted(const std::string& device_id) const;
    
    /**
     * @brief Set device mute state
     * @param device_id Device ID
     * @param mute true to mute, false to unmute
     * @return true if successful
     */
    bool setDeviceMuted(const std::string& device_id, bool mute);
    
    /**
     * @brief Get device latency
     * @param device_id Device ID
     * @return Latency in milliseconds, -1 if error
     */
    int getDeviceLatency(const std::string& device_id) const;
    
    /**
     * @brief Get audio session information
     * @param device_id Device ID
     * @return Map of session names to process IDs
     */
    std::map<std::string, int> getAudioSessions(const std::string& device_id) const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // DEVICEMANAGER_H