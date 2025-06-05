/*
 * WhisperEngine.h
 * 
 * Core engine for integrating whisper.cpp functionality.
 * This class provides a high-level interface for speech recognition
 * using the whisper.cpp library.
 * 
 * Features:
 * - Model loading and management
 * - Audio transcription (synchronous and asynchronous)
 * - Language detection and selection
 * - Performance optimization settings
 */

#ifndef WHISPERENGINE_H
#define WHISPERENGINE_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>

// Forward declarations
struct whisper_context;

/**
 * @brief Main class for whisper.cpp integration
 * 
 * This class encapsulates all whisper.cpp functionality and provides
 * a clean C++ interface for the rest of the application.
 */
class WhisperEngine {
public:
    /**
     * @brief Transcription parameters
     */
    struct TranscriptionParams {
        std::string language = "en";        // Target language code
        bool translate = false;             // Translate to English
        int max_threads = 0;                // 0 = auto-detect
        bool print_timestamps = false;      // Include timestamps in output
        bool print_special_tokens = false;  // Include special tokens
        int beam_size = 5;                  // Beam search size
        float temperature = 0.0f;           // Sampling temperature
        bool detect_language = false;       // Auto-detect language
    };

    /**
     * @brief Transcription result
     */
    struct TranscriptionResult {
        std::string text;                   // Transcribed text
        std::string detected_language;      // Detected language (if enabled)
        float confidence = 0.0f;            // Overall confidence score
        int64_t processing_time_ms = 0;     // Processing time in milliseconds
        
        struct Segment {
            std::string text;
            int64_t start_ms;
            int64_t end_ms;
            float confidence;
        };
        std::vector<Segment> segments;      // Individual segments with timing
    };

    /**
     * @brief Progress callback function type
     */
    using ProgressCallback = std::function<void(float progress)>;
    
    /**
     * @brief Result callback function type
     */
    using ResultCallback = std::function<void(const TranscriptionResult& result)>;

public:
    WhisperEngine();
    ~WhisperEngine();

    // Prevent copying
    WhisperEngine(const WhisperEngine&) = delete;
    WhisperEngine& operator=(const WhisperEngine&) = delete;

    /**
     * @brief Load a whisper model from file
     * @param model_path Path to the model file (.bin or .gguf)
     * @return true if successful, false otherwise
     */
    bool loadModel(const std::string& model_path);

    /**
     * @brief Unload the current model
     */
    void unloadModel();

    /**
     * @brief Check if a model is loaded
     * @return true if a model is loaded, false otherwise
     */
    bool isModelLoaded() const;

    /**
     * @brief Get information about the loaded model
     * @return Model information string
     */
    std::string getModelInfo() const;

    /**
     * @brief Transcribe audio data synchronously
     * @param audio_data Audio samples (16kHz, mono, float32)
     * @param params Transcription parameters
     * @return Transcription result
     */
    TranscriptionResult transcribeAudio(
        const std::vector<float>& audio_data,
        const TranscriptionParams& params = TranscriptionParams()
    );

    /**
     * @brief Transcribe audio data asynchronously
     * @param audio_data Audio samples (16kHz, mono, float32)
     * @param params Transcription parameters
     * @param on_result Callback for result
     * @param on_progress Optional progress callback
     */
    void transcribeAudioAsync(
        const std::vector<float>& audio_data,
        const TranscriptionParams& params,
        ResultCallback on_result,
        ProgressCallback on_progress = nullptr
    );

    /**
     * @brief Cancel any ongoing asynchronous transcription
     */
    void cancelTranscription();

    /**
     * @brief Check if transcription is in progress
     * @return true if transcription is running, false otherwise
     */
    bool isTranscribing() const;

    /**
     * @brief Get supported languages
     * @return Vector of language codes
     */
    static std::vector<std::string> getSupportedLanguages();

    /**
     * @brief Get language name from code
     * @param language_code Two-letter language code
     * @return Full language name
     */
    static std::string getLanguageName(const std::string& language_code);

    /**
     * @brief Set the number of threads to use
     * @param num_threads Number of threads (0 = auto)
     */
    void setThreadCount(int num_threads);

    /**
     * @brief Get the current thread count setting
     * @return Number of threads
     */
    int getThreadCount() const;

    /**
     * @brief Enable or disable GPU acceleration (if available)
     * @param enable true to enable GPU, false to disable
     * @return true if GPU mode was set successfully
     */
    bool setGPUEnabled(bool enable);

    /**
     * @brief Check if GPU acceleration is available
     * @return true if GPU is available, false otherwise
     */
    bool isGPUAvailable() const;

private:
    // Private implementation (PIMPL idiom)
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // WHISPERENGINE_H