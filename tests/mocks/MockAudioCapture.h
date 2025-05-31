/*
 * MockAudioCapture.h
 * 
 * Mock implementation of AudioCapture for testing
 */

#ifndef MOCKAUDIOCAPTURE_H
#define MOCKAUDIOCAPTURE_H

#include "core/AudioCapture.h"
#include "../TestUtils.h"
#include <functional>
#include <thread>
#include <atomic>

class MockAudioCapture : public IAudioCapture {
private:
    bool initialized_ = false;
    bool recording_ = false;
    AudioFormat format_;
    std::string deviceId_;
    AudioCallback callback_;
    std::thread audioThread_;
    std::atomic<bool> stopRequested_{false};
    float simulationFrequency_ = 440.0f;
    
public:
    MockAudioCapture() = default;
    ~MockAudioCapture() override {
        if (recording_) {
            stop();
        }
    }
    
    ErrorCode initialize(const std::string& deviceId, const AudioFormat& format) override {
        if (deviceId == "invalid_device_id") {
            return ErrorCode::AudioDeviceNotFound;
        }
        
        deviceId_ = deviceId;
        format_ = format;
        initialized_ = true;
        return ErrorCode::Success;
    }
    
    ErrorCode start() override {
        if (!initialized_) {
            return ErrorCode::AudioInitializationFailed;
        }
        
        if (recording_) {
            return ErrorCode::InvalidState;
        }
        
        recording_ = true;
        stopRequested_ = false;
        
        // Start audio generation thread
        audioThread_ = std::thread([this]() {
            const int bufferSize = 480; // 30ms at 16kHz
            std::vector<float> buffer(bufferSize);
            
            while (!stopRequested_) {
                // Generate sine wave
                for (int i = 0; i < bufferSize; ++i) {
                    float t = static_cast<float>(i) / format_.sampleRate;
                    buffer[i] = 0.5f * std::sin(2 * M_PI * simulationFrequency_ * t);
                }
                
                // Call callback
                if (callback_) {
                    callback_(buffer.data(), buffer.size());
                }
                
                // Simulate real-time delay
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
        });
        
        return ErrorCode::Success;
    }
    
    ErrorCode stop() override {
        if (!recording_) {
            return ErrorCode::InvalidState;
        }
        
        stopRequested_ = true;
        if (audioThread_.joinable()) {
            audioThread_.join();
        }
        
        recording_ = false;
        return ErrorCode::Success;
    }
    
    ErrorCode pause() override {
        // Not implemented in mock
        return ErrorCode::NotImplemented;
    }
    
    ErrorCode resume() override {
        // Not implemented in mock
        return ErrorCode::NotImplemented;
    }
    
    bool isInitialized() const override {
        return initialized_;
    }
    
    bool isRecording() const override {
        return recording_;
    }
    
    AudioFormat getFormat() const override {
        return format_;
    }
    
    void setAudioCallback(AudioCallback callback) override {
        callback_ = callback;
    }
    
    void setVolumeCallback(VolumeCallback callback) override {
        // Not implemented in mock
    }
    
    // Mock-specific methods
    void setSimulationFrequency(float frequency) {
        simulationFrequency_ = frequency;
    }
    
    void simulateError() {
        initialized_ = false;
        recording_ = false;
    }
};

#endif // MOCKAUDIOCAPTURE_H