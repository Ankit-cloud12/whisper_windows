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

TrayIcon::TrayIcon(QObject* parent)
    : QObject(parent)
    , m_trayIcon(nullptr)
    , m_contextMenu(nullptr)
    , m_recordAction(nullptr)
    , m_isRecording(false)
    , m_flashTimer(nullptr)
    , m_flashState(false)
{
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        createTrayIcon();
        createContextMenu();
        connectSignals();
        
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
    m_isRecording = recording;
    
    if (recording) {
        setToolTip(tr("WhisperApp - Recording..."));
        startFlashing();
        
        // Update icon to recording state
        QIcon recordingIcon = createRecordingIcon();
        setIcon(recordingIcon);
        
        Logger::instance().log(Logger::LogLevel::Debug, "TrayIcon", "Updated to recording state");
    } else {
        setToolTip(tr("WhisperApp - Ready"));
        stopFlashing();
        
        // Update icon to normal state
        QIcon normalIcon = createNormalIcon();
        setIcon(normalIcon);
        
        Logger::instance().log(Logger::LogLevel::Debug, "TrayIcon", "Updated to idle state");
    }
    
    updateContextMenu(recording);
}

void TrayIcon::createTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    
    // Create default icon
    QIcon appIcon = createNormalIcon();
    m_trayIcon->setIcon(appIcon);
    
    m_trayIcon->setToolTip(tr("WhisperApp - Ready"));
    
    // Create flash timer for recording indication
    m_flashTimer = new QTimer(this);
    m_flashTimer->setInterval(500); // Flash every 500ms
    connect(m_flashTimer, &QTimer::timeout, this, &TrayIcon::onFlashTimer);
}

void TrayIcon::createContextMenu()
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
    m_pauseAction = pauseAction;
    
    // Set menu
    if (m_trayIcon) {
        m_trayIcon->setContextMenu(m_contextMenu);
    }
}

void TrayIcon::connectSignals()
{
    if (m_trayIcon) {
        connect(m_trayIcon, &QSystemTrayIcon::activated,
                this, &TrayIcon::onTrayIconActivated);
        
        connect(m_trayIcon, &QSystemTrayIcon::messageClicked,
                this, &TrayIcon::onMessageClicked);
    }
}

void TrayIcon::updateContextMenu(bool recording)
{
    if (m_recordAction) {
        if (recording) {
            m_recordAction->setText(tr("&Stop Recording"));
            m_recordAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaStop));
        } else {
            m_recordAction->setText(tr("&Start Recording"));
            m_recordAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
        }
    }
    
    if (m_pauseAction) {
        m_pauseAction->setEnabled(recording);
    }
}

void TrayIcon::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
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
    if (!m_isRecording) {
        return;
    }
    
    m_flashState = !m_flashState;
    
    if (m_flashState) {
        setIcon(createRecordingIcon(true));
    } else {
        setIcon(createRecordingIcon(false));
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

bool TrayIcon::isSystemTrayAvailable()
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

bool TrayIcon::supportsMessages()
{
    return QSystemTrayIcon::supportsMessages();
}