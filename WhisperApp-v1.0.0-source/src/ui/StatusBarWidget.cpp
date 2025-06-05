#include "StatusBarWidget.h"
#include "../core/Logger.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QFrame>
#include <QPixmap>
#include <QStyle>
#include <QApplication>

StatusBarWidget::StatusBarWidget(QWidget* parent)
    : QWidget(parent)
    , m_isRecording(false)
    , m_recordingDuration(0)
    , m_lastDownloadBytes(0)
    , m_downloadSpeed(0)
{
    setupUI();
    connectSignals();
}

StatusBarWidget::~StatusBarWidget()
{
}

void StatusBarWidget::setupUI()
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 2, 5, 2);
    mainLayout->setSpacing(10);
    
    // Recording status section
    m_recordingIcon = new QLabel(this);
    m_recordingIcon->setFixedSize(16, 16);
    m_recordingLabel = new QLabel(tr("Ready"), this);
    
    mainLayout->addWidget(m_recordingIcon);
    mainLayout->addWidget(m_recordingLabel);
    
    // Separator
    QFrame* separator1 = new QFrame(this);
    separator1->setFrameStyle(QFrame::VLine | QFrame::Sunken);
    mainLayout->addWidget(separator1);
    
    // Model status section
    m_modelIcon = new QLabel(this);
    m_modelIcon->setFixedSize(16, 16);
    m_modelButton = new QPushButton(tr("No model"), this);
    m_modelButton->setFlat(true);
    m_modelButton->setCursor(Qt::PointingHandCursor);
    m_modelButton->setStyleSheet("QPushButton { text-align: left; padding: 2px; }"
                                "QPushButton:hover { text-decoration: underline; }");
    
    mainLayout->addWidget(m_modelIcon);
    mainLayout->addWidget(m_modelButton);
    
    // Separator
    QFrame* separator2 = new QFrame(this);
    separator2->setFrameStyle(QFrame::VLine | QFrame::Sunken);
    mainLayout->addWidget(separator2);
    
    // Device status section
    m_deviceIcon = new QLabel(this);
    m_deviceIcon->setFixedSize(16, 16);
    m_deviceButton = new QPushButton(tr("No device"), this);
    m_deviceButton->setFlat(true);
    m_deviceButton->setCursor(Qt::PointingHandCursor);
    m_deviceButton->setStyleSheet("QPushButton { text-align: left; padding: 2px; }"
                                 "QPushButton:hover { text-decoration: underline; }");
    
    mainLayout->addWidget(m_deviceIcon);
    mainLayout->addWidget(m_deviceButton);
    
    // Separator
    QFrame* separator3 = new QFrame(this);
    separator3->setFrameStyle(QFrame::VLine | QFrame::Sunken);
    mainLayout->addWidget(separator3);
    
    // Network status section
    m_networkIcon = new QLabel(this);
    m_networkIcon->setFixedSize(16, 16);
    m_networkButton = new QPushButton(tr("Offline"), this);
    m_networkButton->setFlat(true);
    m_networkButton->setCursor(Qt::PointingHandCursor);
    m_networkButton->setStyleSheet("QPushButton { text-align: left; padding: 2px; }"
                                  "QPushButton:hover { text-decoration: underline; }");
    
    mainLayout->addWidget(m_networkIcon);
    mainLayout->addWidget(m_networkButton);
    
    // Progress section (hidden by default)
    m_progressWidget = new QWidget(this);
    QHBoxLayout* progressLayout = new QHBoxLayout(m_progressWidget);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    
    QFrame* separator4 = new QFrame(m_progressWidget);
    separator4->setFrameStyle(QFrame::VLine | QFrame::Sunken);
    progressLayout->addWidget(separator4);
    
    m_progressLabel = new QLabel(m_progressWidget);
    m_progressBar = new QProgressBar(m_progressWidget);
    m_progressBar->setMaximumWidth(100);
    m_progressBar->setTextVisible(false);
    
    progressLayout->addWidget(m_progressLabel);
    progressLayout->addWidget(m_progressBar);
    
    m_progressWidget->setVisible(false);
    mainLayout->addWidget(m_progressWidget);
    
    // Stretch to push everything to the left
    mainLayout->addStretch();
    
    // Audio level indicator
    m_audioLevelBar = new QProgressBar(this);
    m_audioLevelBar->setMaximumWidth(100);
    m_audioLevelBar->setMaximumHeight(10);
    m_audioLevelBar->setTextVisible(false);
    m_audioLevelBar->setRange(0, 100);
    m_audioLevelBar->setValue(0);
    m_audioLevelBar->setStyleSheet("QProgressBar { border: 1px solid gray; border-radius: 2px; }"
                                  "QProgressBar::chunk { background-color: #00aa00; }");
    
    mainLayout->addWidget(new QLabel(tr("Audio:"), this));
    mainLayout->addWidget(m_audioLevelBar);
    
    // Message label (for temporary messages)
    m_messageLabel = new QLabel(this);
    m_messageLabel->setVisible(false);
    m_messageLabel->setStyleSheet("QLabel { color: #0066cc; font-weight: bold; }");
    mainLayout->addWidget(m_messageLabel);
    
    // Initialize icons
    updateIcons();
    
    // Create timers
    m_recordingTimer = new QTimer(this);
    m_recordingTimer->setInterval(1000);
    
    m_messageTimer = new QTimer(this);
    m_messageTimer->setSingleShot(true);
    
    m_networkTimer = new QTimer(this);
    m_networkTimer->setInterval(1000);
}

void StatusBarWidget::connectSignals()
{
    connect(m_modelButton, &QPushButton::clicked, this, &StatusBarWidget::onModelClicked);
    connect(m_deviceButton, &QPushButton::clicked, this, &StatusBarWidget::onDeviceClicked);
    connect(m_networkButton, &QPushButton::clicked, this, &StatusBarWidget::onNetworkClicked);
    
    connect(m_recordingTimer, &QTimer::timeout, this, &StatusBarWidget::updateRecordingTime);
    connect(m_messageTimer, &QTimer::timeout, this, &StatusBarWidget::clearMessage);
    connect(m_networkTimer, &QTimer::timeout, this, &StatusBarWidget::updateNetworkSpeed);
}

void StatusBarWidget::setRecordingStatus(bool isRecording, int duration)
{
    m_isRecording = isRecording;
    m_recordingDuration = duration;
    
    if (isRecording) {
        m_recordingIcon->setPixmap(style()->standardPixmap(QStyle::SP_MediaPlay).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_recordingLabel->setText(tr("Recording: %1").arg(formatDuration(duration)));
        m_recordingLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        m_recordingTimer->start();
    } else {
        m_recordingIcon->setPixmap(style()->standardPixmap(QStyle::SP_MediaStop).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_recordingLabel->setText(tr("Ready"));
        m_recordingLabel->setStyleSheet("");
        m_recordingTimer->stop();
    }
}

void StatusBarWidget::setModelStatus(const QString& modelName, bool isLoaded)
{
    m_modelButton->setText(modelName.isEmpty() ? tr("No model") : modelName);
    
    if (isLoaded) {
        m_modelIcon->setPixmap(style()->standardPixmap(QStyle::SP_DialogYesButton).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_modelButton->setStyleSheet("QPushButton { text-align: left; padding: 2px; color: green; }"
                                    "QPushButton:hover { text-decoration: underline; }");
    } else {
        m_modelIcon->setPixmap(style()->standardPixmap(QStyle::SP_DialogNoButton).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_modelButton->setStyleSheet("QPushButton { text-align: left; padding: 2px; color: gray; }"
                                    "QPushButton:hover { text-decoration: underline; }");
    }
}

void StatusBarWidget::setDeviceStatus(const QString& deviceName, bool isConnected)
{
    m_deviceButton->setText(deviceName.isEmpty() ? tr("No device") : deviceName);
    
    if (isConnected) {
        m_deviceIcon->setPixmap(style()->standardPixmap(QStyle::SP_ComputerIcon).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_deviceButton->setStyleSheet("QPushButton { text-align: left; padding: 2px; color: green; }"
                                     "QPushButton:hover { text-decoration: underline; }");
    } else {
        m_deviceIcon->setPixmap(style()->standardPixmap(QStyle::SP_MessageBoxWarning).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_deviceButton->setStyleSheet("QPushButton { text-align: left; padding: 2px; color: red; }"
                                     "QPushButton:hover { text-decoration: underline; }");
    }
}

void StatusBarWidget::setNetworkStatus(bool isOnline, qint64 downloadSpeed)
{
    m_downloadSpeed = downloadSpeed;
    
    if (isOnline) {
        m_networkIcon->setPixmap(style()->standardPixmap(QStyle::SP_DriveNetIcon).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        
        if (downloadSpeed > 0) {
            m_networkButton->setText(tr("Online (%1/s)").arg(formatSize(downloadSpeed)));
            m_networkTimer->start();
        } else {
            m_networkButton->setText(tr("Online"));
            m_networkTimer->stop();
        }
        
        m_networkButton->setStyleSheet("QPushButton { text-align: left; padding: 2px; color: green; }"
                                      "QPushButton:hover { text-decoration: underline; }");
    } else {
        m_networkIcon->setPixmap(style()->standardPixmap(QStyle::SP_MessageBoxCritical).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_networkButton->setText(tr("Offline"));
        m_networkButton->setStyleSheet("QPushButton { text-align: left; padding: 2px; color: red; }"
                                      "QPushButton:hover { text-decoration: underline; }");
        m_networkTimer->stop();
    }
}

void StatusBarWidget::showProgress(const QString& text, int value, int maximum)
{
    m_progressLabel->setText(text);
    m_progressBar->setMaximum(maximum);
    m_progressBar->setValue(value);
    m_progressWidget->setVisible(true);
}

void StatusBarWidget::hideProgress()
{
    m_progressWidget->setVisible(false);
}

void StatusBarWidget::showMessage(const QString& message, int timeout)
{
    m_messageLabel->setText(message);
    m_messageLabel->setVisible(true);
    
    if (timeout > 0) {
        m_messageTimer->start(timeout);
    }
}

void StatusBarWidget::setAudioLevel(float level)
{
    int value = qBound(0, static_cast<int>(level * 100), 100);
    m_audioLevelBar->setValue(value);
    
    // Change color based on level
    if (value > 80) {
        m_audioLevelBar->setStyleSheet("QProgressBar { border: 1px solid gray; border-radius: 2px; }"
                                      "QProgressBar::chunk { background-color: #ff0000; }");
    } else if (value > 60) {
        m_audioLevelBar->setStyleSheet("QProgressBar { border: 1px solid gray; border-radius: 2px; }"
                                      "QProgressBar::chunk { background-color: #ffaa00; }");
    } else {
        m_audioLevelBar->setStyleSheet("QProgressBar { border: 1px solid gray; border-radius: 2px; }"
                                      "QProgressBar::chunk { background-color: #00aa00; }");
    }
}

void StatusBarWidget::updateRecordingTime()
{
    if (m_isRecording) {
        m_recordingDuration++;
        m_recordingLabel->setText(tr("Recording: %1").arg(formatDuration(m_recordingDuration)));
    }
}

void StatusBarWidget::updateNetworkSpeed()
{
    // This would normally calculate actual network speed
    // For now, just update the display
    if (m_downloadSpeed > 0) {
        m_networkButton->setText(tr("Online (%1/s)").arg(formatSize(m_downloadSpeed)));
    }
}

void StatusBarWidget::clearMessage()
{
    m_messageLabel->setVisible(false);
    m_messageLabel->clear();
}

void StatusBarWidget::onModelClicked()
{
    emit modelInfoClicked();
}

void StatusBarWidget::onDeviceClicked()
{
    emit deviceInfoClicked();
}

void StatusBarWidget::onNetworkClicked()
{
    emit networkInfoClicked();
}

QString StatusBarWidget::formatDuration(int seconds) const
{
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
}

QString StatusBarWidget::formatSize(qint64 bytes) const
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    
    if (bytes >= GB) {
        return QString::number(bytes / double(GB), 'f', 2) + " GB";
    } else if (bytes >= MB) {
        return QString::number(bytes / double(MB), 'f', 1) + " MB";
    } else if (bytes >= KB) {
        return QString::number(bytes / double(KB), 'f', 1) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
}

void StatusBarWidget::updateIcons()
{
    // Set initial icons
    m_recordingIcon->setPixmap(style()->standardPixmap(QStyle::SP_MediaStop).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_modelIcon->setPixmap(style()->standardPixmap(QStyle::SP_DialogNoButton).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_deviceIcon->setPixmap(style()->standardPixmap(QStyle::SP_MessageBoxWarning).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_networkIcon->setPixmap(style()->standardPixmap(QStyle::SP_MessageBoxCritical).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
