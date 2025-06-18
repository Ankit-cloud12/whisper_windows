/*
 * AudioCapture.cpp
 * 
 * Real WASAPI audio capture implementation for Windows
 */

// Ensure windows.h is included first for WIN32 builds
#ifdef _WIN32 // Or WIN32, ensure consistency with other files if this doesn't work
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "AudioCapture.h"
#include "Logger.h"
#include "core/ErrorCodes.h"
#include <chrono>
#include <cmath>
#include <algorithm>
#include <queue>
#include <condition_variable>

// Windows includes for WASAPI (already included above if _WIN32 is defined)
// #define WIN32_LEAN_AND_MEAN // Already defined above
// #include <windows.h> // Already included above
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>
#include <wrl/client.h>
#include <string>

using namespace WhisperApp;
using Microsoft::WRL::ComPtr;

// Utility function to convert wide string to string
std::string WideStringToString(LPCWSTR wide_string) {
    if (!wide_string) return "";
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide_string, -1, NULL, 0, NULL, NULL);
    if (size_needed <= 0) return "";
    
    std::string result(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide_string, -1, &result[0], size_needed, NULL, NULL);
    return result;
}

// Utility function to convert string to wide string
std::wstring StringToWideString(const std::string& str) {
    if (str.empty()) return L"";
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (size_needed <= 0) return L"";
    
    std::wstring result(size_needed - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size_needed);
    return result;
}

// Ring buffer for audio data
class RingBuffer {
public:
    RingBuffer(size_t size) : buffer_(size), write_pos_(0), read_pos_(0), size_(0) {}
    
    bool write(const float* data, size_t count) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (size_ + count > buffer_.size()) {
            return false; // Buffer overflow
        }
        
        for (size_t i = 0; i < count; ++i) {
            buffer_[write_pos_] = data[i];
            write_pos_ = (write_pos_ + 1) % buffer_.size();
        }
        size_ += count;
        condition_.notify_one();
        return true;
    }
    
    size_t read(float* data, size_t count) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this, count] { return size_ >= count || !running_; });
        
        size_t to_read = std::min(count, size_);
        for (size_t i = 0; i < to_read; ++i) {
            data[i] = buffer_[read_pos_];
            read_pos_ = (read_pos_ + 1) % buffer_.size();
        }
        size_ -= to_read;
        return to_read;
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        write_pos_ = 0;
        read_pos_ = 0;
        size_ = 0;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }
    
    void stop() {
        running_ = false;
        condition_.notify_all();
    }
    
private:
    std::vector<float> buffer_;
    size_t write_pos_;
    size_t read_pos_;
    size_t size_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> running_{true};
};

// Audio resampler for format conversion
class AudioResampler {
public:
    AudioResampler(int input_rate, int output_rate, int channels)
        : input_rate_(input_rate), output_rate_(output_rate), channels_(channels) {
        ratio_ = static_cast<double>(output_rate) / input_rate;
    }
    
    std::vector<float> resample(const float* input, size_t input_frames) {
        if (input_rate_ == output_rate_) {
            return std::vector<float>(input, input + input_frames * channels_);
        }
        
        size_t output_frames = static_cast<size_t>(input_frames * ratio_);
        std::vector<float> output(output_frames * channels_);
        
        for (size_t i = 0; i < output_frames; ++i) {
            double src_pos = i / ratio_;
            size_t src_idx = static_cast<size_t>(src_pos);
            double frac = src_pos - src_idx;
            
            if (src_idx + 1 < input_frames) {
                // Linear interpolation
                for (int ch = 0; ch < channels_; ++ch) {
                    float sample1 = input[src_idx * channels_ + ch];
                    float sample2 = input[(src_idx + 1) * channels_ + ch];
                    output[i * channels_ + ch] = sample1 + frac * (sample2 - sample1);
                }
            } else if (src_idx < input_frames) {
                // Last sample
                for (int ch = 0; ch < channels_; ++ch) {
                    output[i * channels_ + ch] = input[src_idx * channels_ + ch];
                }
            }
        }
        
        return output;
    }
    
private:
    int input_rate_;
    int output_rate_;
    int channels_;
    double ratio_;
};

// Convert to mono if needed
std::vector<float> convertToMono(const float* input, size_t frames, int channels) {
    if (channels == 1) {
        return std::vector<float>(input, input + frames);
    }
    
    std::vector<float> output(frames);
    for (size_t i = 0; i < frames; ++i) {
        float sum = 0.0f;
        for (int ch = 0; ch < channels; ++ch) {
            sum += input[i * channels + ch];
        }
        output[i] = sum / channels;
    }
    return output;
}

// Private implementation class
class AudioCapture::Impl {
public:
    Impl() : ring_buffer_(48000 * 2 * 10) { // 10 seconds at 48kHz stereo
        // Initialize COM
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            // This is in constructor, throwing here can be problematic if not handled carefully.
            // For now, we log and set a flag. initialize() will check this flag.
            LOG_ERROR("AudioCapture", "Failed to initialize COM: " + std::to_string(hr));
            com_initialized_ = false;
        } else {
            com_initialized_ = true;
        }
    }
    
    ~Impl() {
        shutdown();
        if (com_initialized_) {
            CoUninitialize();
        }
    }
    
    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (initialized_) {
            return true;
        }
        
        if (!com_initialized_) {
            LOG_ERROR("AudioCapture", "COM not initialized during construction.");
            // No throw here, allow initialize() to fail more gracefully or throw.
            return false;
        }
        
        LOG_INFO("AudioCapture", "Initializing WASAPI audio capture system");
        
        // Create device enumerator
        HRESULT hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            reinterpret_cast<void**>(device_enumerator_.GetAddressOf())
        );
        
        if (FAILED(hr)) {
            std::string msg = "Failed to create device enumerator. HRESULT: " + std::to_string(hr);
            LOG_ERROR("AudioCapture", msg);
            throw AudioException(ErrorCode::SystemResourceUnavailable, msg);
        }
        
        initialized_ = true;
        
        // Start device monitoring thread
        device_monitor_thread_ = std::thread(&Impl::deviceMonitorThread, this);
        
        LOG_INFO("AudioCapture", "WASAPI audio capture system initialized");
        return true;
    }
    
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!initialized_) {
                return;
            }
            
            stopCapture();
            monitoring_devices_ = false;
        }
        
        if (device_monitor_thread_.joinable()) {
            device_monitor_thread_.join();
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        device_enumerator_.Reset();
        initialized_ = false;
        LOG_INFO("AudioCapture", "WASAPI audio capture system shut down");
    }
    
    std::vector<AudioDevice> getAudioDevices() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<AudioDevice> devices;
        
        if (!device_enumerator_) {
            return devices;
        }
        
        // Enumerate input devices
        ComPtr<IMMDeviceCollection> device_collection;
        HRESULT hr = device_enumerator_->EnumAudioEndpoints(
            eCapture, DEVICE_STATE_ACTIVE, device_collection.GetAddressOf()
        );
        
        if (FAILED(hr)) {
            std::string msg = "Failed to enumerate audio devices. HRESULT: " + std::to_string(hr);
            LOG_ERROR("AudioCapture", msg);
            // Not throwing here as getAudioDevices might be called for UI listing,
            // allow returning empty list. Critical failure would be GetDefault or SetDevice.
            return devices;
        }
        
        UINT device_count;
        hr = device_collection->GetCount(&device_count);
        if (FAILED(hr)) {
            LOG_WARN("AudioCapture", "Failed to get device count. HRESULT: " + std::to_string(hr));
            return devices;
        }
        
        // Get default device
        ComPtr<IMMDevice> default_device;
        device_enumerator_->GetDefaultAudioEndpoint(eCapture, eConsole, default_device.GetAddressOf());
        
        LPWSTR default_device_id = nullptr;
        if (default_device) {
            default_device->GetId(&default_device_id);
        }
        
        for (UINT i = 0; i < device_count; ++i) {
            ComPtr<IMMDevice> device;
            hr = device_collection->Item(i, device.GetAddressOf());
            if (FAILED(hr)) continue;
            
            AudioDevice audio_device = createAudioDeviceInfo(device.Get());
            
            // Check if this is the default device
            if (default_device_id && audio_device.id == WideStringToString(default_device_id)) {
                audio_device.is_default = true;
            }
            
            devices.push_back(audio_device);
        }
        
        if (default_device_id) {
            CoTaskMemFree(default_device_id);
        }
        
        // Add loopback devices (render endpoints for system audio capture)
        if (loopback_enabled_) {
            ComPtr<IMMDeviceCollection> render_collection;
            hr = device_enumerator_->EnumAudioEndpoints(
                eRender, DEVICE_STATE_ACTIVE, render_collection.GetAddressOf()
            );
            
            if (SUCCEEDED(hr)) {
                UINT render_count;
                hr = render_collection->GetCount(&render_count);
                if (SUCCEEDED(hr)) {
                    for (UINT i = 0; i < render_count; ++i) {
                        ComPtr<IMMDevice> device;
                        hr = render_collection->Item(i, device.GetAddressOf());
                        if (FAILED(hr)) continue;
                        
                        AudioDevice audio_device = createAudioDeviceInfo(device.Get());
                        audio_device.is_loopback = true;
                        audio_device.name += " (Loopback)";
                        audio_device.id += "_loopback";
                        
                        devices.push_back(audio_device);
                    }
                }
            }
        }
        
        return devices;
    }
    
    AudioDevice getDefaultDevice() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!device_enumerator_) {
            return {};
        }
        
        ComPtr<IMMDevice> device;
        HRESULT hr = device_enumerator_->GetDefaultAudioEndpoint(
            eCapture, eConsole, device.GetAddressOf()
        );
        
        if (FAILED(hr)) {
            std::string msg = "Failed to get default audio device. HRESULT: " + std::to_string(hr);
            LOG_ERROR("AudioCapture", msg);
            throw AudioException(ErrorCode::SystemResourceUnavailable, msg);
        }
        
        AudioDevice audio_device = createAudioDeviceInfo(device.Get());
        audio_device.is_default = true;
        return audio_device;
    }
    
    bool setDevice(const std::string& device_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (current_device_id_ == device_id) {
            return true; // Already set
        }
        
        // Check if device exists
        auto devices = getAudioDevices();
        bool found = false;
        for (const auto& device : devices) {
            if (device.id == device_id) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            std::string msg = "Selected audio device not found: " + device_id;
            LOG_ERROR("AudioCapture", msg);
            throw AudioException(ErrorCode::AudioSampleRateInvalid, msg); // Using this as a placeholder for "device not found"
        }
        
        current_device_id_ = device_id;
        LOG_INFO("AudioCapture", "Set audio device: " + device_id);
        
        // If capturing, restart with new device
        if (capturing_) {
            stopCaptureInternal();
            return startCaptureInternal();
        }
        
        return true;
    }
    
    std::string getCurrentDeviceId() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return current_device_id_;
    }
    
    bool startCapture(AudioCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (capturing_) {
            return true;
        }
        
        audio_callback_ = callback;
        return startCaptureInternal();
    }
    
    void stopCapture() {
        std::lock_guard<std::mutex> lock(mutex_);
        stopCaptureInternal();
    }
    
    bool isCapturing() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return capturing_;
    }
    
    std::vector<float> getCapturedAudio() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return captured_buffer_;
    }
    
    void clearBuffer() {
        std::lock_guard<std::mutex> lock(mutex_);
        captured_buffer_.clear();
        ring_buffer_.clear();
        stats_.total_samples = 0;
    }
    
    float getAudioLevel() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return current_level_;
    }
    
    void setLevelCallback(LevelCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        level_callback_ = callback;
    }
    
    void setDeviceChangeCallback(DeviceChangeCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        device_change_callback_ = callback;
    }
    
    void setLoopbackEnabled(bool enable) {
        std::lock_guard<std::mutex> lock(mutex_);
        loopback_enabled_ = enable;
    }
    
    bool isLoopbackEnabled() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return loopback_enabled_;
    }
    
    AudioCapture::CaptureStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }
    
    void resetStats() {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_ = {};
    }
    
    void setConfig(const AudioCaptureConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
    }
    
    AudioCaptureConfig getConfig() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }
    
private:
    AudioDevice createAudioDeviceInfo(IMMDevice* device) const {
        AudioDevice audio_device;
        
        // Get device ID
        LPWSTR device_id;
        HRESULT hr = device->GetId(&device_id);
        if (SUCCEEDED(hr)) {
            audio_device.id = WideStringToString(device_id);
            CoTaskMemFree(device_id);
        }
        
        // Get device properties
        ComPtr<IPropertyStore> props;
        hr = device->OpenPropertyStore(STGM_READ, props.GetAddressOf());
        if (SUCCEEDED(hr)) {
            PROPVARIANT var_name;
            PropVariantInit(&var_name);
            
            hr = props->GetValue(PKEY_Device_FriendlyName, &var_name);
            if (SUCCEEDED(hr) && var_name.vt == VT_LPWSTR) {
                audio_device.name = WideStringToString(var_name.pwszVal);
            }
            PropVariantClear(&var_name);
        }
        
        // Get audio format info
        ComPtr<IAudioClient> audio_client;
        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                             reinterpret_cast<void**>(audio_client.GetAddressOf()));
        if (SUCCEEDED(hr)) {
            WAVEFORMATEX* format;
            hr = audio_client->GetMixFormat(&format);
            if (SUCCEEDED(hr)) {
                audio_device.channels = format->nChannels;
                audio_device.sample_rate = format->nSamplesPerSec;
                CoTaskMemFree(format);
            }
        }
        
        return audio_device;
    }
    
    bool startCaptureInternal() {
        if (current_device_id_.empty()) {
            try {
                auto default_device = getDefaultDevice(); // This can throw
                if (default_device.id.empty()) { // Should not happen if getDefaultDevice throws
                    std::string msg = "No default audio device found and none specified.";
                    LOG_ERROR("AudioCapture", msg);
                    throw AudioException(ErrorCode::SystemResourceUnavailable, msg);
                }
                current_device_id_ = default_device.id;
                LOG_INFO("AudioCapture", "Using default audio device: " + current_device_id_);
            } catch (const AudioException& e) {
                LOG_ERROR("AudioCapture", "Failed to get default device for capture: " + std::string(e.what()));
                throw; // Re-throw if getDefaultDevice failed critically
            }
        }
        
        // Get the device
        ComPtr<IMMDevice> device;
        HRESULT hr;
        
        if (current_device_id_.find("_loopback") != std::string::npos) {
            // Loopback device - get render endpoint
            std::string render_id = current_device_id_;
            render_id = render_id.substr(0, render_id.find("_loopback"));
            
            std::wstring wide_id = StringToWideString(render_id);
            hr = device_enumerator_->GetDevice(wide_id.c_str(), device.GetAddressOf());
        } else {
            // Regular capture device
            std::wstring wide_id = StringToWideString(current_device_id_);
            hr = device_enumerator_->GetDevice(wide_id.c_str(), device.GetAddressOf());
        }
        
        if (FAILED(hr)) {
            std::string msg = "Failed to get specified audio device " + current_device_id_ + ". HRESULT: " + std::to_string(hr);
            LOG_ERROR("AudioCapture", msg);
            throw AudioException(ErrorCode::SystemResourceUnavailable, msg);
        }
        
        // Activate audio client
        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                             reinterpret_cast<void**>(audio_client_.GetAddressOf()));
        if (FAILED(hr)) {
            std::string msg = "Failed to activate audio client for device " + current_device_id_ + ". HRESULT: " + std::to_string(hr);
            LOG_ERROR("AudioCapture", msg);
            throw AudioException(ErrorCode::SystemResourceUnavailable, msg);
        }
        
        // Get mix format
        WAVEFORMATEX* mix_format;
        hr = audio_client_->GetMixFormat(&mix_format);
        if (FAILED(hr)) {
            std::string msg = "Failed to get mix format for device " + current_device_id_ + ". HRESULT: " + std::to_string(hr);
            LOG_ERROR("AudioCapture", msg);
            audio_client_.Reset(); // Release client
            throw AudioException(ErrorCode::AudioFormatUnsupported, msg);
        }
        
        // Store format info
        native_sample_rate_ = mix_format->nSamplesPerSec;
        native_channels_ = mix_format->nChannels;
        
        // Initialize audio client
        AUDCLNT_SHAREMODE share_mode = AUDCLNT_SHAREMODE_SHARED;
        DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
        
        if (current_device_id_.find("_loopback") != std::string::npos) {
            flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;
        }
        
        REFERENCE_TIME buffer_duration = 10000000; // 1 second in 100ns units
        
        hr = audio_client_->Initialize(share_mode, flags, buffer_duration, 0, mix_format, nullptr);
        if (FAILED(hr)) {
            std::string msg = "Failed to initialize audio client for device " + current_device_id_ + ". HRESULT: " + std::to_string(hr);
            LOG_ERROR("AudioCapture", msg);
            CoTaskMemFree(mix_format);
            audio_client_.Reset();
            throw AudioException(ErrorCode::SystemResourceUnavailable, msg);
        }
        
        CoTaskMemFree(mix_format);
        mix_format = nullptr;
        
        // Create event for audio data available
        audio_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!audio_event_) {
            std::string msg = "Failed to create audio event for device " + current_device_id_;
            LOG_ERROR("AudioCapture", msg);
            audio_client_.Reset();
            throw AudioException(ErrorCode::SystemResourceUnavailable, msg);
        }
        
        hr = audio_client_->SetEventHandle(audio_event_);
        if (FAILED(hr)) {
            std::string msg = "Failed to set event handle for device " + current_device_id_ + ". HRESULT: " + std::to_string(hr);
            LOG_ERROR("AudioCapture", msg);
            CloseHandle(audio_event_);
            audio_event_ = nullptr;
            audio_client_.Reset();
            throw AudioException(ErrorCode::SystemResourceUnavailable, msg);
        }
        
        // Get capture client
        hr = audio_client_->GetService(__uuidof(IAudioCaptureClient),
                                      reinterpret_cast<void**>(capture_client_.GetAddressOf()));
        if (FAILED(hr)) {
            std::string msg = "Failed to get capture client for device " + current_device_id_ + ". HRESULT: " + std::to_string(hr);
            LOG_ERROR("AudioCapture", msg);
            CloseHandle(audio_event_);
            audio_event_ = nullptr;
            audio_client_.Reset();
            throw AudioException(ErrorCode::SystemResourceUnavailable, msg);
        }
        
        // Create resampler
        resampler_ = std::make_unique<AudioResampler>(native_sample_rate_, config_.sample_rate, native_channels_);
        
        // Start audio client
        hr = audio_client_->Start();
        if (FAILED(hr)) {
            std::string msg = "Failed to start audio client for device " + current_device_id_ + ". HRESULT: " + std::to_string(hr);
            LOG_ERROR("AudioCapture", msg);
            capture_client_.Reset();
            CloseHandle(audio_event_);
            audio_event_ = nullptr;
            audio_client_.Reset();
            throw AudioException(ErrorCode::SystemResourceUnavailable, msg);
        }
        
        capturing_ = true;
        capture_thread_ = std::thread(&Impl::captureThread, this);
        processing_thread_ = std::thread(&Impl::processingThread, this);
        
        LOG_INFO("AudioCapture", "Started WASAPI audio capture");
        return true;
    }
    
    void stopCaptureInternal() {
        if (!capturing_) {
            return;
        }
        
        capturing_ = false;
        ring_buffer_.stop();
        
        if (audio_client_) {
            audio_client_->Stop();
        }
        
        if (audio_event_) {
            SetEvent(audio_event_); // Wake up capture thread
        }
        
        if (capture_thread_.joinable()) {
            capture_thread_.join();
        }
        if (processing_thread_.joinable()) {
            processing_thread_.join();
        }
        
        if (audio_event_) {
            CloseHandle(audio_event_);
            audio_event_ = nullptr;
        }
        
        capture_client_.Reset();
        audio_client_.Reset();
        resampler_.reset();
        
        LOG_INFO("AudioCapture", "Stopped WASAPI audio capture");
    }
    
    void captureThread() {
        while (capturing_) {
            DWORD wait_result = WaitForSingleObject(audio_event_, 100); // 100ms timeout
            
            if (wait_result != WAIT_OBJECT_0) {
                if (!capturing_) break;
                continue;
            }
            
            UINT32 packet_length = 0;
            HRESULT hr = capture_client_->GetNextPacketSize(&packet_length);
            if (FAILED(hr)) {
                LOG_ERROR("AudioCapture", "Failed to get packet size: " + std::to_string(hr));
                break;
            }
            
            while (packet_length != 0) {
                BYTE* data;
                UINT32 frames_available;
                DWORD flags;
                
                hr = capture_client_->GetBuffer(&data, &frames_available, &flags, nullptr, nullptr);
                if (FAILED(hr)) {
                    LOG_ERROR("AudioCapture", "Failed to get buffer: " + std::to_string(hr));
                    break;
                }
                
                if (frames_available > 0) {
                    // Convert to float
                    std::vector<float> float_data(frames_available * native_channels_);
                    
                    // Assume 32-bit float format (most common for WASAPI shared mode)
                    if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
                        float* float_ptr = reinterpret_cast<float*>(data);
                        std::copy(float_ptr, float_ptr + frames_available * native_channels_, float_data.begin());
                    } else {
                        std::fill(float_data.begin(), float_data.end(), 0.0f);
                    }
                    
                    // Resample to target rate
                    auto resampled = resampler_->resample(float_data.data(), frames_available);
                    
                    // Convert to mono if needed
                    auto mono_data = convertToMono(resampled.data(), resampled.size() / native_channels_, native_channels_);
                    
                    // Write to ring buffer
                    if (!ring_buffer_.write(mono_data.data(), mono_data.size())) {
                        stats_.dropped_samples += mono_data.size();
                        stats_.buffer_overruns++;
                        LOG_WARN("AudioCapture", "Audio buffer overflow");
                    }
                }
                
                hr = capture_client_->ReleaseBuffer(frames_available);
                if (FAILED(hr)) {
                    LOG_ERROR("AudioCapture", "Failed to release buffer: " + std::to_string(hr));
                    break;
                }
                
                hr = capture_client_->GetNextPacketSize(&packet_length);
                if (FAILED(hr)) {
                    break;
                }
            }
        }
    }
    
    void processingThread() {
        const int process_frames = config_.sample_rate * config_.buffer_size_ms / 1000;
        std::vector<float> process_buffer(process_frames);
        
        float silence_duration = 0.0f;
        const float silence_threshold_squared = config_.silence_threshold * config_.silence_threshold;
        
        while (capturing_) {
            size_t read_samples = ring_buffer_.read(process_buffer.data(), process_buffer.size());
            if (read_samples == 0) {
                continue;
            }
            
            // Calculate audio level (RMS)
            float sum_squares = 0.0f;
            for (size_t i = 0; i < read_samples; ++i) {
                sum_squares += process_buffer[i] * process_buffer[i];
            }
            float rms = std::sqrt(sum_squares / read_samples);
            current_level_ = std::min(1.0f, rms);
            
            // Notify level callback
            if (level_callback_) {
                level_callback_(current_level_);
            }
            
            // Update statistics
            stats_.total_samples += read_samples;
            stats_.average_level = (stats_.average_level * 0.95f) + (current_level_ * 0.05f);
            
            // Silence detection
            if (config_.enable_silence_detection) {
                if (sum_squares / read_samples < silence_threshold_squared) {
                    silence_duration += (read_samples / float(config_.sample_rate));
                    
                    if (silence_duration >= config_.silence_duration_ms / 1000.0f) {
                        LOG_INFO("AudioCapture", "Silence detected, stopping capture");
                        capturing_ = false;
                        break;
                    }
                } else {
                    silence_duration = 0.0f;
                }
            }
            
            // Basic noise suppression (spectral subtraction)
            if (config_.enable_noise_suppression) {
                for (size_t i = 0; i < read_samples; ++i) {
                    // Simple noise gate
                    if (std::abs(process_buffer[i]) < 0.01f) {
                        process_buffer[i] = 0.0f;
                    }
                }
            }
            
            // Store in capture buffer
            captured_buffer_.insert(captured_buffer_.end(), 
                                  process_buffer.begin(), 
                                  process_buffer.begin() + read_samples);
            
            // Notify audio callback
            if (audio_callback_) {
                audio_callback_(process_buffer.data(), read_samples);
            }
        }
    }
    
    void deviceMonitorThread() {
        monitoring_devices_ = true;
        
        while (monitoring_devices_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // TODO: Implement proper device change detection using IMMNotificationClient
            // For now, just periodically check if current device is still valid
            if (!current_device_id_.empty()) {
                auto devices = getAudioDevices();
                bool device_found = false;
                for (const auto& device : devices) {
                    if (device.id == current_device_id_) {
                        device_found = true;
                        break;
                    }
                }
                
                if (!device_found && device_change_callback_) {
                    LOG_INFO("AudioCapture", "Current audio device disconnected");
                    device_change_callback_();
                }
            }
        }
    }
    
private:
    mutable std::mutex mutex_;
    bool com_initialized_ = false;
    bool initialized_ = false;
    bool capturing_ = false;
    bool loopback_enabled_ = false;
    std::atomic<bool> monitoring_devices_{false};
    
    AudioCaptureConfig config_;
    std::string current_device_id_;
    
    ComPtr<IMMDeviceEnumerator> device_enumerator_;
    ComPtr<IAudioClient> audio_client_;
    ComPtr<IAudioCaptureClient> capture_client_;
    
    HANDLE audio_event_ = nullptr;
    int native_sample_rate_ = 48000;
    int native_channels_ = 2;
    
    std::unique_ptr<AudioResampler> resampler_;
    
    std::thread capture_thread_;
    std::thread processing_thread_;
    std::thread device_monitor_thread_;
    
    RingBuffer ring_buffer_;
    std::vector<float> captured_buffer_;
    std::atomic<float> current_level_{0.0f};
    
    AudioCallback audio_callback_;
    LevelCallback level_callback_;
    DeviceChangeCallback device_change_callback_;
    
    AudioCapture::CaptureStats stats_;
};

// AudioCapture public methods
AudioCapture::AudioCapture() : pImpl(std::make_unique<Impl>()) {}
AudioCapture::~AudioCapture() = default;

bool AudioCapture::initialize() {
    return pImpl->initialize();
}

void AudioCapture::shutdown() {
    pImpl->shutdown();
}

std::vector<AudioDevice> AudioCapture::getAudioDevices() const {
    return pImpl->getAudioDevices();
}

AudioDevice AudioCapture::getDefaultDevice() const {
    return pImpl->getDefaultDevice();
}

bool AudioCapture::setDevice(const std::string& device_id) {
    return pImpl->setDevice(device_id);
}

std::string AudioCapture::getCurrentDeviceId() const {
    return pImpl->getCurrentDeviceId();
}

void AudioCapture::setConfig(const AudioCaptureConfig& config) {
    pImpl->setConfig(config);
}

AudioCaptureConfig AudioCapture::getConfig() const {
    return pImpl->getConfig();
}

bool AudioCapture::startCapture(AudioCallback callback) {
    return pImpl->startCapture(callback);
}

void AudioCapture::stopCapture() {
    pImpl->stopCapture();
}

bool AudioCapture::isCapturing() const {
    return pImpl->isCapturing();
}

std::vector<float> AudioCapture::getCapturedAudio() const {
    return pImpl->getCapturedAudio();
}

void AudioCapture::clearBuffer() {
    pImpl->clearBuffer();
}

float AudioCapture::getAudioLevel() const {
    return pImpl->getAudioLevel();
}

void AudioCapture::setLevelCallback(LevelCallback callback) {
    pImpl->setLevelCallback(callback);
}

void AudioCapture::setDeviceChangeCallback(DeviceChangeCallback callback) {
    pImpl->setDeviceChangeCallback(callback);
}

void AudioCapture::setLoopbackEnabled(bool enable) {
    pImpl->setLoopbackEnabled(enable);
}

bool AudioCapture::isLoopbackEnabled() const {
    return pImpl->isLoopbackEnabled();
}

AudioCapture::CaptureStats AudioCapture::getStats() const {
    return pImpl->getStats();
}

void AudioCapture::resetStats() {
    pImpl->resetStats();
}