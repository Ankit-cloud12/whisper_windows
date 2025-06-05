/*
 * DeviceManager.cpp
 * 
 * Implementation of Windows audio device management
 */

#include "DeviceManager.h"
#include "Logger.h"
#include <thread>
#include <algorithm>
#include <chrono>
#include <mutex>
#include <atomic>
#include <map>

using namespace WhisperApp;

// Mock Windows multimedia device API structures
struct MockMMDevice {
    std::string id;
    std::string name;
    DeviceType type;
    DeviceState state;
    DeviceCapabilities capabilities;
    float volume;
    bool muted;
};

// Private implementation
class DeviceManager::Impl {
public:
    Impl() {
        // Initialize mock devices
        initializeMockDevices();
    }
    
    ~Impl() {
        shutdown();
    }
    
    bool initialize() {
        if (initialized_) return true;
        
        LOG_INFO("DeviceManager", "Initializing device manager");
        
        // Start monitoring thread if enabled
        if (monitoring_enabled_) {
            startMonitoring();
        }
        
        initialized_ = true;
        return true;
    }
    
    void shutdown() {
        if (!initialized_) return;
        
        stopMonitoring();
        initialized_ = false;
        
        LOG_INFO("DeviceManager", "Device manager shut down");
    }
    
    std::vector<DeviceInfo> getDevices(DeviceType type) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<DeviceInfo> devices;
        
        for (const auto& [id, device] : mock_devices_) {
            if (device.type == type || 
                (type == DeviceType::LOOPBACK && device.type == DeviceType::RENDER)) {
                devices.push_back(convertToDeviceInfo(device));
            }
        }
        
        return devices;
    }
    
    DeviceInfo getDeviceInfo(const std::string& device_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = mock_devices_.find(device_id);
        if (it != mock_devices_.end()) {
            return convertToDeviceInfo(it->second);
        }
        
        return {};
    }
    
    DeviceInfo getDefaultDevice(DeviceType type) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (const auto& [id, device] : mock_devices_) {
            if (device.type == type && device.name.find("Default") != std::string::npos) {
                return convertToDeviceInfo(device);
            }
        }
        
        return {};
    }
    
    bool isFormatSupported(const std::string& device_id, const AudioFormat& format) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = mock_devices_.find(device_id);
        if (it == mock_devices_.end()) return false;
        
        const auto& formats = it->second.capabilities.supported_formats;
        return std::find(formats.begin(), formats.end(), format) != formats.end();
    }
    
    AudioFormat getBestMatchingFormat(const std::string& device_id, 
                                     const AudioFormat& preferred) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = mock_devices_.find(device_id);
        if (it == mock_devices_.end()) return {};
        
        const auto& caps = it->second.capabilities;
        
        // Try exact match first
        if (isFormatSupported(device_id, preferred)) {
            return preferred;
        }
        
        // Find closest match
        AudioFormat best;
        int best_score = -1;
        
        for (const auto& format : caps.supported_formats) {
            int score = 0;
            
            // Prefer matching sample rate
            if (format.sample_rate == preferred.sample_rate) score += 10;
            else score -= std::abs(format.sample_rate - preferred.sample_rate) / 1000;
            
            // Prefer matching channels
            if (format.channels == preferred.channels) score += 5;
            else score -= std::abs(format.channels - preferred.channels);
            
            // Prefer matching bit depth
            if (format.bits_per_sample == preferred.bits_per_sample) score += 3;
            
            if (score > best_score) {
                best_score = score;
                best = format;
            }
        }
        
        return best;
    }
    
    bool testDevice(const std::string& device_id, int duration_ms) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = mock_devices_.find(device_id);
        if (it == mock_devices_.end()) return false;
        
        LOG_INFO("DeviceManager", "Testing device: " + it->second.name);
        
        // Simulate device test
        std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
        
        // Mock: devices always pass test unless unplugged
        return it->second.state != DeviceState::NOT_PRESENT && 
               it->second.state != DeviceState::UNPLUGGED;
    }
    
    DeviceState getDeviceState(const std::string& device_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = mock_devices_.find(device_id);
        if (it != mock_devices_.end()) {
            return it->second.state;
        }
        
        return DeviceState::NOT_PRESENT;
    }
    
    void enableMonitoring(bool enable) {
        if (enable && !monitoring_enabled_) {
            monitoring_enabled_ = true;
            if (initialized_) {
                startMonitoring();
            }
        } else if (!enable && monitoring_enabled_) {
            monitoring_enabled_ = false;
            stopMonitoring();
        }
    }
    
    void setDeviceChangeCallback(DeviceChangeCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        device_change_callback_ = callback;
    }
    
    void setDefaultDeviceChangeCallback(DefaultDeviceChangeCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        default_device_change_callback_ = callback;
    }
    
    void refreshDevices() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Simulate device refresh
        LOG_INFO("DeviceManager", "Refreshing device list");
        
        // Mock: occasionally change device states
        static int refresh_count = 0;
        if (++refresh_count % 5 == 0) {
            simulateDeviceChange();
        }
    }
    
    float getDeviceVolume(const std::string& device_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = mock_devices_.find(device_id);
        if (it != mock_devices_.end()) {
            return it->second.volume;
        }
        
        return -1.0f;
    }
    
    bool setDeviceVolume(const std::string& device_id, float volume) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = mock_devices_.find(device_id);
        if (it != mock_devices_.end()) {
            it->second.volume = std::max(0.0f, std::min(1.0f, volume));
            LOG_INFO("DeviceManager", "Set volume for " + it->second.name + 
                    " to " + std::to_string(volume));
            return true;
        }
        
        return false;
    }
    
    bool isDeviceMuted(const std::string& device_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = mock_devices_.find(device_id);
        if (it != mock_devices_.end()) {
            return it->second.muted;
        }
        
        return false;
    }
    
    bool setDeviceMuted(const std::string& device_id, bool mute) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = mock_devices_.find(device_id);
        if (it != mock_devices_.end()) {
            it->second.muted = mute;
            LOG_INFO("DeviceManager", (mute ? "Muted " : "Unmuted ") + it->second.name);
            return true;
        }
        
        return false;
    }
    
    int getDeviceLatency(const std::string& device_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = mock_devices_.find(device_id);
        if (it != mock_devices_.end()) {
            // Mock latency based on device type
            if (it->second.type == DeviceType::LOOPBACK) {
                return 20; // Lower latency for loopback
            }
            return 10; // Standard latency
        }
        
        return -1;
    }
    
    std::map<std::string, int> getAudioSessions(const std::string& device_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Mock audio sessions
        std::map<std::string, int> sessions;
        
        auto it = mock_devices_.find(device_id);
        if (it != mock_devices_.end() && it->second.type == DeviceType::RENDER) {
            sessions["System Sounds"] = 0;
            sessions["Chrome.exe"] = 1234;
            sessions["Spotify.exe"] = 5678;
        }
        
        return sessions;
    }
    
private:
    void initializeMockDevices() {
        // Create mock audio devices
        AudioFormat pcm_48k_stereo = {48000, 2, 16, "PCM"};
        AudioFormat pcm_44k_stereo = {44100, 2, 16, "PCM"};
        AudioFormat pcm_16k_mono = {16000, 1, 16, "PCM"};
        AudioFormat float_48k_stereo = {48000, 2, 32, "IEEE_FLOAT"};
        
        DeviceCapabilities standard_caps;
        standard_caps.supported_formats = {pcm_48k_stereo, pcm_44k_stereo, pcm_16k_mono, float_48k_stereo};
        standard_caps.min_channels = 1;
        standard_caps.max_channels = 2;
        standard_caps.min_sample_rate = 8000;
        standard_caps.max_sample_rate = 48000;
        standard_caps.supports_exclusive_mode = true;
        standard_caps.supports_loopback = false;
        standard_caps.min_buffer_size_ms = 10;
        standard_caps.max_buffer_size_ms = 500;
        
        DeviceCapabilities render_caps = standard_caps;
        render_caps.supports_loopback = true;
        
        // Default microphone
        mock_devices_["default_mic"] = {
            "default_mic",
            "Default Microphone",
            DeviceType::CAPTURE,
            DeviceState::ACTIVE,
            standard_caps,
            0.75f, // volume
            false  // not muted
        };
        
        // USB microphone
        mock_devices_["usb_mic"] = {
            "usb_mic",
            "USB Microphone",
            DeviceType::CAPTURE,
            DeviceState::ACTIVE,
            standard_caps,
            0.8f,
            false
        };
        
        // Default speakers
        mock_devices_["default_speakers"] = {
            "default_speakers",
            "Default Speakers",
            DeviceType::RENDER,
            DeviceState::ACTIVE,
            render_caps,
            0.5f,
            false
        };
        
        // Headphones
        mock_devices_["headphones"] = {
            "headphones",
            "Headphones",
            DeviceType::RENDER,
            DeviceState::DISABLED,
            render_caps,
            0.6f,
            false
        };
    }
    
    DeviceInfo convertToDeviceInfo(const MockMMDevice& device) const {
        DeviceInfo info;
        info.id = device.id;
        info.friendly_name = device.name;
        info.description = device.name + " (Mock Device)";
        info.manufacturer = "Mock Audio Inc.";
        info.driver_version = "1.0.0.0";
        info.is_default = device.name.find("Default") != std::string::npos;
        info.is_default_communications = info.is_default;
        info.is_enabled = (device.state == DeviceState::ACTIVE);
        info.is_present = (device.state != DeviceState::NOT_PRESENT);
        info.capabilities = device.capabilities;
        
        return info;
    }
    
    void startMonitoring() {
        monitoring_thread_ = std::thread(&Impl::monitoringThread, this);
    }
    
    void stopMonitoring() {
        monitoring_ = false;
        if (monitoring_thread_.joinable()) {
            monitoring_thread_.join();
        }
    }
    
    void monitoringThread() {
        monitoring_ = true;
        int cycle = 0;
        
        while (monitoring_) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            if (!monitoring_) break;
            
            // Simulate occasional device changes
            if (++cycle % 10 == 0) {
                std::lock_guard<std::mutex> lock(mutex_);
                simulateDeviceChange();
            }
        }
    }
    
    void simulateDeviceChange() {
        // Simulate headphones being plugged/unplugged
        auto& headphones = mock_devices_["headphones"];
        
        if (headphones.state == DeviceState::DISABLED) {
            headphones.state = DeviceState::ACTIVE;
            
            LOG_INFO("DeviceManager", "Headphones connected");
            
            if (device_change_callback_) {
                device_change_callback_("headphones", DeviceState::ACTIVE);
            }
            
            // Simulate default device change
            if (default_device_change_callback_) {
                default_device_change_callback_(DeviceType::RENDER, "headphones");
            }
        } else {
            headphones.state = DeviceState::DISABLED;
            
            LOG_INFO("DeviceManager", "Headphones disconnected");
            
            if (device_change_callback_) {
                device_change_callback_("headphones", DeviceState::DISABLED);
            }
            
            // Simulate default device change back to speakers
            if (default_device_change_callback_) {
                default_device_change_callback_(DeviceType::RENDER, "default_speakers");
            }
        }
    }
    
    mutable std::mutex mutex_;
    bool initialized_ = false;
    bool monitoring_enabled_ = false;
    std::atomic<bool> monitoring_{false};
    
    std::map<std::string, MockMMDevice> mock_devices_;
    std::thread monitoring_thread_;
    
    DeviceChangeCallback device_change_callback_;
    DefaultDeviceChangeCallback default_device_change_callback_;
};

// DeviceManager public implementation
DeviceManager::DeviceManager() : pImpl(std::make_unique<Impl>()) {}
DeviceManager::~DeviceManager() = default;

bool DeviceManager::initialize() {
    return pImpl->initialize();
}

void DeviceManager::shutdown() {
    pImpl->shutdown();
}

std::vector<DeviceInfo> DeviceManager::getDevices(DeviceType type) const {
    return pImpl->getDevices(type);
}

DeviceInfo DeviceManager::getDeviceInfo(const std::string& device_id) const {
    return pImpl->getDeviceInfo(device_id);
}

DeviceInfo DeviceManager::getDefaultDevice(DeviceType type) const {
    return pImpl->getDefaultDevice(type);
}

DeviceInfo DeviceManager::getDefaultCommunicationsDevice(DeviceType type) const {
    // For mock, same as default device
    return getDefaultDevice(type);
}

bool DeviceManager::isFormatSupported(const std::string& device_id, const AudioFormat& format) const {
    return pImpl->isFormatSupported(device_id, format);
}

AudioFormat DeviceManager::getBestMatchingFormat(const std::string& device_id, 
                                                const AudioFormat& preferred_format) const {
    return pImpl->getBestMatchingFormat(device_id, preferred_format);
}

bool DeviceManager::testDevice(const std::string& device_id, int duration_ms) {
    return pImpl->testDevice(device_id, duration_ms);
}

DeviceState DeviceManager::getDeviceState(const std::string& device_id) const {
    return pImpl->getDeviceState(device_id);
}

void DeviceManager::enableMonitoring(bool enable) {
    pImpl->enableMonitoring(enable);
}

void DeviceManager::setDeviceChangeCallback(DeviceChangeCallback callback) {
    pImpl->setDeviceChangeCallback(callback);
}

void DeviceManager::setDefaultDeviceChangeCallback(DefaultDeviceChangeCallback callback) {
    pImpl->setDefaultDeviceChangeCallback(callback);
}

void DeviceManager::refreshDevices() {
    pImpl->refreshDevices();
}

float DeviceManager::getDeviceVolume(const std::string& device_id) const {
    return pImpl->getDeviceVolume(device_id);
}

bool DeviceManager::setDeviceVolume(const std::string& device_id, float volume) {
    return pImpl->setDeviceVolume(device_id, volume);
}

bool DeviceManager::isDeviceMuted(const std::string& device_id) const {
    return pImpl->isDeviceMuted(device_id);
}

bool DeviceManager::setDeviceMuted(const std::string& device_id, bool mute) {
    return pImpl->setDeviceMuted(device_id, mute);
}

int DeviceManager::getDeviceLatency(const std::string& device_id) const {
    return pImpl->getDeviceLatency(device_id);
}

std::map<std::string, int> DeviceManager::getAudioSessions(const std::string& device_id) const {
    return pImpl->getAudioSessions(device_id);
}