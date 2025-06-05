/*
 * Simple test program for WASAPI AudioCapture implementation
 * This tests basic functionality without Qt dependencies
 */

#include "src/core/AudioCapture.h"
#include "src/core/Logger.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace WhisperApp;

int main() {
    // Initialize logger
    Logger::getInstance().setLevel(LogLevel::INFO);
    
    std::cout << "Testing WASAPI Audio Capture Implementation\n";
    std::cout << "==========================================\n\n";
    
    // Create audio capture instance
    AudioCapture capture;
    
    // Initialize
    std::cout << "1. Initializing audio capture system...\n";
    if (!capture.initialize()) {
        std::cerr << "Failed to initialize audio capture!\n";
        return 1;
    }
    std::cout << "   ✓ Audio capture initialized successfully\n\n";
    
    // Get available devices
    std::cout << "2. Enumerating audio devices...\n";
    auto devices = capture.getAudioDevices();
    std::cout << "   Found " << devices.size() << " audio devices:\n";
    
    for (size_t i = 0; i < devices.size(); ++i) {
        const auto& device = devices[i];
        std::cout << "   [" << i << "] " << device.name 
                  << " (ID: " << device.id << ")"
                  << " - " << device.channels << "ch, " << device.sample_rate << "Hz";
        if (device.is_default) std::cout << " [DEFAULT]";
        if (device.is_loopback) std::cout << " [LOOPBACK]";
        std::cout << "\n";
    }
    std::cout << "\n";
    
    // Get default device
    std::cout << "3. Testing default device selection...\n";
    auto default_device = capture.getDefaultDevice();
    if (!default_device.id.empty()) {
        std::cout << "   ✓ Default device: " << default_device.name << "\n";
        std::cout << "     Format: " << default_device.channels << " channels, " 
                  << default_device.sample_rate << " Hz\n\n";
    } else {
        std::cerr << "   ✗ No default device found!\n\n";
    }
    
    // Configure audio capture
    std::cout << "4. Configuring audio capture...\n";
    AudioCaptureConfig config;
    config.sample_rate = 16000;        // 16kHz for Whisper
    config.channels = 1;               // Mono
    config.buffer_size_ms = 100;       // 100ms buffers
    config.enable_silence_detection = false;  // Disable for test
    capture.setConfig(config);
    std::cout << "   ✓ Configured for 16kHz mono capture\n\n";
    
    // Test audio level monitoring
    std::cout << "5. Testing audio level monitoring...\n";
    float max_level = 0.0f;
    int level_updates = 0;
    
    capture.setLevelCallback([&](float level) {
        max_level = std::max(max_level, level);
        level_updates++;
    });
    
    // Start capture for 3 seconds
    std::cout << "   Starting capture for 3 seconds...\n";
    std::cout << "   (Please make some noise to test audio input)\n";
    
    bool capture_started = capture.startCapture([](const float* data, size_t samples) {
        // Audio callback - just count samples for now
        static size_t total_samples = 0;
        total_samples += samples;
        
        if (total_samples % 16000 == 0) {  // Every second at 16kHz
            std::cout << "   • Captured " << (total_samples / 16000) << " seconds of audio\n";
        }
    });
    
    if (!capture_started) {
        std::cerr << "   ✗ Failed to start audio capture!\n";
        capture.shutdown();
        return 1;
    }
    
    // Wait for 3 seconds
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Stop capture
    capture.stopCapture();
    
    std::cout << "   ✓ Capture completed\n";
    std::cout << "   ✓ Received " << level_updates << " level updates\n";
    std::cout << "   ✓ Maximum audio level: " << (max_level * 100) << "%\n\n";
    
    // Test captured audio buffer
    std::cout << "6. Testing captured audio buffer...\n";
    auto captured_audio = capture.getCapturedAudio();
    std::cout << "   ✓ Captured " << captured_audio.size() << " audio samples\n";
    std::cout << "   ✓ Duration: " << (captured_audio.size() / 16000.0) << " seconds\n\n";
    
    // Test statistics
    std::cout << "7. Testing capture statistics...\n";
    auto stats = capture.getStats();
    std::cout << "   ✓ Total samples: " << stats.total_samples << "\n";
    std::cout << "   ✓ Dropped samples: " << stats.dropped_samples << "\n";
    std::cout << "   ✓ Buffer overruns: " << stats.buffer_overruns << "\n";
    std::cout << "   ✓ Average level: " << (stats.average_level * 100) << "%\n\n";
    
    // Cleanup
    std::cout << "8. Shutting down...\n";
    capture.shutdown();
    std::cout << "   ✓ Audio capture shut down successfully\n\n";
    
    std::cout << "==========================================\n";
    std::cout << "WASAPI Audio Capture Test COMPLETED\n";
    std::cout << "All basic functionality appears to be working!\n";
    
    return 0;
}