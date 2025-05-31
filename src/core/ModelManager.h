/*
 * ModelManager.h
 * 
 * Manages Whisper model files including downloading, verification, and storage.
 * This class handles all model-related operations for the application.
 * 
 * Features:
 * - Model enumeration and information
 * - Asynchronous model downloading with progress
 * - Model integrity verification (checksum)
 * - Disk space management
 * - Automatic model updates
 */

#ifndef MODELMANAGER_H
#define MODELMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

// Forward declarations
class QNetworkAccessManager;
class QNetworkReply;

/**
 * @brief Information about a Whisper model
 */
struct ModelInfo {
    std::string id;                     // Unique model identifier
    std::string name;                   // Display name
    std::string filename;               // Local filename
    std::string url;                    // Download URL
    std::string checksum;               // SHA256 checksum
    uint64_t size_bytes = 0;            // File size in bytes
    std::string description;            // Model description
    
    // Performance characteristics
    struct Performance {
        float relative_speed = 1.0f;    // Speed relative to base model
        float accuracy = 0.0f;          // Accuracy score (0-100)
        int memory_mb = 0;              // Memory requirement in MB
        bool gpu_capable = false;       // Can use GPU acceleration
    } performance;
    
    // Model capabilities
    struct Capabilities {
        std::vector<std::string> languages; // Supported languages
        bool multilingual = false;          // Supports multiple languages
        bool translation = false;           // Can translate to English
    } capabilities;
    
    bool is_downloaded = false;         // Is model downloaded
    bool is_verified = false;           // Has passed checksum verification
    std::string local_path;             // Full path to local file
};

/**
 * @brief Download progress information
 */
struct DownloadProgress {
    std::string model_id;               // Model being downloaded
    uint64_t bytes_received = 0;        // Bytes downloaded so far
    uint64_t bytes_total = 0;           // Total bytes to download
    float speed_mbps = 0.0f;            // Current download speed
    int eta_seconds = 0;                // Estimated time remaining
    float progress_percent = 0.0f;      // Progress percentage (0-100)
};

/**
 * @brief Model manager class for handling Whisper models
 */
class ModelManager {
public:
    /**
     * @brief Download progress callback
     */
    using ProgressCallback = std::function<void(const DownloadProgress& progress)>;
    
    /**
     * @brief Download completion callback
     */
    using CompletionCallback = std::function<void(bool success, const std::string& error)>;
    
    /**
     * @brief Model update check callback
     */
    using UpdateCallback = std::function<void(const std::vector<std::string>& updated_models)>;

public:
    ModelManager();
    ~ModelManager();

    // Prevent copying
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    /**
     * @brief Initialize the model manager
     * @param models_directory Directory to store models
     * @return true if successful, false otherwise
     */
    bool initialize(const std::string& models_directory);

    /**
     * @brief Get list of available models
     * @return Vector of model information
     */
    std::vector<ModelInfo> getAvailableModels() const;

    /**
     * @brief Get information about a specific model
     * @param model_id Model identifier
     * @return Model information (empty if not found)
     */
    ModelInfo getModelInfo(const std::string& model_id) const;

    /**
     * @brief Get list of downloaded models
     * @return Vector of downloaded model IDs
     */
    std::vector<std::string> getDownloadedModels() const;

    /**
     * @brief Check if a model is downloaded
     * @param model_id Model identifier
     * @return true if downloaded, false otherwise
     */
    bool isModelDownloaded(const std::string& model_id) const;

    /**
     * @brief Get path to a downloaded model
     * @param model_id Model identifier
     * @return Full path to model file (empty if not downloaded)
     */
    std::string getModelPath(const std::string& model_id) const;

    /**
     * @brief Download a model asynchronously
     * @param model_id Model identifier
     * @param progress_callback Progress callback (optional)
     * @param completion_callback Completion callback
     * @return true if download started, false if already downloading
     */
    bool downloadModel(const std::string& model_id,
                      ProgressCallback progress_callback,
                      CompletionCallback completion_callback);

    /**
     * @brief Cancel ongoing download
     * @param model_id Model identifier
     */
    void cancelDownload(const std::string& model_id);

    /**
     * @brief Check if a model is currently downloading
     * @param model_id Model identifier
     * @return true if downloading, false otherwise
     */
    bool isDownloading(const std::string& model_id) const;

    /**
     * @brief Delete a downloaded model
     * @param model_id Model identifier
     * @return true if deleted, false otherwise
     */
    bool deleteModel(const std::string& model_id);

    /**
     * @brief Verify integrity of a downloaded model
     * @param model_id Model identifier
     * @return true if valid, false otherwise
     */
    bool verifyModel(const std::string& model_id);

    /**
     * @brief Get total disk space used by models
     * @return Size in bytes
     */
    uint64_t getTotalDiskUsage() const;

    /**
     * @brief Get available disk space for models
     * @return Size in bytes
     */
    uint64_t getAvailableDiskSpace() const;

    /**
     * @brief Check for model updates
     * @param callback Callback with list of models that have updates
     */
    void checkForUpdates(UpdateCallback callback);

    /**
     * @brief Set download speed limit
     * @param limit_mbps Speed limit in megabits per second (0 = unlimited)
     */
    void setDownloadSpeedLimit(float limit_mbps);

    /**
     * @brief Get recommended model based on system capabilities
     * @return Model ID of recommended model
     */
    std::string getRecommendedModel() const;

    /**
     * @brief Import a model from external file
     * @param file_path Path to model file
     * @param model_id Model identifier to use
     * @return true if imported successfully
     */
    bool importModel(const std::string& file_path, const std::string& model_id);

    /**
     * @brief Export a model to external location
     * @param model_id Model identifier
     * @param destination_path Destination file path
     * @return true if exported successfully
     */
    bool exportModel(const std::string& model_id, const std::string& destination_path);

private:
    // Private implementation (PIMPL idiom)
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // MODELMANAGER_H