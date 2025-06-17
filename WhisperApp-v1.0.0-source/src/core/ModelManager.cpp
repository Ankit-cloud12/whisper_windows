/*
 * ModelManager.cpp
 * 
 * Real implementation of Whisper model downloading, verification, and management.
 * Uses Qt Network for robust HTTP downloads.
 */

#include "ModelManager.h"
#include "core/Logger.h" // Added Logger include
#include <fstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <map>
#include <queue>
#include <mutex>
#include <atomic>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QEventLoop>
#include <QCoreApplication>

#ifdef WHISPER_AVAILABLE
#include "../../third_party/whisper.cpp/whisper.h"
#endif

namespace fs = std::filesystem;

/**
 * @brief Internal download state structure
 */
struct DownloadState {
    std::atomic<bool> is_active{false};
    std::atomic<bool> is_paused{false};
    std::atomic<bool> should_cancel{false};
    std::atomic<uint64_t> bytes_downloaded{0};
    uint64_t total_bytes = 0;
    std::chrono::steady_clock::time_point start_time;
    std::string temp_file_path;
    std::string final_file_path;
    ModelManager::ProgressCallback progress_callback;
    ModelManager::CompletionCallback completion_callback;
};

/**
 * @brief PIMPL implementation class
 */
class ModelManager::Impl {
public:
    // Model database with predefined information
    std::map<std::string, ModelInfo> available_models;
    
    // Download management
    std::map<std::string, std::unique_ptr<DownloadState>> active_downloads;
    mutable std::mutex downloads_mutex;
    
    // Configuration
    std::string models_directory;
    float download_speed_limit = 0.0f; // 0 = unlimited
    
    // Network manager for downloads
    QNetworkAccessManager* network_manager = nullptr;
    
    // Model definitions
    void initializeModelDatabase() {
        // Define all available Whisper models with real URLs and approximate sizes
        available_models = {
            {"tiny", {
                "tiny", "Tiny", "ggml-tiny.bin",
                "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin",
                "bd577a113a864445d4c299885e0cb97d4ba92b5f", // Placeholder checksum
                39000000, // ~39MB
                "Smallest model, fastest processing. Good for quick transcriptions with moderate accuracy.",
                {1.0f, 60.0f, 100, false},
                {{"en", "es", "fr", "de", "it", "pt", "ru", "ja", "ko", "zh"}, true, false},
                false, false, ""
            }},
            {"tiny.en", {
                "tiny.en", "Tiny English", "ggml-tiny.en.bin", 
                "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin",
                "c78c86eb1a8faa21b369bcd33207cc90d64ae9df",
                39000000,
                "English-only tiny model. Faster and more accurate for English.",
                {1.0f, 65.0f, 100, false},
                {{"en"}, false, false},
                false, false, ""
            }},
            {"base", {
                "base", "Base", "ggml-base.bin",
                "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin", 
                "465707469ff3a37a2b9b8d8f89f2f99de7299dac",
                74000000, // ~74MB
                "Base model with better accuracy than tiny. Good balance of speed and quality.",
                {0.8f, 70.0f, 200, false},
                {{"en", "es", "fr", "de", "it", "pt", "ru", "ja", "ko", "zh"}, true, false},
                false, false, ""
            }},
            {"base.en", {
                "base.en", "Base English", "ggml-base.en.bin",
                "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin",
                "137c40403d78fd54d454da0f9bd998f78703390c",
                74000000,
                "English-only base model. Better accuracy for English transcriptions.",
                {0.8f, 75.0f, 200, false},
                {{"en"}, false, false},
                false, false, ""
            }},
            {"small", {
                "small", "Small", "ggml-small.bin",
                "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin",
                "55356645c2b361a969dfd0ef2c5a50d530afd8d5",
                244000000, // ~244MB
                "Small model with good accuracy. Suitable for most use cases.",
                {0.6f, 80.0f, 500, false},
                {{"en", "es", "fr", "de", "it", "pt", "ru", "ja", "ko", "zh"}, true, false},
                false, false, ""
            }},
            {"small.en", {
                "small.en", "Small English", "ggml-small.en.bin",
                "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin",
                "db8a495a91d927739e50b3fc1cc4c6b8f6c2d022",
                244000000,
                "English-only small model. Excellent accuracy for English.",
                {0.6f, 85.0f, 500, false},
                {{"en"}, false, false},
                false, false, ""
            }},
            {"medium", {
                "medium", "Medium", "ggml-medium.bin",
                "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin",
                "fd9727b6e1217c2f614f9b698455c4ffd82463b4",
                769000000, // ~769MB  
                "Medium model with high accuracy. Good for professional use.",
                {0.4f, 90.0f, 1000, false},
                {{"en", "es", "fr", "de", "it", "pt", "ru", "ja", "ko", "zh"}, true, false},
                false, false, ""
            }},
            {"medium.en", {
                "medium.en", "Medium English", "ggml-medium.en.bin",
                "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.en.bin",
                "fd9727b6e1217c2f614f9b698455c4ffd82463b4",
                769000000,
                "English-only medium model. Very high accuracy for English.",
                {0.4f, 92.0f, 1000, false},
                {{"en"}, false, false},
                false, false, ""
            }},
            {"large-v1", {
                "large-v1", "Large v1", "ggml-large-v1.bin",
                "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v1.bin",
                "b1caaf735c4e49c181e2a4b4f83aeb5e4d2b7b7e",
                1550000000, // ~1.55GB
                "Large v1 model with excellent accuracy. Requires significant processing power.",
                {0.2f, 95.0f, 2000, false},
                {{"en", "es", "fr", "de", "it", "pt", "ru", "ja", "ko", "zh"}, true, true},
                false, false, ""
            }},
            {"large-v2", {
                "large-v2", "Large v2", "ggml-large-v2.bin", 
                "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v2.bin",
                "1ab0fb0e3d74e4e6cb85b7fa7e59b2983b39de6f",
                1550000000,
                "Large v2 model. Latest version with best accuracy. Very slow processing.",
                {0.15f, 97.0f, 2000, false},
                {{"en", "es", "fr", "de", "it", "pt", "ru", "ja", "ko", "zh"}, true, true},
                false, false, ""
            }},
            {"large-v3", {
                "large-v3", "Large v3", "ggml-large-v3.bin",
                "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3.bin", 
                "ad82bf6a9043ceed55076a0c556cb5ec7fa2b6b4",
                1550000000,
                "Large v3 model. State-of-the-art accuracy. Requires high-end hardware.",
                {0.1f, 98.0f, 2000, false},
                {{"en", "es", "fr", "de", "it", "pt", "ru", "ja", "ko", "zh"}, true, true},
                false, false, ""
            }}
        };
    }
    
    void updateModelStatus() {
        for (auto& [id, model] : available_models) {
            std::string model_path = models_directory + "/" + model.filename;
            model.is_downloaded = fs::exists(model_path);
            model.local_path = model.is_downloaded ? model_path : "";
            
            if (model.is_downloaded) {
                model.is_verified = verifyModelIntegrity(id);
            }
        }
    }
    
    bool verifyModelIntegrity(const std::string& model_id) {
        auto it = available_models.find(model_id);
        if (it == available_models.end()) return false;
        
        const ModelInfo& model = it->second;
        if (!fs::exists(model.local_path)) return false;
        
        // Check file size first (quick check)
        try {
            auto file_size = fs::file_size(model.local_path);
            
            // Allow some tolerance for file size (±5%)
            uint64_t min_size = static_cast<uint64_t>(model.size_bytes * 0.95);
            uint64_t max_size = static_cast<uint64_t>(model.size_bytes * 1.05);
            
            if (file_size < min_size || file_size > max_size) {
                LOG_WARN("ModelManager", "File size out of range for " + model_id +
                                         " (expected: ~" + std::to_string(model.size_bytes) +
                                         ", actual: " + std::to_string(file_size) + ")");
                return false;
            }
        } catch (const fs::filesystem_error& e) {
            LOG_ERROR("ModelManager", "Error checking file size for " + model_id + ": " + e.what());
            return false;
        }
        
        // Verify with whisper.cpp if available
#ifdef WHISPER_AVAILABLE
        whisper_context* ctx = whisper_init_from_file(model.local_path.c_str());
        if (!ctx) {
            LOG_WARN("ModelManager", "Failed to load model with whisper.cpp for verification: " + model_id);
            return false;
        }
        whisper_free(ctx);
#endif
        
        return true;
    }
    
    // Real HTTP download implementation using Qt Network
    bool downloadModelReal(const std::string& model_id) {
        auto it = available_models.find(model_id);
        if (it == available_models.end()) return false;
        
        const ModelInfo& model = it->second;
        auto download_state = active_downloads[model_id].get();
        
        LOG_INFO("ModelManager", "Starting download for model: " + model_id);
        LOG_INFO("ModelManager", "Download URL: " + model.url);
        LOG_INFO("ModelManager", "Expected size: " + std::to_string(model.size_bytes) + " bytes");
        
        // Initialize network manager if needed
        if (!network_manager) {
            network_manager = new QNetworkAccessManager();
        }
        
        download_state->start_time = std::chrono::steady_clock::now();
        
        // Create network request
        QNetworkRequest request(QUrl(QString::fromStdString(model.url)));
        request.setRawHeader("User-Agent", "WhisperApp/1.0");
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        
        // Start download
        QNetworkReply* reply = network_manager->get(request);
        if (!reply) {
            download_state->completion_callback(false, "Failed to create network request");
            return false;
        }
        
        // Open temporary file
        QFile temp_file(QString::fromStdString(download_state->temp_file_path));
        if (!temp_file.open(QIODevice::WriteOnly)) {
            reply->deleteLater();
            download_state->completion_callback(false, "Failed to create temporary file");
            return false;
        }
        
        // Create event loop to handle download synchronously in this thread
        QEventLoop loop;
        bool download_success = false;
        QString error_message;
        
        // Handle download progress
        QObject::connect(reply, &QNetworkReply::downloadProgress, 
                        [&](qint64 bytesReceived, qint64 bytesTotal) {
            if (download_state->should_cancel) {
                reply->abort();
                return;
            }
            
            download_state->bytes_downloaded = bytesReceived;
            if (bytesTotal > 0) {
                download_state->total_bytes = bytesTotal;
            }
            
            // Update progress callback
            if (download_state->progress_callback && bytesTotal > 0) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - download_state->start_time);
                
                DownloadProgress progress;
                progress.model_id = model_id;
                progress.bytes_received = bytesReceived;
                progress.bytes_total = bytesTotal;
                progress.progress_percent = (float(bytesReceived) / bytesTotal) * 100.0f;
                
                if (elapsed.count() > 0) {
                    progress.speed_mbps = (float(bytesReceived) / (1024 * 1024)) / elapsed.count();
                    if (progress.speed_mbps > 0) {
                        uint64_t remaining_bytes = bytesTotal - bytesReceived;
                        progress.eta_seconds = int(remaining_bytes / (progress.speed_mbps * 1024 * 1024));
                    }
                }
                
                download_state->progress_callback(progress);
            }
        });
        
        // Handle ready read (data available)
        QObject::connect(reply, &QNetworkReply::readyRead, [&]() {
            if (!download_state->should_cancel) {
                temp_file.write(reply->readAll());
            }
        });
        
        // Handle completion
        QObject::connect(reply, &QNetworkReply::finished, [&]() {
            temp_file.close();
            
            if (reply->error() == QNetworkReply::NoError && !download_state->should_cancel) {
                // Download completed successfully
                try {
                    fs::rename(download_state->temp_file_path, download_state->final_file_path);
                    download_success = true;
                    LOG_INFO("ModelManager", "Download completed successfully for " + model_id);
                } catch (const fs::filesystem_error& e) {
                    error_message = QString("Failed to move file: %1").arg(e.what());
                    fs::remove(download_state->temp_file_path);
                    LOG_ERROR("ModelManager", "Failed to move downloaded file for " + model_id + ": " + e.what());
                }
            } else {
                // Download failed or was cancelled
                fs::remove(download_state->temp_file_path);
                if (download_state->should_cancel) {
                    error_message = "Download cancelled";
                    LOG_INFO("ModelManager", "Download cancelled for " + model_id);
                } else {
                    error_message = QString("Network error: %1").arg(reply->errorString());
                    LOG_ERROR("ModelManager", "Download failed for " + model_id + ": " + error_message.toStdString());
                }
            }
            
            loop.quit();
        });
        
        // Start the event loop
        loop.exec();
        
        // Clean up
        reply->deleteLater();
        
        // Call completion callback
        download_state->completion_callback(download_success, error_message.toStdString());
        
        return download_success;
    }
    
    ~Impl() {
        if (network_manager) {
            delete network_manager;
        }
    }
};

ModelManager::ModelManager() : pImpl(std::make_unique<Impl>()) {
    pImpl->initializeModelDatabase();
}

ModelManager::~ModelManager() = default;

bool ModelManager::initialize(const std::string& models_directory) {
    pImpl->models_directory = models_directory;
    
    // Create models directory if it doesn't exist
    try {
        fs::create_directories(models_directory);
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("ModelManager", "Failed to create models directory " + models_directory + ": " + e.what());
        return false;
    }
    
    // Update model status based on local files
    pImpl->updateModelStatus();
    
    LOG_INFO("ModelManager", "Initialized with models directory: " + models_directory);
    return true;
}

std::vector<ModelInfo> ModelManager::getAvailableModels() const {
    std::vector<ModelInfo> models;
    for (const auto& [id, model] : pImpl->available_models) {
        models.push_back(model);
    }
    return models;
}

ModelInfo ModelManager::getModelInfo(const std::string& model_id) const {
    auto it = pImpl->available_models.find(model_id);
    if (it != pImpl->available_models.end()) {
        return it->second;
    }
    return ModelInfo{}; // Return empty struct if not found
}

std::vector<std::string> ModelManager::getDownloadedModels() const {
    std::vector<std::string> downloaded;
    for (const auto& [id, model] : pImpl->available_models) {
        if (model.is_downloaded) {
            downloaded.push_back(id);
        }
    }
    return downloaded;
}

bool ModelManager::isModelDownloaded(const std::string& model_id) const {
    auto it = pImpl->available_models.find(model_id);
    return it != pImpl->available_models.end() && it->second.is_downloaded;
}

std::string ModelManager::getModelPath(const std::string& model_id) const {
    auto it = pImpl->available_models.find(model_id);
    if (it != pImpl->available_models.end() && it->second.is_downloaded) {
        return it->second.local_path;
    }
    return "";
}

bool ModelManager::downloadModel(const std::string& model_id,
                                ProgressCallback progress_callback,
                                CompletionCallback completion_callback) {
    std::lock_guard<std::mutex> lock(pImpl->downloads_mutex);
    
    // Check if already downloading
    if (pImpl->active_downloads.find(model_id) != pImpl->active_downloads.end()) {
        return false;
    }
    
    auto it = pImpl->available_models.find(model_id);
    if (it == pImpl->available_models.end()) {
        completion_callback(false, "Model not found: " + model_id);
        return false;
    }
    
    const ModelInfo& model = it->second;
    if (model.is_downloaded) {
        completion_callback(false, "Model already downloaded: " + model_id);
        return false;
    }
    
    // Create download state
    auto download_state = std::make_unique<DownloadState>();
    download_state->temp_file_path = pImpl->models_directory + "/" + model.filename + ".tmp";
    download_state->final_file_path = pImpl->models_directory + "/" + model.filename;
    download_state->progress_callback = progress_callback;
    download_state->completion_callback = [this, model_id, completion_callback](bool success, const std::string& error) {
        std::lock_guard<std::mutex> lock(pImpl->downloads_mutex);
        pImpl->active_downloads.erase(model_id);
        
        if (success) {
            // Update model status
            pImpl->updateModelStatus();
        }
        
        completion_callback(success, error);
    };
    download_state->is_active = true;
    
    pImpl->active_downloads[model_id] = std::move(download_state);
    
    // Start download in separate thread
    std::thread download_thread([this, model_id]() {
        pImpl->downloadModelReal(model_id);
    });
    
    download_thread.detach();
    return true;
}

void ModelManager::cancelDownload(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(pImpl->downloads_mutex);
    
    auto it = pImpl->active_downloads.find(model_id);
    if (it != pImpl->active_downloads.end()) {
        it->second->should_cancel = true;
    }
}

bool ModelManager::isDownloading(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(pImpl->downloads_mutex);
    return pImpl->active_downloads.find(model_id) != pImpl->active_downloads.end();
}

bool ModelManager::deleteModel(const std::string& model_id) {
    auto it = pImpl->available_models.find(model_id);
    if (it == pImpl->available_models.end() || !it->second.is_downloaded) {
        return false;
    }
    
    try {
        fs::remove(it->second.local_path);
        pImpl->updateModelStatus();
        LOG_INFO("ModelManager", "Deleted model: " + model_id);
        return true;
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("ModelManager", "Failed to delete model " + model_id + ": " + e.what());
        return false;
    }
}

bool ModelManager::verifyModel(const std::string& model_id) {
    return pImpl->verifyModelIntegrity(model_id);
}

uint64_t ModelManager::getTotalDiskUsage() const {
    uint64_t total = 0;
    for (const auto& [id, model] : pImpl->available_models) {
        if (model.is_downloaded) {
            try {
                total += fs::file_size(model.local_path);
            } catch (const fs::filesystem_error&) {
                // Ignore errors
            }
        }
    }
    return total;
}

uint64_t ModelManager::getAvailableDiskSpace() const {
    try {
        auto space_info = fs::space(pImpl->models_directory);
        return space_info.available;
    } catch (const fs::filesystem_error&) {
        return 0;
    }
}

void ModelManager::checkForUpdates(UpdateCallback callback) {
    // For now, we don't support model updates
    // In a future version, this could check model versions
    callback({});
}

void ModelManager::setDownloadSpeedLimit(float limit_mbps) {
    pImpl->download_speed_limit = limit_mbps;
}

std::string ModelManager::getRecommendedModel() const {
    // Simple recommendation based on system capabilities
    // TODO: Could be enhanced with actual system detection
    return "base.en"; // Good balance of speed and accuracy for most users
}

bool ModelManager::importModel(const std::string& file_path, const std::string& model_id) {
    auto it = pImpl->available_models.find(model_id);
    if (it == pImpl->available_models.end()) {
        return false;
    }
    
    std::string dest_path = pImpl->models_directory + "/" + it->second.filename;
    
    try {
        fs::copy_file(file_path, dest_path, fs::copy_options::overwrite_existing);
        pImpl->updateModelStatus();
        LOG_INFO("ModelManager", "Imported model " + model_id + " from " + file_path);
        return true;
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("ModelManager", "Failed to import model " + model_id + " from " + file_path + ": " + e.what());
        return false;
    }
}

bool ModelManager::exportModel(const std::string& model_id, const std::string& destination_path) {
    auto it = pImpl->available_models.find(model_id);
    if (it == pImpl->available_models.end() || !it->second.is_downloaded) {
        return false;
    }
    
    try {
        fs::copy_file(it->second.local_path, destination_path, fs::copy_options::overwrite_existing);
        LOG_INFO("ModelManager", "Exported model " + model_id + " to " + destination_path);
        return true;
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("ModelManager", "Failed to export model " + model_id + " to " + destination_path + ": " + e.what());
        return false;
    }
}