#ifndef APPINFO_H
#define APPINFO_H

#include <QString>

// Stub for AppInfo.h
// In a real application, these would likely be populated via CMake or build process.

namespace WhisperApp {
namespace AppInfo {

    const QString APP_NAME = "WhisperApp";
    const QString APP_VERSION = "1.0.0-stub";
    const QString APP_DISPLAY_NAME = "Whisper App (Stub)";
    const QString APP_ORGANIZATION = "WhisperAppOrg";
    const QString APP_DOMAIN = "whisperapp.example.com";
    const QString APP_DESCRIPTION = "A stub speech-to-text application.";

    inline QString getAppName() { return APP_NAME; }
    inline QString getAppVersion() { return APP_VERSION; }
    inline QString getAppDisplayName() { return APP_DISPLAY_NAME; }
    inline QString getOrganizationName() { return APP_ORGANIZATION; }
    inline QString getOrganizationDomain() { return APP_DOMAIN; }
    inline QString getAppDescription() { return APP_DESCRIPTION; }

} // namespace AppInfo
} // namespace WhisperApp

#endif // APPINFO_H
