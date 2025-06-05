#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <memory>

namespace WhisperApp {

struct UpdateInfo {
    QString version;
    QString releaseDate;
    QString downloadUrl;
    QString releaseNotes;
    qint64 fileSize;
    QString checksum;
    bool isMandatory;
    
    bool isNewerThan(const QString& currentVersion) const;
};

class UpdateChecker : public QObject {
    Q_OBJECT
    
public:
    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker();
    
    // Configuration
    void setUpdateUrl(const QString& url);
    void setCurrentVersion(const QString& version);
    void setCheckInterval(int hours);
    
    // Check for updates
    void checkForUpdates(bool silent = false);
    void downloadUpdate(const UpdateInfo& info);
    
    // Auto-update settings
    void setAutoCheckEnabled(bool enabled);
    bool isAutoCheckEnabled() const;
    
    // Get last check time
    QDateTime lastCheckTime() const;
    
signals:
    void updateAvailable(const UpdateInfo& info);
    void noUpdateAvailable();
    void checkFailed(const QString& error);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadComplete(const QString& filePath);
    void downloadFailed(const QString& error);
    
private slots:
    void onCheckReplyFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    
private:
    void parseUpdateManifest(const QByteArray& data);
    bool verifyChecksum(const QString& filePath, const QString& expectedChecksum);
    void scheduleNextCheck();
    
    QString m_updateUrl;
    QString m_currentVersion;
    int m_checkInterval;
    bool m_autoCheckEnabled;
    QDateTime m_lastCheckTime;
    
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    QNetworkReply* m_currentReply;
    
    UpdateInfo m_latestUpdate;
    QString m_downloadPath;
};

} // namespace WhisperApp

#endif // UPDATECHECKER_H