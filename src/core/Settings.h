/*
 * Settings.h
 * 
 * Application settings management with persistence.
 * This class handles all user preferences and configuration options.
 * 
 * Features:
 * - Type-safe settings access
 * - Automatic persistence to disk
 * - Default values and validation
 * - Import/export functionality
 * - Settings change notifications
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <memory>
#include <functional>
#include <variant>
#include <map>
#include <vector>

// Forward declarations
class QSettings;

/**
 * @brief Settings value type
 */
using SettingsValue = std::variant<
    bool,
    int,
    float,
    std::string,
    std::vector<std::string>
>;

/**
 * @brief Settings categories
 */
namespace SettingsCategory {
    constexpr const char* GENERAL = "General";
    constexpr const char* AUDIO = "Audio";
    constexpr const char* TRANSCRIPTION = "Transcription";
    constexpr const char* UI = "UI";
    constexpr const char* HOTKEYS = "Hotkeys";
    constexpr const char* ADVANCED = "Advanced";
}

/**
 * @brief Common settings keys
 */
namespace SettingsKey {
    // General settings
    constexpr const char* AUTO_START = "General/AutoStart";
    constexpr const char* START_MINIMIZED = "General/StartMinimized";
    constexpr const char* CHECK_UPDATES = "General/CheckUpdates";
    constexpr const char* LANGUAGE = "General/Language";
    
    // Audio settings
    constexpr const char* AUDIO_DEVICE = "Audio/DeviceId";
    constexpr const char* LOOPBACK_ENABLED = "Audio/LoopbackEnabled";
    constexpr const char* NOISE_SUPPRESSION = "Audio/NoiseSuppression";
    constexpr const char* SILENCE_THRESHOLD = "Audio/SilenceThreshold";
    constexpr const char* SILENCE_DURATION = "Audio/SilenceDuration";
    
    // Transcription settings
    constexpr const char* MODEL_ID = "Transcription/ModelId";
    constexpr const char* TARGET_LANGUAGE = "Transcription/Language";
    constexpr const char* TRANSLATE_ENGLISH = "Transcription/TranslateToEnglish";
    constexpr const char* THREAD_COUNT = "Transcription/ThreadCount";
    constexpr const char* GPU_ENABLED = "Transcription/GPUEnabled";
    
    // UI settings
    constexpr const char* THEME = "UI/Theme";
    constexpr const char* WINDOW_GEOMETRY = "UI/WindowGeometry";
    constexpr const char* WINDOW_STATE = "UI/WindowState";
    constexpr const char* SHOW_TRAY_ICON = "UI/ShowTrayIcon";
    constexpr const char* TRAY_NOTIFICATIONS = "UI/TrayNotifications";
    
    // Hotkey settings
    constexpr const char* HOTKEY_START_STOP = "Hotkeys/StartStop";
    constexpr const char* HOTKEY_PUSH_TO_TALK = "Hotkeys/PushToTalk";
    constexpr const char* HOTKEY_INSERT_TEXT = "Hotkeys/InsertText";
    constexpr const char* HOTKEY_CLEAR = "Hotkeys/Clear";
    
    // Advanced settings
    constexpr const char* LOG_LEVEL = "Advanced/LogLevel";
    constexpr const char* MODELS_DIRECTORY = "Advanced/ModelsDirectory";
    constexpr const char* DOWNLOAD_SPEED_LIMIT = "Advanced/DownloadSpeedLimit";
    constexpr const char* TELEMETRY_ENABLED = "Advanced/TelemetryEnabled";
}

/**
 * @brief Main settings management class
 */
class Settings {
public:
    /**
     * @brief Settings change callback
     */
    using ChangeCallback = std::function<void(const std::string& key, const SettingsValue& value)>;

public:
    Settings();
    ~Settings();

    // Prevent copying
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    /**
     * @brief Get a setting value
     * @param key Setting key
     * @param default_value Default value if not found
     * @return Setting value
     */
    template<typename T>
    T value(const std::string& key, const T& default_value) const;

    /**
     * @brief Set a setting value
     * @param key Setting key
     * @param value Setting value
     */
    template<typename T>
    void setValue(const std::string& key, const T& value);

    /**
     * @brief Check if a setting exists
     * @param key Setting key
     * @return true if exists, false otherwise
     */
    bool contains(const std::string& key) const;

    /**
     * @brief Remove a setting
     * @param key Setting key
     */
    void remove(const std::string& key);

    /**
     * @brief Clear all settings
     */
    void clear();

    /**
     * @brief Get all keys in a category
     * @param category Category name
     * @return Vector of keys
     */
    std::vector<std::string> keysInCategory(const std::string& category) const;

    /**
     * @brief Reset settings to defaults
     * @param category Optional category to reset (empty = all)
     */
    void resetToDefaults(const std::string& category = "");

    /**
     * @brief Register a change callback
     * @param callback Callback function
     * @return Callback ID for unregistering
     */
    int registerChangeCallback(ChangeCallback callback);

    /**
     * @brief Unregister a change callback
     * @param callback_id Callback ID from registerChangeCallback
     */
    void unregisterChangeCallback(int callback_id);

    /**
     * @brief Export settings to file
     * @param file_path Export file path
     * @return true if successful, false otherwise
     */
    bool exportSettings(const std::string& file_path) const;

    /**
     * @brief Import settings from file
     * @param file_path Import file path
     * @return true if successful, false otherwise
     */
    bool importSettings(const std::string& file_path);

    /**
     * @brief Get settings file path
     * @return Path to settings file
     */
    std::string getSettingsFilePath() const;

    /**
     * @brief Sync settings to disk
     */
    void sync();

    /**
     * @brief Get default value for a setting
     * @param key Setting key
     * @return Default value
     */
    SettingsValue getDefaultValue(const std::string& key) const;

    /**
     * @brief Validate a setting value
     * @param key Setting key
     * @param value Value to validate
     * @return true if valid, false otherwise
     */
    bool validateValue(const std::string& key, const SettingsValue& value) const;

private:
    // Private implementation (PIMPL idiom)
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// Template implementation
template<typename T>
T Settings::value(const std::string& key, const T& default_value) const {
    // Implementation will be in Settings.cpp
    return default_value;
}

template<typename T>
void Settings::setValue(const std::string& key, const T& value) {
    // Implementation will be in Settings.cpp
}

#endif // SETTINGS_H