/*
 * main.cpp
 * 
 * Entry point for the Whisper application
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QStandardPaths>
#include <QDir>
#include <QTranslator>
#include <QLocale>
#include <QStyleFactory>
#include <QSettings>
#include <QMessageBox>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QThread>
#include <memory>

#include "ui/MainWindow.h"
#include "core/WhisperEngine.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"
#include "core/AppInfo.h"
#include "core/Localization.h"
#include "core/ThemeManager.h"

using namespace WhisperApp;

/**
 * @brief Configure application-wide settings
 */
void setupApplication(QApplication& app) {
    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(APP_VERSION);
    app.setApplicationDisplayName(APP_DISPLAY_NAME);
    app.setOrganizationName(APP_ORGANIZATION);
    app.setOrganizationDomain(APP_DOMAIN);
    
    // Set application icon
    app.setWindowIcon(QIcon(":/icons/app_icon.png"));
    
    // Enable high DPI support
    app.setAttribute(Qt::AA_EnableHighDpiScaling);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
}

/**
 * @brief Initialize the logger
 */
bool initializeLogger() {
    // Get application data directory
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    
    QString logFilePath = appDataPath + "/whisper.log";
    
    try {
        auto& logger = WhisperApp::Logger::getInstance();
        logger.setLogFile(logFilePath.toStdString());
        logger.setLogLevel(WhisperApp::LogLevel::INFO);
        
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::INFO, "Application", "Application starting up");
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::INFO, "Application", "Version: " + std::string(APP_VERSION));
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::INFO, "Application", "Log file: " + logFilePath.toStdString());
        
        return true;
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Logger Error", 
                            QString("Failed to initialize logger: %1").arg(e.what()));
        return false;
    }
}

/**
 * @brief Load application translations
 */
void loadTranslations(QApplication& app) {
    QTranslator* translator = new QTranslator(&app);
    QTranslator* qtTranslator = new QTranslator(&app);
    
    QString locale = QLocale::system().name();
    
    // Load application translations
    if (translator->load(QString("whisperapp_%1").arg(locale), ":/translations")) {
        app.installTranslator(translator);
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::INFO, "Application", "Loaded translations for: " + locale.toStdString());
    } else {
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::WARNING, "Application", "No translations found for: " + locale.toStdString());
    }
    
    // Load Qt translations
    if (qtTranslator->load(QString("qt_%1").arg(locale), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(qtTranslator);
    }
}

/**
 * @brief Apply application theme
 */
void applyTheme(QApplication& app) {
    try {
        QSettings settings;
        QString themeName = settings.value("theme", "default").toString();
        
        ThemeManager themeManager;
        if (themeManager.applyTheme(themeName)) {
            WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::INFO, "Application", "Applied theme: " + themeName.toStdString());
        } else {
            WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::WARNING, "Application", "Failed to apply theme: " + themeName.toStdString());
        }
    } catch (const std::exception& e) {
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::ERROR, "Application", "Theme error: " + std::string(e.what()));
    }
}

/**
 * @brief Show splash screen during initialization
 */
QSplashScreen* showSplashScreen() {
    QPixmap pixmap(":/images/splash.png");
    if (pixmap.isNull()) {
        // Create a simple splash screen if image is not available
        pixmap = QPixmap(400, 200);
        pixmap.fill(Qt::white);
    }
    
    QSplashScreen* splash = new QSplashScreen(pixmap);
    splash->show();
    splash->showMessage("Starting WhisperApp...", Qt::AlignBottom | Qt::AlignCenter, Qt::black);
    
    QApplication::processEvents();
    return splash;
}

/**
 * @brief Initialize the Whisper engine
 */
bool initializeWhisperEngine() {
    try {
        auto engine = std::make_unique<WhisperEngine>();
        
        WhisperApp::LogTimer timer("Application", "WhisperEngine initialization");
        
        if (!engine->initialize()) {
            WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::ERROR, "Application", "Failed to initialize Whisper engine");
            return false;
        }
        
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::INFO, "Application", "Whisper engine initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::ERROR, "Application", "Whisper engine initialization error: " + std::string(e.what()));
        return false;
    }
}

/**
 * @brief Parse command line arguments
 */
void parseCommandLine(QApplication& app) {
    QCommandLineParser parser;
    parser.setApplicationDescription(APP_DESCRIPTION);
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Add custom options
    QCommandLineOption verboseOption(
        QStringList() << "v" << "verbose",
        "Enable verbose logging");
    parser.addOption(verboseOption);
    
    QCommandLineOption logLevelOption(
        QStringList() << "l" << "log-level",
        "Set log level (DEBUG, INFO, WARNING, ERROR)",
        "level", "INFO");
    parser.addOption(logLevelOption);
    
    QCommandLineOption configFileOption(
        QStringList() << "c" << "config",
        "Use custom configuration file",
        "file");
    parser.addOption(configFileOption);
    
    parser.process(app);
    
    // Apply command line options
    if (parser.isSet(verboseOption)) {
        auto& logger = WhisperApp::Logger::getInstance();
        logger.setLogLevel(WhisperApp::LogLevel::DEBUG);
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::INFO, "Application", "Verbose logging enabled");
    }
    
    if (parser.isSet(logLevelOption)) {
        QString levelStr = parser.value(logLevelOption).toUpper();
        auto& logger = WhisperApp::Logger::getInstance();
        
        if (levelStr == "DEBUG") {
            logger.setLogLevel(WhisperApp::LogLevel::DEBUG);
        } else if (levelStr == "INFO") {
            logger.setLogLevel(WhisperApp::LogLevel::INFO);
        } else if (levelStr == "WARNING") {
            logger.setLogLevel(WhisperApp::LogLevel::WARNING);
        } else if (levelStr == "ERROR") {
            logger.setLogLevel(WhisperApp::LogLevel::ERROR);
        } else {
            WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::WARNING, "Application", "Unknown log level: " + levelStr.toStdString());
        }
    }
    
    if (parser.isSet(configFileOption)) {
        QString configFile = parser.value(configFileOption);
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::INFO, "Application", "Using config file: " + configFile.toStdString());
        // TODO: Load custom configuration file
    }
}

/**
 * @brief Main application entry point
 */
int main(int argc, char *argv[]) {
    // Create QApplication
    QApplication app(argc, argv);
    
    // Setup application properties
    setupApplication(app);
    
    // Initialize logger first
    if (!initializeLogger()) {
        return 1;
    }
    
    // Parse command line arguments
    parseCommandLine(app);
    
    // Show splash screen
    QSplashScreen* splash = showSplashScreen();
    
    try {
        // Load translations
        splash->showMessage("Loading translations...", Qt::AlignBottom | Qt::AlignCenter, Qt::black);
        QApplication::processEvents();
        loadTranslations(app);
        
        // Apply theme
        splash->showMessage("Applying theme...", Qt::AlignBottom | Qt::AlignCenter, Qt::black);
        QApplication::processEvents();
        applyTheme(app);
        
        // Initialize Whisper engine
        splash->showMessage("Initializing Whisper engine...", Qt::AlignBottom | Qt::AlignCenter, Qt::black);
        QApplication::processEvents();
        
        if (!initializeWhisperEngine()) {
            splash->finish(nullptr);
            QMessageBox::critical(nullptr, "Initialization Error", 
                                "Failed to initialize Whisper engine. Please check the logs for details.");
            return 2;
        }
        
        // Create and show main window
        splash->showMessage("Creating main window...", Qt::AlignBottom | Qt::AlignCenter, Qt::black);
        QApplication::processEvents();
        
        MainWindow window;
        splash->finish(&window);
        window.show();
        
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::INFO, "Application", "Application startup completed");
        
        // Run application event loop
        int result = app.exec();
        
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::INFO, "Application", "Application shutting down");
        return result;
        
    } catch (const std::exception& e) {
        splash->finish(nullptr);
        
        QString errorMsg = QString("Unhandled exception: %1").arg(e.what());
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::ERROR, "Application", errorMsg.toStdString());
        
        QMessageBox::critical(nullptr, "Fatal Error", errorMsg);
        return 3;
        
    } catch (...) {
        splash->finish(nullptr);
        
        QString errorMsg = "Unknown exception occurred";
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::ERROR, "Application", errorMsg.toStdString());
        
        QMessageBox::critical(nullptr, "Fatal Error", errorMsg);
        return 4;
    }
}