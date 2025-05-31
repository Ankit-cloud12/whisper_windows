#include "UpdateChecker.h"
#include "Logger.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QTimer>
#include <QVersionNumber>

namespace WhisperApp {

bool UpdateInfo::isNewerThan(const QString& currentVersion) const {
    QVersionNumber current = QVersionNumber::fromString(currentVersion);
    QVersionNumber update = QVersionNumber::fromString(version);
    return update > current;
}

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_checkInterval(24) // Default: 24 hours
    , m_autoCheckEnabled(true)
    , m_networkManager(std::make_unique<QNetworkAccessManager>())
    , m_currentReply(nullptr) {
    
    // Default update URL (should be configured)
    m_updateUrl = "https://api.github.com/repos/yourname/whisperapp/releases/latest";
}

UpdateChecker::~UpdateChecker() {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
}

void UpdateChecker::setUpdateUrl(const QString& url) {
    m_updateUrl = url;
}

void UpdateChecker::setCurrentVersion(const QString& version) {
    m_currentVersion = version;
}

void UpdateChecker::setCheckInterval(int hours) {
    m_checkInterval = hours;
}

void UpdateChecker::checkForUpdates(bool silent) {
    if (m_updateUrl.isEmpty()) {
        Logger::error("Update URL not configured");
        if (!silent) {
            emit checkFailed(tr("Update URL not configured"));
        }
        return;
    }
    
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
    
    Logger::info("Checking for updates...");
    
    QNetworkRequest request(QUrl(m_updateUrl));
    request.setHeader(QNetworkRequest::UserAgentHeader, 
                     QString("WhisperApp/%1").arg(m_currentVersion));
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                        QNetworkRequest::AlwaysNetwork);
    
    m_currentReply = m_networkManager->get(request);
    m_currentReply->setProperty("silent", silent);
    
    connect(m_currentReply, &QNetworkReply::finished,
            this, &UpdateChecker::onCheckReplyFinished);
    
    m_lastCheckTime = QDateTime::currentDateTime();
}

void UpdateChecker::onCheckReplyFinished() {
    if (!m_currentReply) return;
    
    bool silent = m_currentReply->property("silent").toBool();
    
    if (m_currentReply->error() != QNetworkReply::NoError) {
        QString error = m_currentReply->errorString();
        Logger::error("Update check failed: " + error);
        if (!silent) {
            emit checkFailed(error);
        }
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        scheduleNextCheck();
        return;
    }
    
    QByteArray data = m_currentReply->readAll();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    
    parseUpdateManifest(data);
    scheduleNextCheck();
}

void UpdateChecker::parseUpdateManifest(const QByteArray& data) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        Logger::error("Failed to parse update manifest: " + error.errorString());
        emit checkFailed(tr("Invalid update data"));
        return;
    }
    
    QJsonObject obj = doc.object();
    
    // Parse GitHub release format
    m_latestUpdate.version = obj["tag_name"].toString().remove('v');
    m_latestUpdate.releaseDate = obj["published_at"].toString();
    m_latestUpdate.releaseNotes = obj["body"].toString();
    
    // Find Windows installer asset
    QJsonArray assets = obj["assets"].toArray();
    for (const QJsonValue& assetVal : assets) {
        QJsonObject asset = assetVal.toObject();
        QString name = asset["name"].toString();
        
        if (name.endsWith("-Setup.exe") || name.endsWith("-installer.exe")) {
            m_latestUpdate.downloadUrl = asset["browser_download_url"].toString();
            m_latestUpdate.fileSize = asset["size"].toDouble();
            break;
        }
    }
    
    if (m_latestUpdate.downloadUrl.isEmpty()) {
        Logger::warn("No installer found in release");
        emit noUpdateAvailable();
        return;
    }
    
    // Check if update is newer
    if (m_latestUpdate.isNewerThan(m_currentVersion)) {
        Logger::info(QString("Update available: %1 -> %2")
                    .arg(m_currentVersion, m_latestUpdate.version));
        emit updateAvailable(m_latestUpdate);
    } else {
        Logger::info("No update available");
        emit noUpdateAvailable();
    }
}

void UpdateChecker::downloadUpdate(const UpdateInfo& info) {
    if (info.downloadUrl.isEmpty()) {
        emit downloadFailed(tr("Invalid download URL"));
        return;
    }
    
    QString downloadDir = QStandardPaths::writableLocation(
        QStandardPaths::DownloadLocation);
    m_downloadPath = QString("%1/WhisperApp-%2-Setup.exe")
                     .arg(downloadDir, info.version);
    
    QFile file(m_downloadPath);
    if (file.exists()) {
        // Verify existing file
        if (!info.checksum.isEmpty() && verifyChecksum(m_downloadPath, info.checksum)) {
            Logger::info("Update already downloaded");
            emit downloadComplete(m_downloadPath);
            return;
        }
        file.remove();
    }
    
    Logger::info("Downloading update: " + info.downloadUrl);
    
    QNetworkRequest request(QUrl(info.downloadUrl));
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    
    m_currentReply = m_networkManager->get(request);
    m_currentReply->setProperty("checksum", info.checksum);
    
    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &UpdateChecker::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &UpdateChecker::onDownloadFinished);
}

void UpdateChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit downloadProgress(bytesReceived, bytesTotal);
}

void UpdateChecker::onDownloadFinished() {
    if (!m_currentReply) return;
    
    if (m_currentReply->error() != QNetworkReply::NoError) {
        QString error = m_currentReply->errorString();
        Logger::error("Download failed: " + error);
        emit downloadFailed(error);
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    QByteArray data = m_currentReply->readAll();
    QString expectedChecksum = m_currentReply->property("checksum").toString();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    
    // Save file
    QFile file(m_downloadPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit downloadFailed(tr("Failed to save update file"));
        return;
    }
    
    file.write(data);
    file.close();
    
    // Verify checksum if provided
    if (!expectedChecksum.isEmpty()) {
        if (!verifyChecksum(m_downloadPath, expectedChecksum)) {
            file.remove();
            emit downloadFailed(tr("Update file verification failed"));
            return;
        }
    }
    
    Logger::info("Update downloaded successfully: " + m_downloadPath);
    emit downloadComplete(m_downloadPath);
}

bool UpdateChecker::verifyChecksum(const QString& filePath, const QString& expectedChecksum) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        hash.addData(file.read(8192));
    }
    
    QString actualChecksum = hash.result().toHex();
    return actualChecksum.compare(expectedChecksum, Qt::CaseInsensitive) == 0;
}

void UpdateChecker::setAutoCheckEnabled(bool enabled) {
    m_autoCheckEnabled = enabled;
    if (enabled) {
        scheduleNextCheck();
    }
}

bool UpdateChecker::isAutoCheckEnabled() const {
    return m_autoCheckEnabled;
}

QDateTime UpdateChecker::lastCheckTime() const {
    return m_lastCheckTime;
}

void UpdateChecker::scheduleNextCheck() {
    if (!m_autoCheckEnabled) return;
    
    QTimer::singleShot(m_checkInterval * 3600 * 1000, this, [this]() {
        checkForUpdates(true); // Silent check
    });
}

} // namespace WhisperApp