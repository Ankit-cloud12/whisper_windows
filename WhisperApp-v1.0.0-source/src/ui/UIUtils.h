#pragma once

#include <QString>
#include <QColor>
#include <QFont>
#include <QIcon>

// Forward declarations
class QWidget;
class QMainWindow;
class QDialog;
class QMenu;
class QAction;

/**
 * @brief UI utility functions and helpers
 * 
 * Provides common UI functionality like theme management,
 * icon loading, dialogs, and DPI handling.
 */
namespace UIUtils
{
    /**
     * @brief Application theme
     */
    enum class Theme {
        Auto,
        Light,
        Dark
    };
    
    /**
     * @brief Initialize UI utilities
     */
    void initialize();
    
    /**
     * @brief Apply application theme
     * @param theme Theme to apply
     * @param widget Target widget (nullptr for application-wide)
     */
    void applyTheme(Theme theme, QWidget* widget = nullptr);
    
    /**
     * @brief Get current theme
     * @return Current theme
     */
    Theme getCurrentTheme();
    
    /**
     * @brief Detect system theme preference
     * @return System theme
     */
    Theme getSystemTheme();
    
    /**
     * @brief Load icon from resources
     * @param name Icon name (without extension)
     * @param size Icon size (0 for original)
     * @return Icon
     */
    QIcon loadIcon(const QString& name, int size = 0);
    
    /**
     * @brief Load themed icon
     * @param lightName Light theme icon name
     * @param darkName Dark theme icon name
     * @param size Icon size
     * @return Icon appropriate for current theme
     */
    QIcon loadThemedIcon(const QString& lightName, const QString& darkName, int size = 0);
    
    /**
     * @brief Get color for current theme
     * @param lightColor Color for light theme
     * @param darkColor Color for dark theme
     * @return Color appropriate for current theme
     */
    QColor getThemedColor(const QColor& lightColor, const QColor& darkColor);
    
    /**
     * @brief Scale value for current DPI
     * @param value Base value
     * @return Scaled value
     */
    int scaleForDPI(int value);
    
    /**
     * @brief Scale font for current DPI
     * @param font Base font
     * @return Scaled font
     */
    QFont scaleFont(const QFont& font);
    
    /**
     * @brief Center window on screen
     * @param window Window to center
     * @param parent Parent widget (for relative centering)
     */
    void centerWindow(QWidget* window, QWidget* parent = nullptr);
    
    /**
     * @brief Create standard about dialog
     * @param parent Parent widget
     */
    void showAboutDialog(QWidget* parent);
    
    /**
     * @brief Show error message
     * @param parent Parent widget
     * @param title Dialog title
     * @param message Error message
     * @param details Detailed error information
     */
    void showError(QWidget* parent, const QString& title, 
                   const QString& message, const QString& details = QString());
    
    /**
     * @brief Show warning message
     * @param parent Parent widget
     * @param title Dialog title
     * @param message Warning message
     */
    void showWarning(QWidget* parent, const QString& title, const QString& message);
    
    /**
     * @brief Show information message
     * @param parent Parent widget
     * @param title Dialog title
     * @param message Information message
     */
    void showInfo(QWidget* parent, const QString& title, const QString& message);
    
    /**
     * @brief Ask yes/no question
     * @param parent Parent widget
     * @param title Dialog title
     * @param question Question text
     * @return true if user selected yes
     */
    bool askQuestion(QWidget* parent, const QString& title, const QString& question);
    
    /**
     * @brief Create standard file menu
     * @param parent Parent widget
     * @return File menu
     */
    QMenu* createFileMenu(QWidget* parent);
    
    /**
     * @brief Create standard edit menu
     * @param parent Parent widget
     * @return Edit menu
     */
    QMenu* createEditMenu(QWidget* parent);
    
    /**
     * @brief Create standard help menu
     * @param parent Parent widget
     * @return Help menu
     */
    QMenu* createHelpMenu(QWidget* parent);
    
    /**
     * @brief Apply window flags for always-on-top
     * @param window Target window
     * @param alwaysOnTop Enable always-on-top
     */
    void setAlwaysOnTop(QWidget* window, bool alwaysOnTop);
    
    /**
     * @brief Set window opacity
     * @param window Target window
     * @param opacity Opacity (0.0 to 1.0)
     */
    void setWindowOpacity(QWidget* window, qreal opacity);
    
    /**
     * @brief Enable drag and drop for file paths
     * @param widget Target widget
     * @param extensions Accepted file extensions
     */
    void enableFileDragDrop(QWidget* widget, const QStringList& extensions);
    
    /**
     * @brief Format duration for display
     * @param seconds Duration in seconds
     * @return Formatted string (HH:MM:SS or MM:SS)
     */
    QString formatDuration(int seconds);
    
    /**
     * @brief Format file size for display
     * @param bytes Size in bytes
     * @return Formatted string (e.g., "1.5 MB")
     */
    QString formatFileSize(qint64 bytes);
    
    /**
     * @brief Format timestamp for display
     * @param timestamp Timestamp
     * @param format Format string
     * @return Formatted string
     */
    QString formatTimestamp(const QDateTime& timestamp, const QString& format = QString());
    
    /**
     * @brief Get application stylesheet
     * @param theme Theme
     * @return Stylesheet string
     */
    QString getStyleSheet(Theme theme);
    
    /**
     * @brief Load custom stylesheet from file
     * @param path Stylesheet file path
     * @return Stylesheet string
     */
    QString loadStyleSheet(const QString& path);
    
    /**
     * @brief Save window geometry
     * @param window Window
     * @param key Settings key
     */
    void saveWindowGeometry(QWidget* window, const QString& key);
    
    /**
     * @brief Restore window geometry
     * @param window Window
     * @param key Settings key
     */
    void restoreWindowGeometry(QWidget* window, const QString& key);
}