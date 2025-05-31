/*
 * TrayIcon.h
 * 
 * System tray icon functionality for Windows.
 * Provides quick access to application features from the system tray.
 * 
 * Features:
 * - System tray icon with context menu
 * - Balloon notifications
 * - Quick recording controls
 * - Status indication
 * - Minimize to tray support
 */

#ifndef TRAYICON_H
#define TRAYICON_H

#include <QObject>
#include <memory>
#include <string>

// Forward declarations
class QSystemTrayIcon;
class QMenu;
class QAction;
class QIcon;
class MainWindow;

/**
 * @brief System tray icon manager
 */
class TrayIcon : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Tray icon status
     */
    enum class Status {
        Idle,           // Not recording
        Recording,      // Currently recording
        Processing,     // Processing audio
        Error,          // Error state
        Disabled        // Functionality disabled
    };

    /**
     * @brief Notification type
     */
    enum class NotificationType {
        Info,           // Information message
        Success,        // Success message
        Warning,        // Warning message
        Error           // Error message
    };

public:
    explicit TrayIcon(MainWindow* main_window, QObject* parent = nullptr);
    ~TrayIcon();

    /**
     * @brief Show or hide the tray icon
     * @param visible true to show, false to hide
     */
    void setVisible(bool visible);

    /**
     * @brief Check if tray icon is visible
     * @return true if visible
     */
    bool isVisible() const;

    /**
     * @brief Set tray icon status
     * @param status New status
     */
    void setStatus(Status status);

    /**
     * @brief Get current status
     * @return Current status
     */
    Status getStatus() const;

    /**
     * @brief Show a notification balloon
     * @param title Notification title
     * @param message Notification message
     * @param type Notification type
     * @param duration_ms Duration in milliseconds
     */
    void showNotification(const QString& title,
                         const QString& message,
                         NotificationType type = NotificationType::Info,
                         int duration_ms = 5000);

    /**
     * @brief Set tooltip text
     * @param tooltip Tooltip text
     */
    void setTooltip(const QString& tooltip);

    /**
     * @brief Update recording time in tooltip
     * @param seconds Recording duration in seconds
     */
    void updateRecordingTime(int seconds);

    /**
     * @brief Set recording state
     * @param is_recording true if recording
     */
    void setRecordingState(bool is_recording);

    /**
     * @brief Enable or disable tray icon
     * @param enabled true to enable
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if system tray is available
     * @return true if available
     */
    static bool isSystemTrayAvailable();

    /**
     * @brief Check if system supports notifications
     * @return true if supported
     */
    static bool supportsNotifications();

signals:
    /**
     * @brief Emitted when tray icon is activated
     * @param reason Activation reason
     */
    void activated(QSystemTrayIcon::ActivationReason reason);

    /**
     * @brief Emitted when show window is requested
     */
    void showWindowRequested();

    /**
     * @brief Emitted when hide window is requested
     */
    void hideWindowRequested();

    /**
     * @brief Emitted when recording start is requested
     */
    void startRecordingRequested();

    /**
     * @brief Emitted when recording stop is requested
     */
    void stopRecordingRequested();

    /**
     * @brief Emitted when settings are requested
     */
    void settingsRequested();

    /**
     * @brief Emitted when exit is requested
     */
    void exitRequested();

private slots:
    /**
     * @brief Handle tray icon activation
     * @param reason Activation reason
     */
    void onActivated(QSystemTrayIcon::ActivationReason reason);

    /**
     * @brief Handle notification clicked
     */
    void onNotificationClicked();

    // Menu action slots
    void onShowHide();
    void onStartStopRecording();
    void onOpenSettings();
    void onAbout();
    void onExit();

    /**
     * @brief Update tray icon based on status
     */
    void updateIcon();

    /**
     * @brief Update menu items based on state
     */
    void updateMenu();

private:
    /**
     * @brief Create context menu
     */
    void createMenu();

    /**
     * @brief Create actions
     */
    void createActions();

    /**
     * @brief Get icon for status
     * @param status Status to get icon for
     * @return Icon for status
     */
    QIcon getIconForStatus(Status status) const;

    /**
     * @brief Get notification icon
     * @param type Notification type
     * @return Icon for notification
     */
    QIcon getNotificationIcon(NotificationType type) const;

    /**
     * @brief Format time for display
     * @param seconds Time in seconds
     * @return Formatted time string
     */
    QString formatTime(int seconds) const;

private:
    // Core components
    QSystemTrayIcon* tray_icon = nullptr;
    QMenu* context_menu = nullptr;
    MainWindow* main_window = nullptr;

    // Actions
    QAction* show_hide_action = nullptr;
    QAction* start_stop_action = nullptr;
    QAction* settings_action = nullptr;
    QAction* about_action = nullptr;
    QAction* exit_action = nullptr;

    // State
    Status current_status = Status::Idle;
    bool is_recording = false;
    bool window_visible = true;
    int recording_time_seconds = 0;

    // Icons (will be loaded from resources)
    struct Icons {
        QIcon idle;
        QIcon recording;
        QIcon processing;
        QIcon error;
        QIcon disabled;
    } icons;

    // Constants
    static constexpr const char* DEFAULT_TOOLTIP = "WhisperApp - Speech to Text";
    static constexpr int DOUBLE_CLICK_INTERVAL = 500; // milliseconds
};

#endif // TRAYICON_H