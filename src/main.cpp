/*
 * WhisperApp - Windows Speech-to-Text Application
 * 
 * Main application entry point
 * 
 * This file initializes the Qt application, sets up the main window,
 * and starts the event loop. It also handles application-wide settings
 * and ensures single instance execution.
 */

#include <QApplication>
#include <QMessageBox>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <memory>
#include <thread>

#include "ui/MainWindow.h"
#include "ui/TrayIcon.h"
#include "core/Settings.h"
#include "core/Logger.h"
#include "core/WhisperEngine.h"
#include "core/AudioCapture.h"
#include "core/DeviceManager.h"
#include "core/ModelManager.h"
#include "system/WindowManager.h"
#include "system/GlobalHotkeys.h"
#include "system/ClipboardManager.h"

// Application constants
const QString APP_NAME = "WhisperApp";
const QString APP_VERSION = "1.0.0";
const QString ORGANIZATION_NAME = "WhisperApp";
const QString ORGANIZATION_DOMAIN = "whisperapp.com";

// Application context for global access
struct AppContext {
    std::unique_ptr<Settings> settings;
    std::unique_ptr<WhisperEngine> whisperEngine;
    std::unique_ptr<AudioCapture> audioCapture;
    std::unique_ptr<DeviceManager> deviceManager;
    std::unique_ptr<ModelManager> modelManager;
    std::unique_ptr<WindowManager> windowManager;
    std::unique_ptr<GlobalHotkeys> globalHotkeys;
    std::unique_ptr<ClipboardManager> clipboardManager;
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<TrayIcon> trayIcon;
};

// Single instance check
bool isRunning()
{
    QSystemSemaphore semaphore("WhisperAppSemaphore", 1);
    semaphore.acquire();

    QSharedMemory sharedMemory("WhisperAppInstance");
    bool isRunning = !sharedMemory.create(1);

    semaphore.release();
    return isRunning;
}

// Initialize application data directories
bool initializeDirectories()
{
    QStringList dirs = {
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs",
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/models",
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/cache",
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/history"
    };

    for (const QString& dir : dirs) {
        QDir d(dir);
        if (!d.exists() && !d.mkpath(dir)) {
            return false;
        }
    }

    return true;
}

int main(int argc, char *argv[])
{
    // Enable high DPI support
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Create application instance
    QApplication app(argc, argv);

    // Set application metadata
    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(APP_VERSION);
    app.setOrganizationName(ORGANIZATION_NAME);
    app.setOrganizationDomain(ORGANIZATION_DOMAIN);

    // Set application icon
    app.setWindowIcon(QIcon(":/icons/app.ico"));

    // Check for single instance
    if (isRunning()) {
        QMessageBox::warning(nullptr, 
            APP_NAME,
            "WhisperApp is already running.\n"
            "Please check the system tray.");
        return 0;
    }

    // Initialize directories
    if (!initializeDirectories()) {
        QMessageBox::critical(nullptr,
            APP_NAME + " - Error",
            "Failed to create application directories.");
        return 1;
    }

    try {
        // Create application context
        AppContext context;

        // Initialize settings first
        context.settings = std::make_unique<Settings>();
        
        // Initialize logger
        QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
        LoggerConfig logConfig;
        logConfig.logDirectory = logPath.toStdString();
        logConfig.enableConsole = true;
        logConfig.enableFile = true;
        logConfig.consoleLevel = static_cast<LogLevel>(
            context.settings->value("General/LogLevel",
            static_cast<int>(LogLevel::INFO)).toInt());

        Logger::getInstance().initialize(logConfig);
        Logger::getInstance().info("WhisperApp", "WhisperApp starting up...");
        
        // Initialize core components
        context.deviceManager = std::make_unique<DeviceManager>();
        context.modelManager = std::make_unique<ModelManager>(
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString() + "/models");
        
        // Initialize audio capture
        context.audioCapture = std::make_unique<AudioCapture>();
        
        // Initialize Whisper engine
        context.whisperEngine = std::make_unique<WhisperEngine>();
        
        // Load default model if available
        if (context.modelManager->hasModel("tiny")) {
            context.whisperEngine->loadModel(context.modelManager->getModelPath("tiny"));
        }
        
        // Initialize system components
        context.windowManager = std::make_unique<WindowManager>();
        context.clipboardManager = std::make_unique<ClipboardManager>();
        
        // Create main window
        context.mainWindow = std::make_unique<MainWindow>();
        
        // Create tray icon
        context.trayIcon = std::make_unique<TrayIcon>(&app);
        
        // Initialize global hotkeys
        context.globalHotkeys = std::make_unique<GlobalHotkeys>();
        
        // Connect components to main window
        // Audio level updates
        QObject::connect(context.audioCapture.get(), &AudioCapture::levelChanged,
                        context.mainWindow.get(), &MainWindow::updateAudioLevel);
        
        // Transcription results
        QObject::connect(context.whisperEngine.get(), &WhisperEngine::transcriptionComplete,
                        context.mainWindow.get(), &MainWindow::onTranscriptionComplete);
        
        QObject::connect(context.whisperEngine.get(), &WhisperEngine::transcriptionError,
                        context.mainWindow.get(), &MainWindow::onTranscriptionError);
        
        // Recording control
        QObject::connect(context.mainWindow.get(), &MainWindow::recordingStarted,
                        [&context]() {
            Logger::getInstance().info("WhisperApp", "Recording started");
            context.audioCapture->startCapture();
        });
        
        QObject::connect(context.mainWindow.get(), &MainWindow::recordingStopped,
                        [&context]() {
            Logger::getInstance().info("WhisperApp", "Recording stopped");
            context.audioCapture->stopCapture();
            
            // Get audio data and transcribe
            auto audioData = context.audioCapture->getAudioData();
            if (!audioData.empty()) {
                // Configure transcription parameters
                WhisperEngine::TranscriptionParams params;
                params.language = context.settings->value("Transcription/Language", "auto").toString().toStdString();
                params.translate = context.settings->value("Transcription/Translate", false).toBool();
                params.print_timestamps = context.settings->value("Transcription/ShowTimestamps", true).toBool();
                
                // Perform async transcription
                context.whisperEngine->transcribeAudioAsync(
                    audioData,
                    params,
                    // Result callback
                    [&context](const WhisperEngine::TranscriptionResult& result) {
                        if (result.confidence > 0.0f) {
                            // Success
                            QString text = QString::fromStdString(result.text);
                            QMetaObject::invokeMethod(context.mainWindow.get(),
                                "onTranscriptionComplete",
                                Qt::QueuedConnection,
                                Q_ARG(QString, text));
                        } else {
                            // Error
                            QMetaObject::invokeMethod(context.mainWindow.get(),
                                "onTranscriptionError",
                                Qt::QueuedConnection,
                                Q_ARG(QString, QString::fromStdString(result.text)));
                        }
                    },
                    // Progress callback
                    [&context](float progress) {
                        // Update progress in status bar
                        QString progressText = QString("Processing... %1%").arg(static_cast<int>(progress * 100));
                        QMetaObject::invokeMethod(context.mainWindow.get(),
                            [&context, progressText]() {
                                if (context.mainWindow && context.mainWindow->statusBar()) {
                                    context.mainWindow->statusBar()->showMessage(progressText, 0);
                                }
                            },
                            Qt::QueuedConnection);
                    }
                );
            } else {
                Logger::getInstance().warn("WhisperApp", "No audio data captured");
                QMetaObject::invokeMethod(context.mainWindow.get(),
                    "onTranscriptionError",
                    Qt::QueuedConnection,
                    Q_ARG(QString, "No audio data captured"));
            }
        });
        
        // Model changes
        QObject::connect(context.mainWindow.get(), &MainWindow::modelChanged,
                        [&context](const QString& modelName) {
            if (context.modelManager->hasModel(modelName.toStdString())) {
                context.whisperEngine->loadModel(
                    context.modelManager->getModelPath(modelName.toStdString()));
            }
        });
        
        // Language changes
        QObject::connect(context.mainWindow.get(), &MainWindow::languageChanged,
                        [&context](const QString& language) {
            context.whisperEngine->setLanguage(language.toStdString());
        });
        
        // Type text request
        QObject::connect(context.mainWindow.get(), &MainWindow::typeTextRequested,
                        [&context](const QString& text) {
            context.windowManager->typeText(text.toStdWString());
        });
        
        // Global hotkeys
        context.globalHotkeys->registerHotkey("RecordToggle",
            context.settings->value("Hotkeys/RecordToggle", "Ctrl+Alt+R").toString().toStdWString(),
            [&context]() {
                QMetaObject::invokeMethod(context.mainWindow.get(), "toggleRecording");
            });
        
        context.globalHotkeys->registerHotkey("TypeLast",
            context.settings->value("Hotkeys/TypeLast", "Ctrl+Alt+T").toString().toStdWString(),
            [&context]() {
                // Get last transcription and type it
                // This would be implemented in MainWindow
            });
        
        // Tray icon connections
        QObject::connect(context.trayIcon.get(), &TrayIcon::activated,
                        [&context](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger) {
                if (context.mainWindow->isVisible()) {
                    context.mainWindow->hide();
                } else {
                    context.mainWindow->show();
                    context.mainWindow->raise();
                    context.mainWindow->activateWindow();
                }
            }
        });
        
        QObject::connect(context.trayIcon.get(), &TrayIcon::recordingToggled,
                        context.mainWindow.get(), &MainWindow::toggleRecording);
        
        QObject::connect(context.trayIcon.get(), &TrayIcon::showWindowRequested,
                        [&context]() {
            context.mainWindow->show();
            context.mainWindow->raise();
            context.mainWindow->activateWindow();
        });
        
        QObject::connect(context.trayIcon.get(), &TrayIcon::quitRequested,
                        &app, &QApplication::quit);
        
        // Settings changed
        QObject::connect(context.mainWindow.get(), &MainWindow::settingsChanged,
                        [&context]() {
            // Reload settings
            Logger::getInstance().setConsoleLevel(static_cast<LogLevel>(
                context.settings->value("General/LogLevel",
                static_cast<int>(LogLevel::INFO)).toInt()));
            
            // Update audio device
            QString deviceId = context.settings->value("Audio/InputDevice", "").toString();
            if (!deviceId.isEmpty()) {
                context.audioCapture->setDevice(deviceId.toStdString());
            }
            
            // Update hotkeys
            context.globalHotkeys->unregisterHotkey("RecordToggle");
            context.globalHotkeys->unregisterHotkey("TypeLast");
            
            context.globalHotkeys->registerHotkey("RecordToggle",
                context.settings->value("Hotkeys/RecordToggle", "Ctrl+Alt+R").toString().toStdWString(),
                [&context]() {
                    QMetaObject::invokeMethod(context.mainWindow.get(), "toggleRecording");
                });
            
            context.globalHotkeys->registerHotkey("TypeLast",
                context.settings->value("Hotkeys/TypeLast", "Ctrl+Alt+T").toString().toStdWString(),
                [&context]() {
                    // Get last transcription and type it
                });
        });
        
        // Tray notification requests
        QObject::connect(context.mainWindow.get(), &MainWindow::trayNotificationRequested,
                        context.trayIcon.get(), &TrayIcon::showMessage);
        
        // Show tray icon
        context.trayIcon->show();
        
        // Load window state from settings
        if (context.settings->value("UI/StartMinimized", false).toBool()) {
            context.mainWindow->hide();
            if (context.settings->value("UI/ShowTrayNotification", true).toBool()) {
                context.trayIcon->showMessage("WhisperApp", 
                    "WhisperApp is running in the background. Click the tray icon to open.");
            }
        } else {
            context.mainWindow->show();
        }

        // Execute application event loop
        int result = app.exec();
        
        // Cleanup
        Logger::getInstance().info("WhisperApp", "WhisperApp shutting down...");
        context.globalHotkeys->unregisterAll();
        
        return result;
        
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr,
            APP_NAME + " - Error",
            QString("Failed to initialize application:\n%1").arg(e.what()));
        return 1;
    } catch (...) {
        QMessageBox::critical(nullptr,
            APP_NAME + " - Error",
            "An unknown error occurred during initialization.");
        return 1;
    }
}

/*
 * Platform-specific entry point for Windows
 * This ensures proper Windows subsystem linkage
 */
#ifdef _WIN32
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif