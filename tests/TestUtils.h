/*
 * TestUtils.h
 * 
 * Common utilities and helper functions for unit tests.
 */

#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <limits>
#include <mutex>
#include <condition_variable>

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace TestUtils {

/**
 * @brief Generate test audio data
 */
class AudioGenerator {
public:
    /**
     * @brief Generate a sine wave
     * @param frequency Frequency in Hz
     * @param duration Duration in seconds
     * @param sampleRate Sample rate in Hz
     * @param amplitude Amplitude (0.0 to 1.0)
     * @return Vector of float samples
     */
    static std::vector<float> generateSineWave(float frequency, 
                                              float duration, 
                                              int sampleRate = 16000,
                                              float amplitude = 0.5f) {
        std::vector<float> samples;
        size_t numSamples = static_cast<size_t>(duration * sampleRate);
        samples.reserve(numSamples);
        
        const float twoPi = 2.0f * M_PI;
        for (size_t i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            float sample = amplitude * std::sin(twoPi * frequency * t);
            samples.push_back(sample);
        }
        
        return samples;
    }
    
    /**
     * @brief Generate white noise
     * @param duration Duration in seconds
     * @param sampleRate Sample rate in Hz
     * @param amplitude Amplitude (0.0 to 1.0)
     * @return Vector of float samples
     */
    static std::vector<float> generateWhiteNoise(float duration,
                                                int sampleRate = 16000,
                                                float amplitude = 0.5f) {
        std::vector<float> samples;
        size_t numSamples = static_cast<size_t>(duration * sampleRate);
        samples.reserve(numSamples);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        for (size_t i = 0; i < numSamples; ++i) {
            samples.push_back(amplitude * dist(gen));
        }
        
        return samples;
    }
    
    /**
     * @brief Generate silence
     * @param duration Duration in seconds
     * @param sampleRate Sample rate in Hz
     * @return Vector of float samples (all zeros)
     */
    static std::vector<float> generateSilence(float duration,
                                            int sampleRate = 16000) {
        size_t numSamples = static_cast<size_t>(duration * sampleRate);
        return std::vector<float>(numSamples, 0.0f);
    }
    
    /**
     * @brief Mix multiple audio signals
     * @param signals Vector of audio signals to mix
     * @return Mixed audio signal
     */
    static std::vector<float> mixSignals(const std::vector<std::vector<float>>& signals) {
        if (signals.empty()) {
            return std::vector<float>();
        }
        
        // Find the longest signal
        size_t maxLength = 0;
        for (const auto& signal : signals) {
            maxLength = std::max(maxLength, signal.size());
        }
        
        std::vector<float> mixed(maxLength, 0.0f);
        
        // Mix signals
        for (const auto& signal : signals) {
            for (size_t i = 0; i < signal.size(); ++i) {
                mixed[i] += signal[i] / signals.size();
            }
        }
        
        return mixed;
    }
};

/**
 * @brief File system utilities for tests
 */
class FileUtils {
public:
    /**
     * @brief Create a temporary directory for tests
     * @return Path to temporary directory
     */
    static std::string createTempDirectory() {
        namespace fs = std::filesystem;
        
        auto tempPath = fs::temp_directory_path();
        auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto dirName = "whisperapp_test_" + std::to_string(timestamp);
        auto testDir = tempPath / dirName;
        
        fs::create_directories(testDir);
        return testDir.string();
    }
    
    /**
     * @brief Clean up temporary directory
     * @param path Path to temporary directory
     */
    static void cleanupTempDirectory(const std::string& path) {
        namespace fs = std::filesystem;
        
        if (fs::exists(path)) {
            fs::remove_all(path);
        }
    }
    
    /**
     * @brief Write data to a temporary file
     * @param data Data to write
     * @param extension File extension
     * @return Path to temporary file
     */
    static std::string writeTempFile(const std::vector<uint8_t>& data,
                                   const std::string& extension = ".tmp") {
        namespace fs = std::filesystem;
        
        auto tempPath = fs::temp_directory_path();
        auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto fileName = "whisperapp_test_" + std::to_string(timestamp) + extension;
        auto filePath = tempPath / fileName;
        
        std::ofstream file(filePath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();
        
        return filePath.string();
    }
};

/**
 * @brief Performance measurement utilities
 */
class PerformanceUtils {
public:
    /**
     * @brief Simple timer for measuring execution time
     */
    class Timer {
    private:
        std::chrono::steady_clock::time_point start_;
        std::string name_;
        
    public:
        Timer(const std::string& name = "") 
            : start_(std::chrono::steady_clock::now()), name_(name) {}
        
        ~Timer() {
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
            if (!name_.empty()) {
                std::cout << name_ << ": ";
            }
            std::cout << "Execution time: " << duration.count() << " ms" << std::endl;
        }
        
        uint64_t elapsed() const {
            auto end = std::chrono::steady_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count();
        }
    };
    
    /**
     * @brief Measure average execution time
     * @param func Function to measure
     * @param iterations Number of iterations
     * @return Average execution time in milliseconds
     */
    template<typename Func>
    static double measureAverageTime(Func func, int iterations = 100) {
        auto start = std::chrono::steady_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            func();
        }
        
        auto end = std::chrono::steady_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return static_cast<double>(totalDuration.count()) / (iterations * 1000.0);
    }
};

/**
 * @brief Math utilities for tests
 */
class MathUtils {
public:
    /**
     * @brief Calculate signal-to-noise ratio
     * @param signal Original signal
     * @param noisy Signal with noise
     * @return SNR in dB
     */
    static float calculateSNR(const std::vector<float>& signal,
                             const std::vector<float>& noisy) {
        if (signal.size() != noisy.size() || signal.empty()) {
            return -std::numeric_limits<float>::infinity();
        }
        
        float signalPower = 0.0f;
        float noisePower = 0.0f;
        
        for (size_t i = 0; i < signal.size(); ++i) {
            signalPower += signal[i] * signal[i];
            float noise = noisy[i] - signal[i];
            noisePower += noise * noise;
        }
        
        if (noisePower == 0.0f) {
            return std::numeric_limits<float>::infinity();
        }
        
        return 10.0f * std::log10(signalPower / noisePower);
    }
    
    /**
     * @brief Calculate RMS (Root Mean Square) of a signal
     * @param signal Input signal
     * @return RMS value
     */
    static float calculateRMS(const std::vector<float>& signal) {
        if (signal.empty()) {
            return 0.0f;
        }
        
        float sumSquares = 0.0f;
        for (float sample : signal) {
            sumSquares += sample * sample;
        }
        
        return std::sqrt(sumSquares / signal.size());
    }
    
    /**
     * @brief Check if two floating point values are approximately equal
     * @param a First value
     * @param b Second value
     * @param epsilon Tolerance
     * @return True if values are within epsilon
     */
    static bool approximatelyEqual(float a, float b, float epsilon = 1e-6f) {
        return std::abs(a - b) < epsilon;
    }
};

/**
 * @brief Mock callback tracking for async operations
 */
template<typename T>
class CallbackTracker {
private:
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    bool called_ = false;
    T result_;
    
public:
    void onCallback(const T& result) {
        std::lock_guard<std::mutex> lock(mutex_);
        result_ = result;
        called_ = true;
        cv_.notify_all();
    }
    
    bool waitForCallback(int timeoutMs = 5000) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                           [this] { return called_; });
    }
    
    T getResult() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return result_;
    }
    
    bool wasCalled() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return called_;
    }
    
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        called_ = false;
        result_ = T();
    }
};

} // namespace TestUtils

#endif // TESTUTILS_H