/*
 * DeviceManagerTest.cpp
 * 
 * Unit tests for DeviceManager functionality
 */

#include "../../src/core/DeviceManager.h"
#include "../TestUtils.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <algorithm>

// Simple test framework macros
#define TEST(TestClass, TestName) \
    void TestClass##_##TestName(); \
    struct TestClass##_##TestName##_Register { \
        TestClass##_##TestName##_Register() { \
            std::cout << "Running " #TestClass "." #TestName << "..." << std::endl; \
            TestClass##_##TestName(); \
            std::cout << "  PASSED" << std::endl; \
        } \
    } TestClass##_##TestName##_Instance; \
    void TestClass##_##TestName()

#define EXPECT_TRUE(expr) \
    if (!(expr)) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #expr " to be true" << std::endl; \
        assert(false); \
    }

#define EXPECT_FALSE(expr) \
    if (expr) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #expr " to be false" << std::endl; \
        assert(false); \
    }

#define EXPECT_EQ(a, b) \
    if ((a) != (b)) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #a " == " #b << std::endl; \
        std::cerr << "  Actual: " << (a) << " != " << (b) << std::endl; \
        assert(false); \
    }

#define EXPECT_GT(a, b) \
    if (!((a) > (b))) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #a " > " #b << std::endl; \
        std::cerr << "  Actual: " << (a) << " <= " << (b) << std::endl; \
        assert(false); \
    }

#define EXPECT_GE(a, b) \
    if (!((a) >= (b))) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #a " >= " #b << std::endl; \
        std::cerr << "  Actual: " << (a) << " < " << (b) << std::endl; \
        assert(false); \
    }

#define EXPECT_LE(a, b) \
    if (!((a) <= (b))) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #a " <= " #b << std::endl; \
        std::cerr << "  Actual: " << (a) << " > " << (b) << std::endl; \
        assert(false); \
    }

#define EXPECT_NEAR(a, b, tolerance) \
    if (std::abs((a) - (b)) > (tolerance)) { \
        std::cerr << "Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " #a " near " #b << " (tolerance: " << tolerance << ")" << std::endl; \
        std::cerr << "  Actual: " << (a) << " vs " << (b) << std::endl; \
        assert(false); \
    }

// Test helper class
class DeviceManagerTestHelper {
public:
    std::unique_ptr<DeviceManager> manager;
    bool device_changed = false;
    bool default_changed = false;
    std::string changed_device_id;
    DeviceState changed_state;
    DeviceType changed_type;
    
    DeviceManagerTestHelper() {
        manager = std::make_unique<DeviceManager>();
        reset();
    }
    
    void reset() {
        device_changed = false;
        default_changed = false;
        changed_device_id.clear();
        changed_state = DeviceState::ACTIVE;
        changed_type = DeviceType::CAPTURE;
    }
    
    void onDeviceChange(const std::string& device_id, DeviceState state) {
        device_changed = true;
        changed_device_id = device_id;
        changed_state = state;
    }
    
    void onDefaultChange(DeviceType type, const std::string& device_id) {
        default_changed = true;
        changed_type = type;
        changed_device_id = device_id;
    }
};

TEST(DeviceManager, InitializeShutdown) {
    DeviceManagerTestHelper helper;
    
    EXPECT_TRUE(helper.manager->initialize());
    EXPECT_TRUE(helper.manager->initialize()); // Should be safe to call multiple times
    
    helper.manager->shutdown();
    helper.manager->shutdown(); // Should be safe to call multiple times
}

TEST(DeviceManager, EnumerateDevices) {
    DeviceManagerTestHelper helper;
    EXPECT_TRUE(helper.manager->initialize());
    
    // Test capture devices
    auto capture_devices = helper.manager->getDevices(DeviceType::CAPTURE);
    EXPECT_FALSE(capture_devices.empty());
    
    for (const auto& device : capture_devices) {
        EXPECT_FALSE(device.id.empty());
        EXPECT_FALSE(device.friendly_name.empty());
        EXPECT_FALSE(device.capabilities.supported_formats.empty());
        EXPECT_GT(device.capabilities.max_channels, 0);
        EXPECT_GT(device.capabilities.max_sample_rate, 0);
    }
    
    // Test render devices
    auto render_devices = helper.manager->getDevices(DeviceType::RENDER);
    EXPECT_FALSE(render_devices.empty());
    
    // Test loopback devices
    auto loopback_devices = helper.manager->getDevices(DeviceType::LOOPBACK);
    // Should include render devices that support loopback
}

TEST(DeviceManager, DefaultDevices) {
    DeviceManagerTestHelper helper;
    EXPECT_TRUE(helper.manager->initialize());
    
    // Test default capture device
    auto default_capture = helper.manager->getDefaultDevice(DeviceType::CAPTURE);
    EXPECT_FALSE(default_capture.id.empty());
    EXPECT_TRUE(default_capture.is_default);
    
    // Test default render device
    auto default_render = helper.manager->getDefaultDevice(DeviceType::RENDER);
    EXPECT_FALSE(default_render.id.empty());
    EXPECT_TRUE(default_render.is_default);
    
    // Test default communications devices
    auto default_comm_capture = helper.manager->getDefaultCommunicationsDevice(DeviceType::CAPTURE);
    EXPECT_FALSE(default_comm_capture.id.empty());
}

TEST(DeviceManager, DeviceInfo) {
    DeviceManagerTestHelper helper;
    EXPECT_TRUE(helper.manager->initialize());
    
    auto devices = helper.manager->getDevices(DeviceType::CAPTURE);
    EXPECT_FALSE(devices.empty());
    
    // Get detailed info for first device
    auto info = helper.manager->getDeviceInfo(devices[0].id);
    EXPECT_EQ(info.id, devices[0].id);
    EXPECT_FALSE(info.friendly_name.empty());
    EXPECT_FALSE(info.description.empty());
    EXPECT_FALSE(info.manufacturer.empty());
    EXPECT_FALSE(info.driver_version.empty());
    
    // Test non-existent device
    auto invalid_info = helper.manager->getDeviceInfo("invalid_device_id");
    EXPECT_TRUE(invalid_info.id.empty());
}

TEST(DeviceManager, FormatSupport) {
    DeviceManagerTestHelper helper;
    EXPECT_TRUE(helper.manager->initialize());
    
    auto devices = helper.manager->getDevices(DeviceType::CAPTURE);
    EXPECT_FALSE(devices.empty());
    
    const auto& device = devices[0];
    
    // Test supported format
    if (!device.capabilities.supported_formats.empty()) {
        const auto& format = device.capabilities.supported_formats[0];
        EXPECT_TRUE(helper.manager->isFormatSupported(device.id, format));
    }
    
    // Test unsupported format
    AudioFormat unsupported = {192000, 8, 64, "UNSUPPORTED"};
    EXPECT_FALSE(helper.manager->isFormatSupported(device.id, unsupported));
    
    // Test best matching format
    AudioFormat preferred = {16000, 1, 16, "PCM"};
    auto best_match = helper.manager->getBestMatchingFormat(device.id, preferred);
    EXPECT_FALSE(best_match.format_tag.empty());
}

TEST(DeviceManager, DeviceTest) {
    DeviceManagerTestHelper helper;
    EXPECT_TRUE(helper.manager->initialize());
    
    auto devices = helper.manager->getDevices(DeviceType::CAPTURE);
    EXPECT_FALSE(devices.empty());
    
    // Test device functionality
    bool test_result = helper.manager->testDevice(devices[0].id, 100);
    EXPECT_TRUE(test_result);
    
    // Test non-existent device
    test_result = helper.manager->testDevice("invalid_device_id", 100);
    EXPECT_FALSE(test_result);
}

TEST(DeviceManager, DeviceState) {
    DeviceManagerTestHelper helper;
    EXPECT_TRUE(helper.manager->initialize());
    
    auto devices = helper.manager->getDevices(DeviceType::CAPTURE);
    EXPECT_FALSE(devices.empty());
    
    auto state = helper.manager->getDeviceState(devices[0].id);
    EXPECT_TRUE(state == DeviceState::ACTIVE || 
                state == DeviceState::DISABLED ||
                state == DeviceState::NOT_PRESENT ||
                state == DeviceState::UNPLUGGED);
    
    // Test non-existent device
    state = helper.manager->getDeviceState("invalid_device_id");
    EXPECT_EQ(state, DeviceState::NOT_PRESENT);
}

TEST(DeviceManager, DeviceMonitoring) {
    DeviceManagerTestHelper helper;
    EXPECT_TRUE(helper.manager->initialize());
    
    // Set callbacks
    helper.manager->setDeviceChangeCallback(
        [&helper](const std::string& id, DeviceState state) {
            helper.onDeviceChange(id, state);
        }
    );
    
    helper.manager->setDefaultDeviceChangeCallback(
        [&helper](DeviceType type, const std::string& id) {
            helper.onDefaultChange(type, id);
        }
    );
    
    // Enable monitoring
    helper.manager->enableMonitoring(true);
    
    // Disable monitoring
    helper.manager->enableMonitoring(false);
}

TEST(DeviceManager, RefreshDevices) {
    DeviceManagerTestHelper helper;
    EXPECT_TRUE(helper.manager->initialize());
    
    auto devices_before = helper.manager->getDevices(DeviceType::CAPTURE);
    
    // Refresh devices multiple times
    for (int i = 0; i < 10; ++i) {
        helper.manager->refreshDevices();
    }
    
    auto devices_after = helper.manager->getDevices(DeviceType::CAPTURE);
    
    // Device count should remain stable in mock implementation
    EXPECT_EQ(devices_before.size(), devices_after.size());
}

TEST(DeviceManager, VolumeControl) {
    DeviceManagerTestHelper helper;
    EXPECT_TRUE(helper.manager->initialize());
    
    auto devices = helper.manager->getDevices(DeviceType::RENDER);
    EXPECT_FALSE(devices.empty());
    
    const auto& device = devices[0];
    
    // Get current volume
    float volume = helper.manager->getDeviceVolume(device.id);
    EXPECT_GE(volume, 0.0f);
    EXPECT_LE(volume, 1.0f);
    
    // Set volume
    EXPECT_TRUE(helper.manager->setDeviceVolume(device.id, 0.7f));
    
    // Verify volume changed
    float new_volume = helper.manager->getDeviceVolume(device.id);
    EXPECT_NEAR(new_volume, 0.7f, 0.01f);
    
    // Test invalid device
    EXPECT_FALSE(helper.manager->setDeviceVolume("invalid_device_id", 0.5f));
    EXPECT_EQ(helper.manager->getDeviceVolume("invalid_device_id"), -1.0f);
}

TEST(DeviceManager, MuteControl) {
    DeviceManagerTestHelper helper;
    EXPECT_TRUE(helper.manager->initialize());
    
    auto devices = helper.manager->getDevices(DeviceType::RENDER);
    EXPECT_FALSE(devices.empty());
    
    const auto& device = devices[0];
    
    // Get current mute state
    bool is_muted = helper.manager->isDeviceMuted(device.id);
    
    // Toggle mute
    EXPECT_TRUE(helper.manager->setDeviceMuted(device.id, !is_muted));
    EXPECT_EQ(helper.manager->isDeviceMuted(device.id), !is_muted);
    
    // Toggle back
    EXPECT_TRUE(helper.manager->setDeviceMuted(device.id, is_muted));
    EXPECT_EQ(helper.manager->isDeviceMuted(device.id), is_muted);
    
    // Test invalid device
    EXPECT_FALSE(helper.manager->setDeviceMuted("invalid_device_id", true));
    EXPECT_FALSE(helper.manager->isDeviceMuted("invalid_device_id"));
}

TEST(DeviceManager, DeviceLatency) {
    DeviceManagerTestHelper helper;
    EXPECT_TRUE(helper.manager->initialize());
    
    auto devices = helper.manager->getDevices(DeviceType::CAPTURE);
    EXPECT_FALSE(devices.empty());
    
    int latency = helper.manager->getDeviceLatency(devices[0].id);
    EXPECT_GT(latency, 0);
    
    // Test loopback device (should have different latency)
    auto loopback_devices = helper.manager->getDevices(DeviceType::LOOPBACK);
    if (!loopback_devices.empty()) {
        int loopback_latency = helper.manager->getDeviceLatency(loopback_devices[0].id);
        EXPECT_GT(loopback_latency, 0);
    }
    
    // Test invalid device
    EXPECT_EQ(helper.manager->getDeviceLatency("invalid_device_id"), -1);
}

TEST(DeviceManager, AudioSessions) {
    DeviceManagerTestHelper helper;
    EXPECT_TRUE(helper.manager->initialize());
    
    auto render_devices = helper.manager->getDevices(DeviceType::RENDER);
    EXPECT_FALSE(render_devices.empty());
    
    auto sessions = helper.manager->getAudioSessions(render_devices[0].id);
    
    // Should have some mock sessions for render devices
    EXPECT_FALSE(sessions.empty());
    
    for (const auto& [name, pid] : sessions) {
        EXPECT_FALSE(name.empty());
        EXPECT_GE(pid, 0);
    }
    
    // Capture devices shouldn't have sessions
    auto capture_devices = helper.manager->getDevices(DeviceType::CAPTURE);
    if (!capture_devices.empty()) {
        auto capture_sessions = helper.manager->getAudioSessions(capture_devices[0].id);
        EXPECT_TRUE(capture_sessions.empty());
    }
}

TEST(DeviceManager, DeviceCapabilities) {
    DeviceManagerTestHelper helper;
    EXPECT_TRUE(helper.manager->initialize());
    
    auto devices = helper.manager->getDevices(DeviceType::CAPTURE);
    EXPECT_FALSE(devices.empty());
    
    for (const auto& device : devices) {
        const auto& caps = device.capabilities;
        
        // Validate capabilities
        EXPECT_GT(caps.min_channels, 0);
        EXPECT_GE(caps.max_channels, caps.min_channels);
        EXPECT_GT(caps.min_sample_rate, 0);
        EXPECT_GE(caps.max_sample_rate, caps.min_sample_rate);
        EXPECT_GT(caps.min_buffer_size_ms, 0);
        EXPECT_GE(caps.max_buffer_size_ms, caps.min_buffer_size_ms);
        
        // Check format support
        EXPECT_FALSE(caps.supported_formats.empty());
        
        for (const auto& format : caps.supported_formats) {
            EXPECT_GT(format.sample_rate, 0);
            EXPECT_GT(format.channels, 0);
            EXPECT_GT(format.bits_per_sample, 0);
            EXPECT_FALSE(format.format_tag.empty());
        }
    }
}

// Main function
int main() {
    std::cout << "Running DeviceManager tests..." << std::endl;
    std::cout << "All tests completed!" << std::endl;
    return 0;
}