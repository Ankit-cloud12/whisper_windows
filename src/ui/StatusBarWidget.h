#pragma once

#include <QWidget>

// Forward declarations
class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;

/**
 * @brief Custom status bar widget with multiple sections
 * 
 * Provides a rich status bar with recording status, model info,
 * device info, and network status indicators.
 */
class StatusBarWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit StatusBarWidget(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~StatusBarWidget();
    
    /**
     * @brief Set recording status
     * @param isRecording Recording state
     * @param duration Duration in seconds
     */
    void setRecordingStatus(bool isRecording, int duration = 0);
    
    /**
     * @brief Set model status
     * @param modelName Model name
     * @param isLoaded Load state
     */
    void setModelStatus(const QString& modelName, bool isLoaded);
    
    /**
     * @brief Set device status
     * @param deviceName Device name
     * @param isConnected Connection state
     */
    void setDeviceStatus(const QString& deviceName, bool isConnected);
    
    /**
     * @brief Set network status
     * @param isOnline Online state
     * @param downloadSpeed Download speed in bytes/sec
     */
    void setNetworkStatus(bool isOnline, qint64 downloadSpeed = 0);
    
    /**
     * @brief Show progress
     * @param text Progress text
     * @param value Progress value (0-100)
     * @param maximum Maximum value
     */
    void showProgress(const QString& text, int value, int maximum = 100);
    
    /**
     * @brief Hide progress
     */
    void hideProgress();
    
    /**
     * @brief Show temporary message
     * @param message Message text
     * @param timeout Timeout in milliseconds
     */
    void showMessage(const QString& message, int timeout = 3000);
    
    /**
     * @brief Set audio level
     * @param level Audio level (0.0 to 1.0)
     */
    void setAudioLevel(float level);

signals:
    /**
     * @brief Emitted when model info is clicked
     */
    void modelInfoClicked();
    
    /**
     * @brief Emitted when device info is clicked
     */
    void deviceInfoClicked();
    
    /**
     * @brief Emitted when network info is clicked
     */
    void networkInfoClicked();

private slots:
    void updateRecordingTime();
    void updateNetworkSpeed();
    void clearMessage();
    void onModelClicked();
    void onDeviceClicked();
    void onNetworkClicked();

private:
    void setupUI();
    void connectSignals();
    QString formatDuration(int seconds) const;
    QString formatSize(qint64 bytes) const;

private:
    // Status sections
    QLabel* m_recordingIcon;
    QLabel* m_recordingLabel;
    QLabel* m_modelIcon;
    QPushButton* m_modelButton;
    QLabel* m_deviceIcon;
    QPushButton* m_deviceButton;
    QLabel* m_networkIcon;
    QPushButton* m_networkButton;
    
    // Progress section
    QWidget* m_progressWidget;
    QLabel* m_progressLabel;
    QProgressBar* m_progressBar;
    
    // Message section
    QLabel* m_messageLabel;
    
    // Audio level
    QProgressBar* m_audioLevelBar;
    
    // Timers
    QTimer* m_recordingTimer;
    QTimer* m_messageTimer;
    QTimer* m_networkTimer;
    
    // State
    bool m_isRecording;
    int m_recordingDuration;
    qint64 m_lastDownloadBytes;
    qint64 m_downloadSpeed;
};