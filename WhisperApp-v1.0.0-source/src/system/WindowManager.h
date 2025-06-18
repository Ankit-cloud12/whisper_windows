/*
 * WindowManager.h
 * 
 * Window management utilities for Windows.
 * Handles window state, positioning, and focus management.
 * 
 * Features:
 * - Window state persistence
 * - Multi-monitor support
 * - Active window detection
 * - Window positioning
 * - Focus management
 */

#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#ifdef _WIN32
#include <windows.h> // Moved to the top
#endif

#include <QObject>
#include <QRect>
#include <QString>
#include <memory>
#include <vector>
#include <string>

// Windows types - a second include guard is fine, or remove this one
// #ifdef _WIN32
// #include <windows.h>
// #endif

// Forward declarations
class QWidget;
class QScreen;

/**
 * @brief Window information
 */
struct WindowInfo {
    HWND handle = nullptr;              // Window handle
    QString title;                      // Window title
    QString class_name;                 // Window class
    int process_id = 0;                 // Process ID
    bool is_visible = false;            // Is visible
    bool is_minimized = false;          // Is minimized
    QRect geometry;                     // Window geometry
};

/**
 * @brief Monitor information
 */
struct MonitorInfo {
    QString name;                       // Monitor name
    QRect geometry;                     // Total geometry
    QRect available_geometry;           // Available geometry (excluding taskbar)
    bool is_primary = false;            // Is primary monitor
    float dpi_scale = 1.0f;            // DPI scaling factor
};

/**
 * @brief Window state for persistence
 */
struct WindowState {
    QRect geometry;                     // Window position and size
    bool is_maximized = false;          // Was maximized
    bool is_fullscreen = false;         // Was fullscreen
    QString monitor_name;               // Monitor name
    int window_state_flags = 0;         // Qt window state flags
};

/**
 * @brief Window manager utilities
 */
class WindowManager : public QObject {
    Q_OBJECT

public:
    explicit WindowManager(QObject* parent = nullptr);
    ~WindowManager();

    /**
     * @brief Save window state
     * @param widget Widget to save state for
     * @param key Unique key for this window
     * @return true if saved successfully
     */
    bool saveWindowState(QWidget* widget, const QString& key);

    /**
     * @brief Restore window state
     * @param widget Widget to restore state for
     * @param key Unique key for this window
     * @return true if restored successfully
     */
    bool restoreWindowState(QWidget* widget, const QString& key);

    /**
     * @brief Get current window state
     * @param widget Widget to get state for
     * @return Window state
     */
    WindowState getWindowState(QWidget* widget) const;

    /**
     * @brief Set window state
     * @param widget Widget to set state for
     * @param state Window state to apply
     */
    void setWindowState(QWidget* widget, const WindowState& state);

    /**
     * @brief Get active window information
     * @return Active window info
     */
    WindowInfo getActiveWindow() const;

    /**
     * @brief Get window information by handle
     * @param hwnd Window handle
     * @return Window info
     */
    WindowInfo getWindowInfo(HWND hwnd) const;

    /**
     * @brief Get all visible windows
     * @return Vector of visible windows
     */
    std::vector<WindowInfo> getVisibleWindows() const;

    /**
     * @brief Find windows by title
     * @param title_pattern Title pattern (supports wildcards)
     * @return Vector of matching windows
     */
    std::vector<WindowInfo> findWindowsByTitle(const QString& title_pattern) const;

    /**
     * @brief Find windows by class
     * @param class_name Window class name
     * @return Vector of matching windows
     */
    std::vector<WindowInfo> findWindowsByClass(const QString& class_name) const;

    /**
     * @brief Set window always on top
     * @param widget Widget to modify
     * @param on_top true to set on top, false to remove
     */
    void setAlwaysOnTop(QWidget* widget, bool on_top);

    /**
     * @brief Check if window is always on top
     * @param widget Widget to check
     * @return true if always on top
     */
    bool isAlwaysOnTop(QWidget* widget) const;

    /**
     * @brief Center window on screen
     * @param widget Widget to center
     * @param screen Target screen (nullptr = primary)
     */
    void centerOnScreen(QWidget* widget, QScreen* screen = nullptr);

    /**
     * @brief Ensure window is visible on screen
     * @param widget Widget to check
     */
    void ensureVisible(QWidget* widget);

    /**
     * @brief Get available monitors
     * @return Vector of monitor information
     */
    std::vector<MonitorInfo> getMonitors() const;

    /**
     * @brief Get monitor containing point
     * @param point Screen point
     * @return Monitor info (primary if not found)
     */
    MonitorInfo getMonitorAt(const QPoint& point) const;

    /**
     * @brief Get monitor containing window
     * @param widget Widget to check
     * @return Monitor info
     */
    MonitorInfo getMonitorForWindow(QWidget* widget) const;

    /**
     * @brief Bring window to front
     * @param widget Widget to bring forward
     * @param force Force activation
     */
    void bringToFront(QWidget* widget, bool force = false);

    /**
     * @brief Flash window in taskbar
     * @param widget Widget to flash
     * @param count Flash count (0 = until focused)
     */
    void flashWindow(QWidget* widget, int count = 3);

    /**
     * @brief Set window transparency
     * @param widget Widget to modify
     * @param opacity Opacity (0.0 - 1.0)
     */
    void setWindowOpacity(QWidget* widget, qreal opacity);

    /**
     * @brief Enable or disable window shadow
     * @param widget Widget to modify
     * @param enabled true to enable shadow
     */
    void setWindowShadow(QWidget* widget, bool enabled);

    /**
     * @brief Set window to click-through
     * @param widget Widget to modify
     * @param click_through true to enable click-through
     */
    void setClickThrough(QWidget* widget, bool click_through);

    /**
     * @brief Save window geometry to settings
     * @param widget Widget to save
     * @param key Settings key
     */
    void saveGeometry(QWidget* widget, const QString& key);

    /**
     * @brief Restore window geometry from settings
     * @param widget Widget to restore
     * @param key Settings key
     * @return true if restored
     */
    bool restoreGeometry(QWidget* widget, const QString& key);

    /**
     * @brief Type text into the active window
     * @param text Text to type (wide string for Windows API)
     * @return true if successful
     */
    bool typeText(const std::wstring& text);

signals:
    /**
     * @brief Emitted when active window changes
     * @param info New active window info
     */
    void activeWindowChanged(const WindowInfo& info);

    /**
     * @brief Emitted when monitor configuration changes
     */
    void monitorsChanged();

    /**
     * @brief Emitted when window state changes
     * @param widget Widget that changed
     * @param state New state
     */
    void windowStateChanged(QWidget* widget, const WindowState& state);

private:
    /**
     * @brief Window enumeration callback
     */
    static BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam);

    /**
     * @brief Monitor enumeration callback
     */
    static BOOL CALLBACK enumMonitorsProc(HMONITOR monitor, 
                                         HDC hdc, 
                                         LPRECT rect, 
                                         LPARAM data);

    /**
     * @brief Get window text safely
     * @param hwnd Window handle
     * @return Window text
     */
    QString getWindowText(HWND hwnd) const;

    /**
     * @brief Get window class name
     * @param hwnd Window handle
     * @return Class name
     */
    QString getWindowClassName(HWND hwnd) const;

    /**
     * @brief Convert RECT to QRect
     * @param rect Windows RECT
     * @return Qt QRect
     */
    QRect rectToQRect(const RECT& rect) const;

    /**
     * @brief Apply DPI scaling to geometry
     * @param geometry Original geometry
     * @param dpi_scale DPI scale factor
     * @return Scaled geometry
     */
    QRect applyDpiScaling(const QRect& geometry, float dpi_scale) const;

private:
    // Private implementation
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    // Cached data
    mutable std::vector<MonitorInfo> cached_monitors;
    mutable bool monitors_cached = false;
};

#endif // WINDOWMANAGER_H