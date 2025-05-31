/*
 * MockWhisperEngine.h
 * 
 * Mock implementation of WhisperEngine for testing
 */

#ifndef MOCKWHISPERENGINE_H
#define MOCKWHISPERENGINE_H

#include "core/WhisperEngine.h"
#include "../TestUtils.h"
#include <thread>
#include <chrono>

class MockWhisperEngine : public IWhisperEngine {
private:
    bool initialized_ = false;
    bool processing_ = false;
    std::string modelPath_;
    TranscriptionCallback callback_;
    ProgressCallback progressCallback_;
    std::thread processingThread_;
    bool shouldFail_ = false;
    int processingDelayMs_ = 100;
    
public:
    MockWhisperEngine() = default;
    ~MockWhisperEngine() override {
        if (processing_) {
            cancel();
        }
    }
    
    ErrorCode initialize(const std::string& modelPath) override {
        if (modelPath.find("invalid") != std::string::npos) {
            return ErrorCode::ModelNotFound;
        }
        
        modelPath_ = modelPath;
        initialized_ = true;
        return ErrorCode::Success;
    }
    
    ErrorCode transcribe(const std::vector<float>& audioData, 
                        const TranscriptionOptions& options) override {
        if (!initialized_) {
            return ErrorCode::NotInitialized;
        }
        
        if (processing_) {
            return ErrorCode::InvalidState;
        }
        
        if (shouldFail_) {
            return ErrorCode::TranscriptionFailed;
        }
        
        processing_ = true;
        
        // Simulate async processing
        processingThread_ = std::thread([this, audioData, options]() {
            // Simulate processing time
            std::this_thread::sleep_for(std::chrono::milliseconds(processingDelayMs_));
            
            if (!processing_) {
                return; // Cancelled
            }
            
            // Generate mock result
            TranscriptionResult result;
            result.text = "This is a mock transcription of " + 
                         std::to_string(audioData.size()) + " audio samples.";
            result.language = options.language.empty() ? "en" : options.language;
            
            // Add mock segments
            int segmentCount = 3;
            int segmentDuration = static_cast<int>(audioData.size() / 16.0f); // ms
            for (int i = 0; i < segmentCount; ++i) {
                TranscriptionSegment segment;
                segment.start = i * segmentDuration / segmentCount;
                segment.end = (i + 1) * segmentDuration / segmentCount;
                segment.text = "Segment " + std::to_string(i + 1);
                
                if (options.wordTimestamps) {
                    // Add mock words
                    segment.words.push_back({"Segment", segment.start, segment.start + 100});
                    segment.words.push_back({std::to_string(i + 1), 
                                           segment.start + 100, segment.end});
                }
                
                result.segments.push_back(segment);
            }
            
            processing_ = false;
            
            // Call callback
            if (callback_) {
                callback_(result);
            }
        });
        
        return ErrorCode::Success;
    }
    
    void cancel() override {
        processing_ = false;
        if (processingThread_.joinable()) {
            processingThread_.join();
        }
    }
    
    bool isInitialized() const override {
        return initialized_;
    }
    
    bool isProcessing() const override {
        return processing_;
    }
    
    ModelInfo getModelInfo() const override {
        ModelInfo info;
        info.name = "mock_model";
        info.type = "mock";
        info.language = "en";
        info.multilingual = false;
        return info;
    }
    
    std::vector<std::string> getSupportedLanguages() const override {
        return {"en", "es", "fr", "de", "it", "pt", "ru", "zh", "ja"};
    }
    
    void setTranscriptionCallback(TranscriptionCallback callback) override {
        callback_ = callback;
    }
    
    void setProgressCallback(ProgressCallback callback) override {
        progressCallback_ = callback;
    }
    
    // Mock-specific methods
    void setShouldFail(bool shouldFail) {
        shouldFail_ = shouldFail;
    }
    
    void setProcessingDelay(int delayMs) {
        processingDelayMs_ = delayMs;
    }
};

#endif // MOCKWHISPERENGINE_H