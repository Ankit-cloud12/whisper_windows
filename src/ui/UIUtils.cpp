#include "UIUtils.h"
#include "../core/Settings.h"
#include "../core/Logger.h"
#include "AboutDialog.h"
#include <QWidget>
#include <QMainWindow>
#include <QDialog>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QStyle>
#include <QStyleFactory>
#include <QPalette>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QScreen>
#include <QSettings>
#include <QDateTime>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

namespace UIUtils
{
    static Theme g_currentTheme = Theme::Auto;
    static qreal g_dpiScale = 1.0;
    
    void initialize()
    {
        // Calculate DPI scale
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            g_dpiScale = screen->logicalDotsPerInch() / 96.0;
        }
        
        // Load theme preference
        Settings& settings = Settings::instance();
        int themeValue = settings.getSetting(Settings::Key::Theme).toInt();
        g_currentTheme = static_cast<Theme>(themeValue);
        
        // Apply initial theme
        applyTheme(g_currentTheme);
        
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::Info, "UIUtils",
                             QString("UI initialized with DPI scale: %1").arg(g_dpiScale).toStdString());
    }
    
    void applyTheme(Theme theme, QWidget* widget)
    {
        g_currentTheme = theme;
        
        Theme effectiveTheme = theme;
        if (theme == Theme::Auto) {
            effectiveTheme = getSystemTheme();
        }
        
        QString styleSheet = getStyleSheet(effectiveTheme);
        
        if (widget) {
            widget->setStyleSheet(styleSheet);
        } else {
            qApp->setStyleSheet(styleSheet);
        }
        
        // Update palette
        QPalette palette;
        if (effectiveTheme == Theme::Dark) {
            // Dark theme colors
            palette.setColor(QPalette::Window, QColor(53, 53, 53));
            palette.setColor(QPalette::WindowText, Qt::white);
            palette.setColor(QPalette::Base, QColor(42, 42, 42));
            palette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
            palette.setColor(QPalette::ToolTipBase, Qt::white);
            palette.setColor(QPalette::ToolTipText, Qt::white);
            palette.setColor(QPalette::Text, Qt::white);
            palette.setColor(QPalette::Button, QColor(53, 53, 53));
            palette.setColor(QPalette::ButtonText, Qt::white);
            palette.setColor(QPalette::BrightText, Qt::red);
            palette.setColor(QPalette::Link, QColor(42, 130, 218));
            palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
            palette.setColor(QPalette::HighlightedText, Qt::black);
            palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
        } else {
            // Light theme (default)
            palette = qApp->style()->standardPalette();
        }
        
        if (widget) {
            widget->setPalette(palette);
        } else {
            qApp->setPalette(palette);
        }
        
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::Debug, "UIUtils",
                             QString("Applied theme: %1").arg(static_cast<int>(effectiveTheme)).toStdString());
    }
    
    Theme getCurrentTheme()
    {
        return g_currentTheme;
    }
    
    Theme getSystemTheme()
    {
        // Check system theme (Windows 10/11)
#ifdef Q_OS_WIN
        QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 
                          QSettings::NativeFormat);
        if (settings.contains("AppsUseLightTheme")) {
            bool isLight = settings.value("AppsUseLightTheme").toBool();
            return isLight ? Theme::Light : Theme::Dark;
        }
#endif
        
        // Default to light theme
        return Theme::Light;
    }
    
    QIcon loadIcon(const QString& name, int size)
    {
        QString iconPath = QString(":/icons/%1.png").arg(name);
        QIcon icon(iconPath);
        
        if (icon.isNull()) {
            // Try SVG
            iconPath = QString(":/icons/%1.svg").arg(name);
            icon = QIcon(iconPath);
        }
        
        if (!icon.isNull() && size > 0) {
            return QIcon(icon.pixmap(size, size));
        }
        
        return icon;
    }
    
    QIcon loadThemedIcon(const QString& lightName, const QString& darkName, int size)
    {
        Theme effectiveTheme = g_currentTheme;
        if (g_currentTheme == Theme::Auto) {
            effectiveTheme = getSystemTheme();
        }
        
        QString iconName = (effectiveTheme == Theme::Dark) ? darkName : lightName;
        return loadIcon(iconName, size);
    }
    
    QColor getThemedColor(const QColor& lightColor, const QColor& darkColor)
    {
        Theme effectiveTheme = g_currentTheme;
        if (g_currentTheme == Theme::Auto) {
            effectiveTheme = getSystemTheme();
        }
        
        return (effectiveTheme == Theme::Dark) ? darkColor : lightColor;
    }
    
    int scaleForDPI(int value)
    {
        return static_cast<int>(value * g_dpiScale);
    }
    
    QFont scaleFont(const QFont& font)
    {
        QFont scaledFont = font;
        scaledFont.setPointSizeF(font.pointSizeF() * g_dpiScale);
        return scaledFont;
    }
    
    void centerWindow(QWidget* window, QWidget* parent)
    {
        if (!window) return;
        
        QRect targetGeometry;
        
        if (parent) {
            targetGeometry = parent->geometry();
        } else {
            QScreen* screen = QApplication::primaryScreen();
            if (screen) {
                targetGeometry = screen->availableGeometry();
            }
        }
        
        window->move(targetGeometry.center() - window->rect().center());
    }
    
    void showAboutDialog(QWidget* parent)
    {
        AboutDialog dialog(parent);
        dialog.exec();
    }
    
    void showError(QWidget* parent, const QString& title, 
                   const QString& message, const QString& details)
    {
        QMessageBox msgBox(parent);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle(title);
        msgBox.setText(message);
        
        if (!details.isEmpty()) {
            msgBox.setDetailedText(details);
        }
        
        msgBox.exec();
        
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::Error, "UIUtils",
                             QString("Error dialog: %1 - %2").arg(title, message).toStdString());
    }
    
    void showWarning(QWidget* parent, const QString& title, const QString& message)
    {
        QMessageBox::warning(parent, title, message);
        
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::Warning, "UIUtils",
                             QString("Warning dialog: %1 - %2").arg(title, message).toStdString());
    }
    
    void showInfo(QWidget* parent, const QString& title, const QString& message)
    {
        QMessageBox::information(parent, title, message);
    }
    
    bool askQuestion(QWidget* parent, const QString& title, const QString& question)
    {
        int result = QMessageBox::question(parent, title, question,
                                         QMessageBox::Yes | QMessageBox::No);
        return result == QMessageBox::Yes;
    }
    
    QMenu* createFileMenu(QWidget* parent)
    {
        QMenu* menu = new QMenu(QObject::tr("&File"), parent);
        
        menu->addAction(QObject::tr("&New"), parent, SLOT(newFile()))
            ->setShortcut(QKeySequence::New);
        menu->addAction(QObject::tr("&Open..."), parent, SLOT(openFile()))
            ->setShortcut(QKeySequence::Open);
        menu->addAction(QObject::tr("&Save"), parent, SLOT(saveFile()))
            ->setShortcut(QKeySequence::Save);
        menu->addAction(QObject::tr("Save &As..."), parent, SLOT(saveFileAs()))
            ->setShortcut(QKeySequence::SaveAs);
        menu->addSeparator();
        menu->addAction(QObject::tr("E&xit"), parent, SLOT(close()))
            ->setShortcut(QKeySequence::Quit);
        
        return menu;
    }
    
    QMenu* createEditMenu(QWidget* parent)
    {
        QMenu* menu = new QMenu(QObject::tr("&Edit"), parent);
        
        menu->addAction(QObject::tr("&Undo"), parent, SLOT(undo()))
            ->setShortcut(QKeySequence::Undo);
        menu->addAction(QObject::tr("&Redo"), parent, SLOT(redo()))
            ->setShortcut(QKeySequence::Redo);
        menu->addSeparator();
        menu->addAction(QObject::tr("Cu&t"), parent, SLOT(cut()))
            ->setShortcut(QKeySequence::Cut);
        menu->addAction(QObject::tr("&Copy"), parent, SLOT(copy()))
            ->setShortcut(QKeySequence::Copy);
        menu->addAction(QObject::tr("&Paste"), parent, SLOT(paste()))
            ->setShortcut(QKeySequence::Paste);
        menu->addSeparator();
        menu->addAction(QObject::tr("Select &All"), parent, SLOT(selectAll()))
            ->setShortcut(QKeySequence::SelectAll);
        
        return menu;
    }
    
    QMenu* createHelpMenu(QWidget* parent)
    {
        QMenu* menu = new QMenu(QObject::tr("&Help"), parent);
        
        menu->addAction(QObject::tr("&Help"), parent, SLOT(showHelp()))
            ->setShortcut(QKeySequence::HelpContents);
        menu->addSeparator();
        menu->addAction(QObject::tr("&About"), parent, SLOT(showAbout()));
        
        return menu;
    }
    
    void setAlwaysOnTop(QWidget* window, bool alwaysOnTop)
    {
        if (!window) return;
        
        Qt::WindowFlags flags = window->windowFlags();
        
        if (alwaysOnTop) {
            flags |= Qt::WindowStaysOnTopHint;
        } else {
            flags &= ~Qt::WindowStaysOnTopHint;
        }
        
        window->setWindowFlags(flags);
        window->show(); // Required to apply the change
    }
    
    void setWindowOpacity(QWidget* window, qreal opacity)
    {
        if (!window) return;
        
        opacity = qBound(0.0, opacity, 1.0);
        window->setWindowOpacity(opacity);
    }
    
    void enableFileDragDrop(QWidget* widget, const QStringList& extensions)
    {
        if (!widget) return;
        
        widget->setAcceptDrops(true);
        
        // Store extensions in widget property
        widget->setProperty("acceptedExtensions", extensions);
        
        // Note: The actual drag/drop handling needs to be implemented
        // in the widget's dragEnterEvent and dropEvent methods
    }
    
    QString formatDuration(int seconds)
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
    
    QString formatFileSize(qint64 bytes)
    {
        const qint64 KB = 1024;
        const qint64 MB = KB * 1024;
        const qint64 GB = MB * 1024;
        const qint64 TB = GB * 1024;
        
        if (bytes >= TB) {
            return QString::number(bytes / double(TB), 'f', 2) + " TB";
        } else if (bytes >= GB) {
            return QString::number(bytes / double(GB), 'f', 2) + " GB";
        } else if (bytes >= MB) {
            return QString::number(bytes / double(MB), 'f', 1) + " MB";
        } else if (bytes >= KB) {
            return QString::number(bytes / double(KB), 'f', 1) + " KB";
        } else {
            return QString::number(bytes) + " B";
        }
    }
    
    QString formatTimestamp(const QDateTime& timestamp, const QString& format)
    {
        if (format.isEmpty()) {
            // Use locale-specific format
            return timestamp.toString(Qt::SystemLocaleLongDate);
        }
        return timestamp.toString(format);
    }
    
    QString getStyleSheet(Theme theme)
    {
        QString styleSheet;
        
        if (theme == Theme::Dark) {
            styleSheet = R"(
                QToolTip {
                    color: #ffffff;
                    background-color: #2a2a2a;
                    border: 1px solid white;
                }
                
                QGroupBox {
                    border: 1px solid #666;
                    border-radius: 5px;
                    margin-top: 10px;
                    font-weight: bold;
                }
                
                QGroupBox::title {
                    subcontrol-origin: margin;
                    left: 10px;
                    padding: 0 5px 0 5px;
                }
                
                QTabWidget::pane {
                    border-top: 2px solid #666;
                }
                
                QTabBar::tab {
                    background-color: #3a3a3a;
                    padding: 5px 10px;
                    margin-right: 2px;
                }
                
                QTabBar::tab:selected {
                    background-color: #555;
                }
                
                QProgressBar {
                    border: 1px solid #666;
                    border-radius: 3px;
                    text-align: center;
                }
                
                QProgressBar::chunk {
                    background-color: #2a82da;
                    border-radius: 3px;
                }
            )";
        } else {
            styleSheet = R"(
                QGroupBox {
                    border: 1px solid #ccc;
                    border-radius: 5px;
                    margin-top: 10px;
                    font-weight: bold;
                }
                
                QGroupBox::title {
                    subcontrol-origin: margin;
                    left: 10px;
                    padding: 0 5px 0 5px;
                }
                
                QTabWidget::pane {
                    border-top: 2px solid #ccc;
                }
                
                QTabBar::tab {
                    background-color: #f0f0f0;
                    padding: 5px 10px;
                    margin-right: 2px;
                }
                
                QTabBar::tab:selected {
                    background-color: #fff;
                }
                
                QProgressBar {
                    border: 1px solid #ccc;
                    border-radius: 3px;
                    text-align: center;
                }
                
                QProgressBar::chunk {
                    background-color: #4caf50;
                    border-radius: 3px;
                }
            )";
        }
        
        return styleSheet;
    }
    
    QString loadStyleSheet(const QString& path)
    {
        QFile file(path);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream stream(&file);
            return stream.readAll();
        }
        
        WhisperApp::Logger::getInstance().log(WhisperApp::LogLevel::Warning, "UIUtils",
                             QString("Failed to load stylesheet: %1").arg(path).toStdString());
        return QString();
    }
    
    void saveWindowGeometry(QWidget* window, const QString& key)
    {
        if (!window) return;
        
        QSettings settings("WhisperApp", "WindowGeometry");
        settings.setValue(key + "/geometry", window->saveGeometry());
        
        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(window);
        if (mainWindow) {
            settings.setValue(key + "/state", mainWindow->saveState());
        }
    }
    
    void restoreWindowGeometry(QWidget* window, const QString& key)
    {
        if (!window) return;
        
        QSettings settings("WhisperApp", "WindowGeometry");
        
        if (settings.contains(key + "/geometry")) {
            window->restoreGeometry(settings.value(key + "/geometry").toByteArray());
        }
        
        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(window);
        if (mainWindow && settings.contains(key + "/state")) {
            mainWindow->restoreState(settings.value(key + "/state").toByteArray());
        }
    }
}