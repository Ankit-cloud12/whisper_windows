#include "TrayIcon.h"
#include "../core/Settings.h"
#include "../core/Logger.h"
#include "UIUtils.h"
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QStyle>
#include <QPixmap>
#include <QPainter>

// TODO: Replace placeholder/system icons for Processing, Error, and Disabled states with custom-designed application icons.

TrayIcon::TrayIcon(MainWindow* main_window, QObject* parent)
    : QObject(parent)
    , main_window(main_window) // Store main_window
    , m_trayIcon(nullptr)
    , m_contextMenu(nullptr)
    , m_recordAction(nullptr)
    // , m_isRecording(false) // Replaced by current_status
    , m_flashTimer(nullptr)
    , m_flashState(false)
    , current_status(Status::Idle) // Initialize current_status
{
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        // Initialize icons
        icons.idle = createNormalIcon();
        icons.recording = createRecordingIcon(false); // Non-flashing base
        icons.processing = createProcessingIcon();
        icons.error = createErrorIcon();
        icons.disabled = createDisabledIcon();

        createTrayIcon(); // Sets initial icon to icons.idle
        createContextMenu();
        connectSignals();
        
        setStatus(Status::Idle); // Set initial state and update icon/tooltip

        Logger::instance().log(Logger::LogLevel::Info, "TrayIcon", "System tray icon initialized");
    } else {
        Logger::instance().log(Logger::LogLevel::Warning, "TrayIcon", "System tray not available");
    }
}

TrayIcon::~TrayIcon()
{
    if (m_flashTimer) {
        m_flashTimer->stop();
        delete m_flashTimer;
    }
    
    if (m_trayIcon) {
        m_trayIcon->hide();
        delete m_trayIcon;
    }
    
    if (m_contextMenu) {
        delete m_contextMenu;
    }
}

void TrayIcon::show()
{
    if (m_trayIcon) {
        m_trayIcon->show();
        
        Settings& settings = Settings::instance();
        if (settings.getSetting(Settings::Key::ShowTrayNotifications).toBool()) {
            showMessage(tr("WhisperApp"), 
                       tr("Application is running in the system tray"), 
                       Information, 3000);
        }
        
        Logger::instance().log(Logger::LogLevel::Debug, "TrayIcon", "Tray icon shown");
    }
}

void TrayIcon::hide()
{
    if (m_trayIcon) {
        m_trayIcon->hide();
        Logger::instance().log(Logger::LogLevel::Debug, "TrayIcon", "Tray icon hidden");
    }
}

void TrayIcon::setIcon(const QIcon& icon)
{
    if (m_trayIcon) {
        m_trayIcon->setIcon(icon);
    }
}

void TrayIcon::showMessage(const QString& title, const QString& message, MessageIcon icon, int millisecondsTimeoutHint)
{
    if (!m_trayIcon || !m_trayIcon->isVisible()) {
        return;
    }
    
    Settings& settings = Settings::instance();
    if (!settings.getSetting(Settings::Key::ShowTrayNotifications).toBool()) {
        return;
    }
    
    QSystemTrayIcon::MessageIcon msgIcon = QSystemTrayIcon::Information;
    
    switch (icon) {
        case Information:
            msgIcon = QSystemTrayIcon::Information;
            break;
        case Warning:
            msgIcon = QSystemTrayIcon::Warning;
            break;
        case Critical:
            msgIcon = QSystemTrayIcon::Critical;
            break;
    }
    
    m_trayIcon->showMessage(title, message, msgIcon, millisecondsTimeoutHint);
    
    Logger::instance().log(Logger::LogLevel::Debug, "TrayIcon", 
                          QString("Showing message: %1 - %2").arg(title, message).toStdString());
}

void TrayIcon::setToolTip(const QString& tip)
{
    if (m_trayIcon) {
        m_trayIcon->setToolTip(tip);
    }
}

void TrayIcon::updateRecordingState(bool recording)
{
    // m_isRecording = recording; // Replaced by current_status
    setStatus(recording ? Status::Recording : Status::Idle);
    // updateContextMenu(recording); // updateContextMenu is now called within setStatus->updateIcon
}

// New method setStatus
void TrayIcon::setStatus(Status status) {
    if (current_status == status) {
        return;
    }
    current_status = status;
    updateIcon(); // Centralized icon and tooltip update
    updateMenu(); // Update context menu based on new state (was updateContextMenu)
}

Status TrayIcon::getStatus() const {
    return current_status;
}

void TrayIcon::updateIcon() {
    if (!m_trayIcon) return;

    stopFlashing(); // Stop flashing by default, will be started if needed

    switch (current_status) {
        case Status::Idle:
            setIcon(icons.idle);
            setToolTip(tr("WhisperApp - Ready"));
            Logger::instance().log(Logger::LogLevel::Debug, "TrayIcon", "Status changed to Idle");
            break;
        case Status::Recording:
            // The onFlashTimer will use createRecordingIcon(bool) for flashing effect.
            // Set the base non-flashing icon here.
            setIcon(icons.recording);
            setToolTip(tr("WhisperApp - Recording..."));
            startFlashing();
            Logger::instance().log(Logger::LogLevel::Debug, "TrayIcon", "Status changed to Recording");
            break;
        case Status::Processing:
            setIcon(icons.processing);
            setToolTip(tr("WhisperApp - Processing audio..."));
            Logger::instance().log(Logger::LogLevel::Debug, "TrayIcon", "Status changed to Processing");
            break;
        case Status::Error:
            setIcon(icons.error);
            setToolTip(tr("WhisperApp - Error occurred. Check application."));
            Logger::instance().log(Logger::LogLevel::Debug, "TrayIcon", "Status changed to Error");
            break;
        case Status::Disabled:
            setIcon(icons.disabled);
            setToolTip(tr("WhisperApp - Disabled (e.g., no microphone or model issue)."));
            Logger::instance().log(Logger::LogLevel::Debug, "TrayIcon", "Status changed to Disabled");
            break;
        default:
            setIcon(icons.idle);
            setToolTip(tr("WhisperApp"));
            break;
    }
}


void TrayIcon::createTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    
    // Initial icon is set by setStatus -> updateIcon in constructor
    // m_trayIcon->setIcon(icons.idle);
    // m_trayIcon->setToolTip(tr("WhisperApp - Ready"));
    
    // Create flash timer for recording indication
    m_flashTimer = new QTimer(this);
    m_flashTimer->setInterval(500); // Flash every 500ms
    connect(m_flashTimer, &QTimer::timeout, this, &TrayIcon::onFlashTimer);
}

// Renamed from updateContextMenu to updateMenu to match TrayIcon.h slot name
void TrayIcon::updateMenu()
{
    m_contextMenu = new QMenu();
    
    // Apply theme to menu
    UIUtils::applyTheme(UIUtils::getCurrentTheme(), m_contextMenu);
    
    // Create actions
    QAction* showAction = new QAction(tr("&Show Window"), this);
    showAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(showAction, &QAction::triggered, this, &TrayIcon::showWindowRequested);
    
    m_recordAction = new QAction(tr("&Start Recording"), this);
    m_recordAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
    m_recordAction->setShortcut(QKeySequence("Ctrl+Shift+R"));
    connect(m_recordAction, &QAction::triggered, this, &TrayIcon::toggleRecordingRequested);
    
    QAction* pauseAction = new QAction(tr("&Pause Recording"), this);
    pauseAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause));
    pauseAction->setEnabled(false);
    connect(pauseAction, &QAction::triggered, this, &TrayIcon::pauseRecordingRequested);
    
    QAction* separator1 = new QAction(this);
    separator1->setSeparator(true);
    
    QAction* historyAction = new QAction(tr("&History..."), this);
    historyAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(historyAction, &QAction::triggered, this, &TrayIcon::showHistoryRequested);
    
    QAction* settingsAction = new QAction(tr("&Settings..."), this);
    settingsAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(settingsAction, &QAction::triggered, this, &TrayIcon::showSettingsRequested);
    
    QAction* separator2 = new QAction(this);
    separator2->setSeparator(true);
    
    QAction* aboutAction = new QAction(tr("&About"), this);
    aboutAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
    connect(aboutAction, &QAction::triggered, this, &TrayIcon::showAboutRequested);
    
    QAction* helpAction = new QAction(tr("&Help"), this);
    helpAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogHelpButton));
    connect(helpAction, &QAction::triggered, this, &TrayIcon::showHelpRequested);
    
    QAction* separator3 = new QAction(this);
    separator3->setSeparator(true);
    
    QAction* exitAction = new QAction(tr("E&xit"), this);
    exitAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCloseButton));
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAction, &QAction::triggered, this, &TrayIcon::exitRequested);
    
    // Build menu
    m_contextMenu->addAction(showAction);
    m_contextMenu->addAction(separator1);
    m_contextMenu->addAction(m_recordAction);
    m_contextMenu->addAction(pauseAction);
    m_contextMenu->addAction(separator2);
    m_contextMenu->addAction(historyAction);
    m_contextMenu->addAction(settingsAction);
    m_contextMenu->addAction(separator3);
    m_contextMenu->addAction(helpAction);
    m_contextMenu->addAction(aboutAction);
    m_contextMenu->addAction(separator3);
    m_contextMenu->addAction(exitAction);
    
    // Store actions for later updates
    m_pauseAction = pauseAction; // Ensure m_pauseAction is a member if used like this
    
    // Set menu
    if (m_trayIcon) {
        m_trayIcon->setContextMenu(m_contextMenu);
    }

    // Update menu items based on current state
    bool isCurrentlyRecording = (current_status == Status::Recording);
    if (m_recordAction) {
        if (isCurrentlyRecording) {
            m_recordAction->setText(tr("&Stop Recording"));
            m_recordAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaStop));
        } else {
            m_recordAction->setText(tr("&Start Recording"));
            m_recordAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
        }
    }
    
    if (m_pauseAction) {
        m_pauseAction->setEnabled(isCurrentlyRecording);
    }
}

void TrayIcon::connectSignals()
{
    if (m_trayIcon) {
        connect(m_trayIcon, &QSystemTrayIcon::activated,
                this, &TrayIcon::onActivated); // Connect to onActivated as per TrayIcon.h

        connect(m_trayIcon, &QSystemTrayIcon::messageClicked,
                this, &TrayIcon::onNotificationClicked); // Connect to onNotificationClicked
    }
}

// This method is now updateMenu, called by setStatus.
// void TrayIcon::updateContextMenu(bool recording)
// {
//     if (m_recordAction) {
//         if (recording) {
//             m_recordAction->setText(tr("&Stop Recording"));
//             m_recordAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaStop));
//         } else {
//             m_recordAction->setText(tr("&Start Recording"));
//             m_recordAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
//         }
//     }

//     if (m_pauseAction) {
//         m_pauseAction->setEnabled(recording);
//     }
// }

void TrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Trigger:
            // Single click - show window
            emit showWindowRequested();
            break;
            
        case QSystemTrayIcon::DoubleClick:
            // Double click - toggle recording
            emit toggleRecordingRequested();
            break;
            
        case QSystemTrayIcon::MiddleClick:
            // Middle click - show history
            emit showHistoryRequested();
            break;
            
        case QSystemTrayIcon::Context:
            // Right click - menu shown automatically
            break;
            
        default:
            break;
    }
}

void TrayIcon::onMessageClicked()
{
    emit showWindowRequested();
    Logger::instance().log(Logger::LogLevel::Debug, "TrayIcon", "Notification clicked");
}

void TrayIcon::onFlashTimer()
{
    if (current_status != Status::Recording) { // Check against current_status
        return;
    }
    
    m_flashState = !m_flashState;
    
    // We use createRecordingIcon(bool) to get bright/dim versions for flashing
    // icons.recording should be the base (e.g., non-bright) recording icon.
    if (m_flashState) {
        m_trayIcon->setIcon(createRecordingIcon(true)); // Bright version for flash
    } else {
        m_trayIcon->setIcon(icons.recording); // Base recording icon (non-bright)
    }
}

void TrayIcon::startFlashing()
{
    if (m_flashTimer && !m_flashTimer->isActive()) {
        m_flashTimer->start();
    }
}

void TrayIcon::stopFlashing()
{
    if (m_flashTimer && m_flashTimer->isActive()) {
        m_flashTimer->stop();
        m_flashState = false;
    }
}

QIcon TrayIcon::createNormalIcon()
{
    // Create a simple microphone icon
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw microphone shape
    QPen pen(QColor(100, 100, 100), 4);
    painter.setPen(pen);
    painter.setBrush(QColor(200, 200, 200));
    
    // Microphone body
    QRectF micBody(22, 12, 20, 30);
    painter.drawRoundedRect(micBody, 10, 10);
    
    // Microphone stand
    painter.drawLine(32, 42, 32, 52);
    painter.drawArc(22, 40, 20, 20, 0, 180 * 16);
    
    // Base
    painter.drawLine(20, 52, 44, 52);
    
    return QIcon(pixmap);
}

// The parameter 'bright' is for the flashing effect.
// icons.recording will store the base (non-bright) recording icon.
QIcon TrayIcon::createRecordingIcon(bool bright)
{
    // Create a red recording icon
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw microphone shape
    QColor micColor = bright ? QColor(255, 100, 100) : QColor(200, 0, 0);
    QPen pen(micColor.darker(), 4);
    painter.setPen(pen);
    painter.setBrush(micColor);
    
    // Microphone body
    QRectF micBody(22, 12, 20, 30);
    painter.drawRoundedRect(micBody, 10, 10);
    
    // Microphone stand
    painter.drawLine(32, 42, 32, 52);
    painter.drawArc(22, 40, 20, 20, 0, 180 * 16);
    
    // Base
    painter.drawLine(20, 52, 44, 52);
    
    // Recording indicator (red dot)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 0, 0));
    painter.drawEllipse(44, 8, 12, 12);
    
    return QIcon(pixmap);
}

QIcon TrayIcon::createProcessingIcon()
{
    // Using a system icon as a placeholder
    QStyle* style = QApplication::style();
    return style->standardIcon(QStyle::SP_ArrowClockwise);
}

QIcon TrayIcon::createErrorIcon()
{
    // Using a system icon as a placeholder
    QStyle* style = QApplication::style();
    return style->standardIcon(QStyle::SP_MessageBoxCritical);
}

QIcon TrayIcon::createDisabledIcon()
{
    // Attempt to create a grayed-out version of the idle icon
    QPixmap originalPixmap = icons.idle.pixmap(64, 64);
    if (originalPixmap.isNull() && m_trayIcon) { // Fallback if icons.idle not yet populated during early call
        originalPixmap = createNormalIcon().pixmap(64,64);
    }

    QImage image = originalPixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QRgb pixel = image.pixel(x, y);
            int gray = qGray(pixel);
            image.setPixel(x, y, qRgba(gray, gray, gray, qAlpha(pixel) / 2)); // Gray out and reduce alpha
        }
    }
    return QIcon(QPixmap::fromImage(image));
    // Fallback to a system icon if pixmap manipulation is problematic
    // QStyle* style = QApplication::style();
    // return style->standardIcon(QStyle::SP_MessageBoxWarning);
}


// Methods from TrayIcon.h that were not in the original .cpp
// but are needed for full class functionality as per .h
// These are simplified versions or direct calls.

void TrayIcon::setVisible(bool visible) {
    if (visible) show();
    else hide();
}

bool TrayIcon::isVisible() const {
    return m_trayIcon ? m_trayIcon->isVisible() : false;
}

// setTooltip is already present

void TrayIcon::updateRecordingTime(int seconds) {
    // This function might need more complex logic if the tooltip
    // needs to combine recording time with current status text.
    // For now, if recording, it will be overridden by updateIcon's tooltip.
    // If not recording, this call might be irrelevant.
    // Consider integrating this into updateIcon if status is Recording.
    if (current_status == Status::Recording) {
        setToolTip(tr("WhisperApp - Recording... (%1s)").arg(seconds));
    }
    recording_time_seconds = seconds; // Store it anyway
}

void TrayIcon::setEnabled(bool enabled) {
    if (m_trayIcon) {
        m_trayIcon->setEnabled(enabled);
        if (!enabled) {
            setStatus(Status::Disabled);
        } else if (current_status == Status::Disabled) {
            setStatus(Status::Idle); // Or previous state if known
        }
    }
}

// Static methods
bool TrayIcon::isSystemTrayAvailable()
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

bool TrayIcon::supportsMessages()
{
    return QSystemTrayIcon::supportsMessages();
}

// Slots from .h that might not have been fully connected or implemented in the old .cpp
// These are placeholders or simplified implementations based on typical usage.
void TrayIcon::onShowHide() {
    // This would typically be connected to an action like "Show/Hide Window"
    // The actual show/hide logic for the main window is via signals:
    // showWindowRequested() / hideWindowRequested()
    // This slot might be redundant if actions directly emit those signals.
}

void TrayIcon::onStartStopRecording() {
    // Connected to start/stop recording action
    // Emits toggleRecordingRequested()
    if (current_status == Status::Recording) {
        emit stopRecordingRequested();
    } else {
        emit startRecordingRequested();
    }
}

void TrayIcon::onOpenSettings() {
    emit settingsRequested();
}

void TrayIcon::onAbout() {
    // Assuming main_window has a method to show about dialog
    // This requires main_window to be correctly passed and stored.
    // emit showAboutRequested(); // if main_window handles it via signal
}

void TrayIcon::onExit() {
    emit exitRequested();
}

// getIconForStatus and getNotificationIcon are marked const in .h
// And are helper methods, not slots.
QIcon TrayIcon::getIconForStatus(Status status) const {
    switch (status) {
        case Status::Idle: return icons.idle;
        case Status::Recording: return icons.recording;
        case Status::Processing: return icons.processing;
        case Status::Error: return icons.error;
        case Status::Disabled: return icons.disabled;
        default: return icons.idle;
    }
}

QIcon TrayIcon::getNotificationIcon(NotificationType type) const {
    // This is a helper, actual icon for QSystemTrayIcon::showMessage
    // is determined by its own enum. This method could map
    // NotificationType to QSystemTrayIcon::MessageIcon if needed internally.
    // For now, returning a default.
    return icons.idle;
}

QString TrayIcon::formatTime(int seconds) const {
    // Helper for formatting time, e.g. for tooltips or messages
    return QString("%1:%2").arg(seconds / 60, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0'));
}