/*
 * SettingsDialog.h
 * 
 * Settings dialog for configuring application preferences.
 * Provides a tabbed interface for organizing various settings categories.
 * 
 * Features:
 * - General application settings
 * - Audio device configuration
 * - Transcription settings
 * - Hotkey configuration
 * - UI preferences
 * - Advanced options
 */

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <memory>
#include <map>
#include <string>

// Forward declarations
class QTabWidget;
class QDialogButtonBox;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QLineEdit;
class QSlider;
class QLabel;
class QPushButton;
class QListWidget;
class QKeySequenceEdit;
class Settings;
class AudioCapture;
class ModelManager;

/**
 * @brief Settings dialog for application configuration
 */
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(Settings* settings, 
                          AudioCapture* audio_capture,
                          ModelManager* model_manager,
                          QWidget* parent = nullptr);
    ~SettingsDialog();

    /**
     * @brief Show dialog and focus on specific tab
     * @param tab_name Name of tab to focus (empty = last used)
     */
    void showTab(const QString& tab_name = QString());

signals:
    /**
     * @brief Emitted when settings are applied
     */
    void settingsApplied();

    /**
     * @brief Emitted when specific setting changes
     * @param key Setting key
     * @param value New value
     */
    void settingChanged(const QString& key, const QVariant& value);

protected:
    /**
     * @brief Handle accept (OK button)
     */
    void accept() override;

    /**
     * @brief Handle reject (Cancel button)
     */
    void reject() override;

private slots:
    // General tab slots
    void onAutoStartChanged(bool checked);
    void onStartMinimizedChanged(bool checked);
    void onCheckUpdatesChanged(bool checked);
    void onLanguageChanged(int index);
    void onThemeChanged(int index);

    // Audio tab slots
    void onAudioDeviceChanged(int index);
    void onLoopbackModeChanged(bool checked);
    void onNoiseSuppressionChanged(bool checked);
    void onSilenceThresholdChanged(int value);
    void onSilenceDurationChanged(int value);
    void onTestAudioDevice();
    void onRefreshAudioDevices();

    // Transcription tab slots
    void onModelChanged(int index);
    void onTargetLanguageChanged(int index);
    void onTranslateEnglishChanged(bool checked);
    void onThreadCountChanged(int value);
    void onGPUEnabledChanged(bool checked);
    void onDownloadModel();
    void onDeleteModel();
    void onModelInfoRequested();

    // Hotkeys tab slots
    void onHotkeyChanged(const QString& action);
    void onClearHotkey();
    void onResetHotkeys();
    void onDetectConflicts();

    // Interface tab slots
    void onShowTrayIconChanged(bool checked);
    void onTrayNotificationsChanged(bool checked);
    void onFontSizeChanged(int value);
    void onWordWrapChanged(bool checked);
    void onShowTimestampsChanged(bool checked);

    // Advanced tab slots
    void onLogLevelChanged(int index);
    void onModelsDirectoryChanged();
    void onBrowseModelsDirectory();
    void onDownloadSpeedLimitChanged(int value);
    void onTelemetryChanged(bool checked);
    void onClearCache();
    void onResetSettings();
    void onExportSettings();
    void onImportSettings();

    // Button slots
    void onApply();
    void onRestoreDefaults();

private:
    /**
     * @brief Create general settings tab
     * @return Tab widget
     */
    QWidget* createGeneralTab();

    /**
     * @brief Create audio settings tab
     * @return Tab widget
     */
    QWidget* createAudioTab();

    /**
     * @brief Create transcription settings tab
     * @return Tab widget
     */
    QWidget* createTranscriptionTab();

    /**
     * @brief Create hotkeys settings tab
     * @return Tab widget
     */
    QWidget* createHotkeysTab();

    /**
     * @brief Create interface settings tab
     * @return Tab widget
     */
    QWidget* createInterfaceTab();

    /**
     * @brief Create advanced settings tab
     * @return Tab widget
     */
    QWidget* createAdvancedTab();

    /**
     * @brief Load current settings into UI
     */
    void loadSettings();

    /**
     * @brief Save UI values to settings
     */
    void saveSettings();

    /**
     * @brief Apply settings changes
     */
    void applySettings();

    /**
     * @brief Check if settings have been modified
     * @return true if modified
     */
    bool hasUnsavedChanges() const;

    /**
     * @brief Update audio device list
     */
    void updateAudioDevices();

    /**
     * @brief Update model list
     */
    void updateModelList();

    /**
     * @brief Update language list
     */
    void updateLanguageList();

    /**
     * @brief Update theme list
     */
    void updateThemeList();

    /**
     * @brief Validate current settings
     * @return true if valid
     */
    bool validateSettings();

    /**
     * @brief Show validation error
     * @param message Error message
     */
    void showValidationError(const QString& message);

private:
    // Main UI components
    QTabWidget* tab_widget = nullptr;
    QDialogButtonBox* button_box = nullptr;
    QPushButton* apply_button = nullptr;
    QPushButton* restore_defaults_button = nullptr;

    // General tab widgets
    QCheckBox* auto_start_checkbox = nullptr;
    QCheckBox* start_minimized_checkbox = nullptr;
    QCheckBox* check_updates_checkbox = nullptr;
    QComboBox* language_combo = nullptr;
    QComboBox* theme_combo = nullptr;

    // Audio tab widgets
    QComboBox* audio_device_combo = nullptr;
    QCheckBox* loopback_checkbox = nullptr;
    QCheckBox* noise_suppression_checkbox = nullptr;
    QSlider* silence_threshold_slider = nullptr;
    QLabel* silence_threshold_label = nullptr;
    QSpinBox* silence_duration_spin = nullptr;
    QPushButton* test_audio_button = nullptr;
    QPushButton* refresh_devices_button = nullptr;

    // Transcription tab widgets
    QComboBox* model_combo = nullptr;
    QComboBox* target_language_combo = nullptr;
    QCheckBox* translate_english_checkbox = nullptr;
    QSpinBox* thread_count_spin = nullptr;
    QCheckBox* gpu_enabled_checkbox = nullptr;
    QPushButton* download_model_button = nullptr;
    QPushButton* delete_model_button = nullptr;
    QPushButton* model_info_button = nullptr;
    QLabel* model_size_label = nullptr;
    QLabel* model_performance_label = nullptr;

    // Hotkeys tab widgets
    struct HotkeyWidget {
        QLabel* label = nullptr;
        QKeySequenceEdit* key_edit = nullptr;
        QPushButton* clear_button = nullptr;
    };
    std::map<std::string, HotkeyWidget> hotkey_widgets;
    QPushButton* reset_hotkeys_button = nullptr;
    QPushButton* detect_conflicts_button = nullptr;
    QLabel* conflict_label = nullptr;

    // Interface tab widgets
    QCheckBox* show_tray_icon_checkbox = nullptr;
    QCheckBox* tray_notifications_checkbox = nullptr;
    QSpinBox* font_size_spin = nullptr;
    QCheckBox* word_wrap_checkbox = nullptr;
    QCheckBox* show_timestamps_checkbox = nullptr;

    // Advanced tab widgets
    QComboBox* log_level_combo = nullptr;
    QLineEdit* models_directory_edit = nullptr;
    QPushButton* browse_models_button = nullptr;
    QSpinBox* download_speed_spin = nullptr;
    QCheckBox* telemetry_checkbox = nullptr;
    QPushButton* clear_cache_button = nullptr;
    QPushButton* reset_settings_button = nullptr;
    QPushButton* export_settings_button = nullptr;
    QPushButton* import_settings_button = nullptr;

    // Core components
    Settings* settings = nullptr;
    AudioCapture* audio_capture = nullptr;
    ModelManager* model_manager = nullptr;

    // State tracking
    std::map<std::string, QVariant> original_values;
    bool is_applying = false;
};

#endif // SETTINGSDIALOG_H