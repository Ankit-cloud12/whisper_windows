#include "Localization.h"
#include <QDir>
#include <QApplication>
#include <QDebug>

namespace WhisperApp {

void LocalizationManager::initialize() {
    // Initialize language mappings
    m_languageToLocaleMap = {
        {Language::English, "en_US"},
        {Language::Spanish, "es_ES"},
        {Language::French, "fr_FR"},
        {Language::German, "de_DE"},
        {Language::Chinese, "zh_CN"},
        {Language::Japanese, "ja_JP"},
        {Language::Korean, "ko_KR"},
        {Language::Russian, "ru_RU"},
        {Language::Portuguese, "pt_BR"},
        {Language::Italian, "it_IT"}
    };
    
    // Create reverse mapping
    for (const auto& pair : m_languageToLocaleMap) {
        m_localeToLanguageMap[pair.second] = pair.first;
    }
    
    // Create translators
    m_translator = std::make_unique<QTranslator>();
    m_qtTranslator = std::make_unique<QTranslator>();
    
    // Install translators
    QCoreApplication::installTranslator(m_translator.get());
    QCoreApplication::installTranslator(m_qtTranslator.get());
    
    // Load system locale by default
    QString systemLocale = QLocale::system().name();
    if (!loadLanguage(systemLocale)) {
        // Fall back to English if system locale not supported
        loadLanguage(Language::English);
    }
}

bool LocalizationManager::loadLanguage(Language language) {
    QString locale = languageToLocale(language);
    return loadLanguage(locale);
}

bool LocalizationManager::loadLanguage(const QString& locale) {
    // Set current locale
    m_currentLocale = QLocale(locale);
    QLocale::setDefault(m_currentLocale);
    
    // Load application translations
    QString translationPath = QApplication::applicationDirPath() + "/translations";
    bool appLoaded = m_translator->load(QString("whisperapp_%1").arg(locale), translationPath);
    
    // Load Qt translations
    bool qtLoaded = m_qtTranslator->load(QString("qt_%1").arg(locale), translationPath);
    
    if (appLoaded || locale == "en_US") {  // English is built-in
        m_currentLanguage = localeToLanguage(locale);
        emit languageChanged(m_currentLanguage);
        return true;
    }
    
    qWarning() << "Failed to load translations for locale:" << locale;
    return false;
}

QList<QPair<LocalizationManager::Language, QString>> LocalizationManager::availableLanguages() const {
    QList<QPair<Language, QString>> languages;
    
    for (const auto& pair : m_languageToLocaleMap) {
        languages.append({pair.first, languageDisplayName(pair.first)});
    }
    
    return languages;
}

QString LocalizationManager::languageDisplayName(Language language) const {
    switch (language) {
        case Language::English: return tr("English");
        case Language::Spanish: return tr("Spanish");
        case Language::French: return tr("French");
        case Language::German: return tr("German");
        case Language::Chinese: return tr("Chinese (Simplified)");
        case Language::Japanese: return tr("Japanese");
        case Language::Korean: return tr("Korean");
        case Language::Russian: return tr("Russian");
        case Language::Portuguese: return tr("Portuguese (Brazil)");
        case Language::Italian: return tr("Italian");
        default: return tr("Unknown");
    }
}

QString LocalizationManager::languageToLocale(Language language) const {
    auto it = m_languageToLocaleMap.find(language);
    if (it != m_languageToLocaleMap.end()) {
        return it->second;
    }
    return "en_US";  // Default to English
}

LocalizationManager::Language LocalizationManager::localeToLanguage(const QString& locale) const {
    // Try exact match first
    auto it = m_localeToLanguageMap.find(locale);
    if (it != m_localeToLanguageMap.end()) {
        return it->second;
    }
    
    // Try language code only (e.g., "en" from "en_GB")
    QString langCode = locale.left(2);
    for (const auto& pair : m_localeToLanguageMap) {
        if (pair.first.startsWith(langCode)) {
            return pair.second;
        }
    }
    
    return Language::English;  // Default to English
}

} // namespace WhisperApp