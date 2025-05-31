/*
 * ModelDownloader.h
 * 
 * UI dialog for downloading and managing Whisper models.
 * Provides a user-friendly interface for model selection and download.
 * 
 * Features:
 * - Model list with details (size, performance, languages)
 * - Download progress with speed and ETA
 * - Download queue management
 * - Model deletion and verification
 * - Disk space monitoring
 */

#ifndef MODELDOWNLOADER_H
#define MODELDOWNLOADER_H

#include <QDialog>
#include <memory>
#include <string>
#include <vector>
#include <map>

// Forward declarations
class QTableWidget;
class QProgressBar;
class QPushButton;
class QLabel;
class QTimer;
class ModelManager;
struct ModelInfo;
struct DownloadProgress;

/**
 * @brief Model download status
 */
enum class ModelStatus {
    NotDownloaded,      // Model not downloaded
    Downloading,        // Currently downloading
    Downloaded,         // Downloaded and verified
    UpdateAvailable,    // New version available
    Corrupted,          // Failed verification
    Queued             // In download queue
};

/**
 * @brief Model downloader dialog
 */
class ModelDownloader : public QDialog {
    Q_OBJECT

public:
    explicit ModelDownloader(ModelManager* model_manager, 
                           QWidget* parent = nullptr);
    ~ModelDownloader();

    /**
     * @brief Show dialog and optionally start download
     * @param model_id Model to pre-select (empty = none)
     */
    void showWithModel(const QString& model_id = QString());

    /**
     * @brief Get currently selected model
     * @return Selected model ID
     */
    QString getSelectedModel() const;

    /**
     * @brief Refresh model list
     */
    void refreshModelList();

signals:
    /**
     * @brief Emitted when a model is successfully downloaded
     * @param model_id Downloaded model ID
     */
    void modelDownloaded(const QString& model_id);

    /**
     * @brief Emitted when model selection changes
     * @param model_id Selected model ID
     */
    void modelSelected(const QString& model_id);

private slots:
    /**
     * @brief Handle model selection change
     * @param row Selected row
     * @param column Selected column
     */
    void onModelSelectionChanged(int row, int column);

    /**
     * @brief Handle download button click
     */
    void onDownloadClicked();

    /**
     * @brief Handle cancel download
     */
    void onCancelDownload();

    /**
     * @brief Handle delete model
     */
    void onDeleteModel();

    /**
     * @brief Handle verify model
     */
    void onVerifyModel();

    /**
     * @brief Handle download progress update
     * @param progress Download progress info
     */
    void onDownloadProgress(const DownloadProgress& progress);

    /**
     * @brief Handle download completion
     * @param model_id Model ID
     * @param success Success status
     * @param error Error message if failed
     */
    void onDownloadComplete(const QString& model_id, 
                           bool success, 
                           const QString& error);

    /**
     * @brief Update disk space display
     */
    void updateDiskSpace();

    /**
     * @brief Update download statistics
     */
    void updateDownloadStats();

    /**
     * @brief Check for model updates
     */
    void checkForUpdates();

    /**
     * @brief Handle timer timeout
     */
    void onTimerTimeout();

private:
    /**
     * @brief Setup UI components
     */
    void setupUi();

    /**
     * @brief Create model table
     */
    void createModelTable();

    /**
     * @brief Populate model table
     */
    void populateModelTable();

    /**
     * @brief Update model row
     * @param row Row index
     * @param model Model information
     * @param status Model status
     */
    void updateModelRow(int row, 
                       const ModelInfo& model, 
                       ModelStatus status);

    /**
     * @brief Get status for model
     * @param model_id Model ID
     * @return Model status
     */
    ModelStatus getModelStatus(const QString& model_id) const;

    /**
     * @brief Format file size for display
     * @param bytes Size in bytes
     * @return Formatted size string
     */
    QString formatFileSize(uint64_t bytes) const;

    /**
     * @brief Format download speed
     * @param mbps Speed in megabits per second
     * @return Formatted speed string
     */
    QString formatSpeed(float mbps) const;

    /**
     * @brief Format time remaining
     * @param seconds Seconds remaining
     * @return Formatted time string
     */
    QString formatTimeRemaining(int seconds) const;

    /**
     * @brief Update button states
     */
    void updateButtonStates();

    /**
     * @brief Show download details
     * @param model_id Model ID
     */
    void showModelDetails(const QString& model_id);

    /**
     * @brief Get icon for model status
     * @param status Model status
     * @return Status icon
     */
    QIcon getStatusIcon(ModelStatus status) const;

    /**
     * @brief Show confirmation dialog
     * @param title Dialog title
     * @param message Dialog message
     * @return true if confirmed
     */
    bool showConfirmation(const QString& title, 
                         const QString& message);

private:
    // UI components
    QTableWidget* model_table = nullptr;
    QProgressBar* download_progress = nullptr;
    QLabel* download_status_label = nullptr;
    QLabel* download_speed_label = nullptr;
    QLabel* download_eta_label = nullptr;
    QLabel* disk_space_label = nullptr;
    QPushButton* download_button = nullptr;
    QPushButton* cancel_button = nullptr;
    QPushButton* delete_button = nullptr;
    QPushButton* verify_button = nullptr;
    QPushButton* refresh_button = nullptr;
    QPushButton* close_button = nullptr;

    // Details panel
    QLabel* model_name_label = nullptr;
    QLabel* model_size_label = nullptr;
    QLabel* model_performance_label = nullptr;
    QLabel* model_languages_label = nullptr;
    QLabel* model_description_label = nullptr;

    // Core components
    ModelManager* model_manager = nullptr;

    // State tracking
    QString current_download_id;
    std::map<QString, ModelStatus> model_status_map;
    bool is_downloading = false;

    // Update timer
    QTimer* update_timer = nullptr;

    // Constants
    static constexpr int UPDATE_INTERVAL_MS = 1000;
    static constexpr int COLUMN_COUNT = 6;
    
    // Column indices
    enum ColumnIndex {
        COL_STATUS = 0,
        COL_NAME = 1,
        COL_SIZE = 2,
        COL_PERFORMANCE = 3,
        COL_LANGUAGES = 4,
        COL_DESCRIPTION = 5
    };
};

#endif // MODELDOWNLOADER_H