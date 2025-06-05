#include "Localization.h"
#include "Logger.h"
#include <QLocale>
#include <QTranslator>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

class Localization::Impl {
public:
    QString currentLanguage;
    std::map<QString, QString> translations;
    std::map<QString, LanguageInfo> supportedLanguages;
    QTranslator* translator = nullptr;
    
    Impl() {
        // Initialize supported languages
        supportedLanguages = {
            {"en_US", {"en_US", "English", "English (US)", "🇺🇸"}},
            {"es_ES", {"es_ES", "Español", "Spanish", "🇪🇸"}},
            {"fr_FR", {"fr_FR", "Français", "French", "🇫🇷"}},
            {"de_DE", {"de_DE", "Deutsch", "German", "🇩🇪"}},
            {"zh_CN", {"zh_CN", "中文", "Chinese (Simplified)", "🇨🇳"}},
            {"ja_JP", {"ja_JP", "日本語", "Japanese", "🇯🇵"}}
        };
        
        // Set default language
        currentLanguage = QLocale::system().name();
        if (supportedLanguages.find(currentLanguage) == supportedLanguages.end()) {
            currentLanguage = "en_US";
        }
    }
    
    ~Impl() {
        if (translator) {
            QCoreApplication::removeTranslator(translator);
            delete translator;
        }
    }
};

Localization& Localization::instance() {
    static Localization instance;
    return instance;
}

Localization::Localization() : pImpl(std::make_unique<Impl>()) {}
Localization::~Localization() = default;

bool Localization::loadLanguage(const QString& language) {
    if (pImpl->supportedLanguages.find(language) == pImpl->supportedLanguages.end()) {
        LOG_ERROR("Localization", "Unsupported language: " + language.toStdString());
        return false;
    }
    
    // Remove previous translator
    if (pImpl->translator) {
        QCoreApplication::removeTranslator(pImpl->translator);
        delete pImpl->translator;
    }
    
    // Load new translator
    pImpl->translator = new QTranslator();
    QString translationFile = QString(":/translations/whisperapp_%1").arg(language);
    
    if (pImpl->translator->load(translationFile)) {
        QCoreApplication::installTranslator(pImpl->translator);
        pImpl->currentLanguage = language;
        LOG_INFO("Localization", "Loaded language: " + language.toStdString());
        emit languageChanged(language);
        return true;
    } else {
        LOG_WARN("Localization", "Failed to load translation file: " + translationFile.toStdString());
        delete pImpl->translator;
        pImpl->translator = nullptr;
        return false;
    }
}

QString Localization::getCurrentLanguage() const {
    return pImpl->currentLanguage;
}

std::vector<LanguageInfo> Localization::getSupportedLanguages() const {
    std::vector<LanguageInfo> result;
    for (const auto& pair : pImpl->supportedLanguages) {
        result.push_back(pair.second);
    }
    return result;
}

QString Localization::getLanguageName(const QString& code) const {
    auto it = pImpl->supportedLanguages.find(code);
    if (it != pImpl->supportedLanguages.end()) {
        return it->second.nativeName;
    }
    return code;
}

QString Localization::translate(const QString& key, const QString& defaultValue) const {
    // For now, just return the default value
    // In a real implementation, this would look up the translation
    return defaultValue.isEmpty() ? key : defaultValue;
}

// Convenience function for translation
QString tr(const QString& key, const QString& defaultValue) {
    return Localization::instance().translate(key, defaultValue);
}