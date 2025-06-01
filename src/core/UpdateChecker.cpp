#include "UpdateChecker.h"
#include "Logger.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>

UpdateChecker::UpdateChecker(QObject* parent) 
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_checkTimer(new QTimer(this))
    , m_checkInterval(24 * 60 * 60 * 1000) // 24 hours
    , m_autoCheck(true)
    , m_updateAvailable(false) {
    
    connect(m_checkTimer, &QTimer::timeout, this, &UpdateChecker::checkForUpdates);
    
    // Start auto-check if enabled
    if (m_autoCheck) {
        m_checkTimer->start(m_checkInterval);
        // Check immediately on startup
        QTimer::singleShot(5000, this, &UpdateChecker::checkForUpdates);
    }
}

UpdateChecker::~UpdateChecker() = default;

void UpdateChecker::checkForUpdates() {
    LOG_INFO("UpdateChecker", "Checking for updates...");
    
    QNetworkRequest request(QUrl(UPDATE_CHECK_URL));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "WhisperApp/1.0.0");
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleUpdateCheckResponse(reply);
    });
}

void UpdateChecker::handleUpdateCheckResponse(QNetworkReply* reply) {
    reply->deleteLater();
    
    if (reply->error() != QNetworkReply::NoError) {
        LOG_ERROR("UpdateChecker", "Failed to check for updates: " + reply->errorString().toStdString());
        emit updateCheckFailed(reply->errorString());
        return;
    }
    
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (!doc.isObject()) {
        LOG_ERROR("UpdateChecker", "Invalid update response format");
        emit updateCheckFailed("Invalid response format");
        return;
    }
    
    QJsonObject root = doc.object();
    
    // Parse GitHub release API response
    QString tagName = root["tag_name"].toString();
    QString version = tagName.startsWith("v") ? tagName.mid(1) : tagName;
    QString downloadUrl = "";
    QString releaseNotes = root["body"].toString();
    
    // Find Windows installer asset
    QJsonArray assets = root["assets"].toArray();
    for (const auto& asset : assets) {
        QJsonObject assetObj = asset.toObject();
        QString name = assetObj["name"].toString();
        if (name.endsWith(".exe") || name.contains("windows", Qt::CaseInsensitive)) {
            downloadUrl = assetObj["browser_download_url"].toString();
            break;
        }
    }
    
    // Compare versions
    if (isNewerVersion(version)) {
        m_updateAvailable = true;
        m_latestVersion = version;
        m_downloadUrl = downloadUrl;
        m_releaseNotes = releaseNotes;
        
        LOG_INFO("UpdateChecker", "Update available: " + version.toStdString());
        emit updateAvailable(version, downloadUrl, releaseNotes);
    } else {
        m_updateAvailable = false;
        LOG_INFO("UpdateChecker", "No updates available");
        emit noUpdateAvailable();
    }
}

bool UpdateChecker::isNewerVersion(const QString& remoteVersion) const {
    // Simple version comparison
    QStringList currentParts = QString(APP_VERSION).split('.');
    QStringList remoteParts = remoteVersion.split('.');
    
    for (int i = 0; i < qMin(currentParts.size(), remoteParts.size()); ++i) {
        int current = currentParts[i].toInt();
        int remote = remoteParts[i].toInt();
        
        if (remote > current) {
            return true;
        } else if (remote < current) {
            return false;
        }
    }
    
    // If all compared parts are equal, check if remote has more parts
    return remoteParts.size() > currentParts.size();
}

void UpdateChecker::setAutoCheck(bool enabled) {
    m_autoCheck = enabled;
    
    if (enabled) {
        m_checkTimer->start(m_checkInterval);
    } else {
        m_checkTimer->stop();
    }
}

void UpdateChecker::setCheckInterval(int hours) {
    m_checkInterval = hours * 60 * 60 * 1000;
    
    if (m_checkTimer->isActive()) {
        m_checkTimer->setInterval(m_checkInterval);
    }
}

bool UpdateChecker::isUpdateAvailable() const {
    return m_updateAvailable;
}

QString UpdateChecker::getLatestVersion() const {
    return m_latestVersion;
}

QString UpdateChecker::getDownloadUrl() const {
    return m_downloadUrl;
}

QString UpdateChecker::getReleaseNotes() const {
    return m_releaseNotes;
}

void UpdateChecker::downloadUpdate() {
    if (!m_updateAvailable || m_downloadUrl.isEmpty()) {
        LOG_WARN("UpdateChecker", "No update available to download");
        return;
    }
    
    LOG_INFO("UpdateChecker", "Opening download URL: " + m_downloadUrl.toStdString());
    QDesktopServices::openUrl(QUrl(m_downloadUrl));
}

void UpdateChecker::ignoreUpdate(const QString& version) {
    m_ignoredVersions.insert(version);
    
    if (version == m_latestVersion) {
        m_updateAvailable = false;
    }
}

bool UpdateChecker::isVersionIgnored(const QString& version) const {
    return m_ignoredVersions.contains(version);
}