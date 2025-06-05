/*
 * Localization.h
 * 
 * Localization support for WhisperApp
 * Provides translation infrastructure and string management
 */

#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <QString>
#include <QObject>
#include <QTranslator>
#include <QLocale>
#include <QCoreApplication>
#include <memory>
#include <unordered_map>

namespace WhisperApp {

/**
 * @brief Localization manager for handling translations
 */
class LocalizationManager : public QObject {
    Q_OBJECT
    
public:
    /**
     * @brief Get singleton instance
     */
    static LocalizationManager& instance() {
        static LocalizationManager instance;
        return instance;
    }
    
    /**
     * @brief Available languages
     */
    enum class Language {
        English,
        Spanish,
        French,
        German,
        Chinese,
        Japanese,
        Korean,
        Russian,
        Portuguese,
        Italian
    };
    
    /**
     * @brief Initialize localization system
     */
    void initialize();
    
    /**
     * @brief Load translation for specified language
     * @param language Language to load
     * @return true if successful
     */
    bool loadLanguage(Language language);
    
    /**
     * @brief Load translation for specified locale
     * @param locale Locale string (e.g., "en_US", "es_ES")
     * @return true if successful
     */
    bool loadLanguage(const QString& locale);
    
    /**
     * @brief Get current language
     */
    Language currentLanguage() const { return m_currentLanguage; }
    
    /**
     * @brief Get current locale
     */
    QLocale currentLocale() const { return m_currentLocale; }
    
    /**
     * @brief Get available languages
     */
    QList<QPair<Language, QString>> availableLanguages() const;
    
    /**
     * @brief Get language display name
     */
    QString languageDisplayName(Language language) const;
    
    /**
     * @brief Get locale string for language
     */
    QString languageToLocale(Language language) const;
    
    /**
     * @brief Convert locale string to language enum
     */
    Language localeToLanguage(const QString& locale) const;

signals:
    /**
     * @brief Emitted when language changes
     */
    void languageChanged(Language language);
    
private:
    LocalizationManager() = default;
    ~LocalizationManager() = default;
    
    // Delete copy constructor and assignment operator
    LocalizationManager(const LocalizationManager&) = delete;
    LocalizationManager& operator=(const LocalizationManager&) = delete;
    
    Language m_currentLanguage = Language::English;
    QLocale m_currentLocale;
    std::unique_ptr<QTranslator> m_translator;
    std::unique_ptr<QTranslator> m_qtTranslator;
    
    std::unordered_map<Language, QString> m_languageToLocaleMap;
    std::unordered_map<QString, Language> m_localeToLanguageMap;
};

/**
 * @brief Convenience macros for translation
 */
#define TR(text) QCoreApplication::translate("WhisperApp", text)
#define TR_CONTEXT(context, text) QCoreApplication::translate(context, text)

/**
 * @brief String ID constants for consistent translation keys
 */
namespace StringId {
    // Application
    constexpr const char* APP_NAME = "WhisperApp";
    constexpr const char* APP_DESCRIPTION = "Real-time speech to text transcription";
    
    // Main Window
    constexpr const char* MAIN_WINDOW_TITLE = "WhisperApp - Speech to Text";
    constexpr const char* FILE_MENU = "&File";
    constexpr const char* EDIT_MENU = "&Edit";
    constexpr const char* VIEW_MENU = "&View";
    constexpr const char* TOOLS_MENU = "&Tools";
    constexpr const char* HELP_MENU = "&Help";
    
    // Actions
    constexpr const char* ACTION_NEW = "&New";
    constexpr const char* ACTION_OPEN = "&Open...";
    constexpr const char* ACTION_SAVE = "&Save";
    constexpr const char* ACTION_SAVE_AS = "Save &As...";
    constexpr const char* ACTION_EXIT = "E&xit";
    constexpr const char* ACTION_RECORD = "&Record";
    constexpr const char* ACTION_STOP = "&Stop";
    constexpr const char* ACTION_SETTINGS = "&Settings...";
    constexpr const char* ACTION_ABOUT = "&About WhisperApp";
    
    // Status messages
    constexpr const char* STATUS_READY = "Ready";
    constexpr const char* STATUS_RECORDING = "Recording...";
    constexpr const char* STATUS_PROCESSING = "Processing...";
    constexpr const char* STATUS_TRANSCRIBING = "Transcribing...";
    constexpr const char* STATUS_COMPLETE = "Transcription complete";
    constexpr const char* STATUS_ERROR = "Error: %1";
    
    // Buttons
    constexpr const char* BUTTON_START_RECORDING = "Start Recording";
    constexpr const char* BUTTON_STOP_RECORDING = "Stop Recording";
    constexpr const char* BUTTON_OK = "OK";
    constexpr const char* BUTTON_CANCEL = "Cancel";
    constexpr const char* BUTTON_APPLY = "Apply";
    constexpr const char* BUTTON_CLOSE = "Close";
    constexpr const char* BUTTON_BROWSE = "Browse...";
    constexpr const char* BUTTON_DOWNLOAD = "Download";
    
    // Labels
    constexpr const char* LABEL_MODEL = "Model:";
    constexpr const char* LABEL_LANGUAGE = "Language:";
    constexpr const char* LABEL_DEVICE = "Device:";
    constexpr const char* LABEL_SAMPLE_RATE = "Sample Rate:";
    constexpr const char* LABEL_QUALITY = "Quality:";
    
    // Messages
    constexpr const char* MSG_CONFIRM_EXIT = "Are you sure you want to exit?";
    constexpr const char* MSG_UNSAVED_CHANGES = "You have unsaved changes. Do you want to save them?";
    constexpr const char* MSG_MODEL_DOWNLOAD_REQUIRED = "This model needs to be downloaded first. Download now?";
    constexpr const char* MSG_RECORDING_IN_PROGRESS = "Recording is in progress. Stop recording?";
    
    // Errors
    constexpr const char* ERROR_MODEL_NOT_FOUND = "Model file not found";
    constexpr const char* ERROR_MICROPHONE_NOT_AVAILABLE = "Microphone not available";
    constexpr const char* ERROR_TRANSCRIPTION_FAILED = "Transcription failed";
    constexpr const char* ERROR_FILE_SAVE_FAILED = "Failed to save file";
    
    // Tooltips
    constexpr const char* TOOLTIP_RECORD_BUTTON = "Click to start or stop recording (Ctrl+R)";
    constexpr const char* TOOLTIP_MODEL_COMBO = "Select the AI model for transcription";
    constexpr const char* TOOLTIP_LANGUAGE_COMBO = "Select the language for transcription";
    constexpr const char* TOOLTIP_AUDIO_LEVEL = "Real-time audio level indicator";
}

} // namespace WhisperApp

#endif // LOCALIZATION_H