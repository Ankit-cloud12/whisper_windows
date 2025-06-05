#include "WindowManager.h"
#include <QWidget>
#include <QApplication>
#include <QScreen>
#include <QSettings>
#include <QDebug>

// Standard library includes
#include <memory>
#include <vector>
#include <string>

// Windows API includes
#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#endif

// Private implementation class
class WindowManager::Impl {
public:
    // Active window tracking
    mutable HWND lastActiveWindow = nullptr;
};

WindowManager::WindowManager(QObject* parent)
    : QObject(parent)
    , pImpl(std::make_unique<Impl>())
{
    qDebug() << "WindowManager: Initialized";
}

WindowManager::~WindowManager() = default;

bool WindowManager::saveWindowState(QWidget* widget, const QString& key)
{
    if (!widget) return false;
    
    QSettings settings;
    settings.beginGroup("WindowStates");
    settings.beginGroup(key);
    
    WindowState state = getWindowState(widget);
    settings.setValue("geometry", state.geometry);
    settings.setValue("maximized", state.is_maximized);
    settings.setValue("fullscreen", state.is_fullscreen);
    settings.setValue("monitor", state.monitor_name);
    settings.setValue("flags", state.window_state_flags);
    
    settings.endGroup();
    settings.endGroup();
    
    return true;
}

bool WindowManager::restoreWindowState(QWidget* widget, const QString& key)
{
    if (!widget) return false;
    
    QSettings settings;
    settings.beginGroup("WindowStates");
    settings.beginGroup(key);
    
    if (!settings.contains("geometry")) {
        settings.endGroup();
        settings.endGroup();
        return false;
    }
    
    WindowState state;
    state.geometry = settings.value("geometry").toRect();
    state.is_maximized = settings.value("maximized").toBool();
    state.is_fullscreen = settings.value("fullscreen").toBool();
    state.monitor_name = settings.value("monitor").toString();
    state.window_state_flags = settings.value("flags").toInt();
    
    settings.endGroup();
    settings.endGroup();
    
    setWindowState(widget, state);
    return true;
}

WindowState WindowManager::getWindowState(QWidget* widget) const
{
    WindowState state;
    if (!widget) return state;
    
    state.geometry = widget->geometry();
    state.is_maximized = widget->isMaximized();
    state.is_fullscreen = widget->isFullScreen();
    state.window_state_flags = widget->windowState();
    
    // Get monitor info
    if (QScreen* screen = widget->screen()) {
        state.monitor_name = screen->name();
    }
    
    return state;
}

void WindowManager::setWindowState(QWidget* widget, const WindowState& state)
{
    if (!widget) return;
    
    // Set geometry first
    widget->setGeometry(state.geometry);
    
    // Apply window state
    if (state.is_maximized) {
        widget->showMaximized();
    } else if (state.is_fullscreen) {
        widget->showFullScreen();
    } else {
        widget->showNormal();
    }
    
    emit windowStateChanged(widget, state);
}

WindowInfo WindowManager::getActiveWindow() const
{
    HWND hwnd = GetForegroundWindow();
    
    if (hwnd != pImpl->lastActiveWindow) {
        pImpl->lastActiveWindow = hwnd;
        WindowInfo info = getWindowInfo(hwnd);
        const_cast<WindowManager*>(this)->emit activeWindowChanged(info);
    }
    
    return getWindowInfo(hwnd);
}

WindowInfo WindowManager::getWindowInfo(HWND hwnd) const
{
    WindowInfo info;
    info.handle = hwnd;
    
    if (!hwnd) return info;
    
    // Get title
    info.title = getWindowText(hwnd);
    
    // Get class name
    info.class_name = getWindowClassName(hwnd);
    
    // Get process info
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    info.process_id = processId;
    
    // Get window rect
    RECT rect;
    if (GetWindowRect(hwnd, &rect)) {
        info.geometry = rectToQRect(rect);
    }
    
    // Get window state
    info.is_minimized = IsIconic(hwnd);
    info.is_visible = IsWindowVisible(hwnd);
    
    return info;
}

std::vector<WindowInfo> WindowManager::getVisibleWindows() const
{
    std::vector<WindowInfo> windows;
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&windows));
    return windows;
}

std::vector<WindowInfo> WindowManager::findWindowsByTitle(const QString& title_pattern) const
{
    std::vector<WindowInfo> allWindows = getVisibleWindows();
    std::vector<WindowInfo> matches;
    
    for (const auto& window : allWindows) {
        if (window.title.contains(title_pattern, Qt::CaseInsensitive)) {
            matches.push_back(window);
        }
    }
    
    return matches;
}

std::vector<WindowInfo> WindowManager::findWindowsByClass(const QString& class_name) const
{
    std::vector<WindowInfo> allWindows = getVisibleWindows();
    std::vector<WindowInfo> matches;
    
    for (const auto& window : allWindows) {
        if (window.class_name.compare(class_name, Qt::CaseInsensitive) == 0) {
            matches.push_back(window);
        }
    }
    
    return matches;
}

void WindowManager::setAlwaysOnTop(QWidget* widget, bool on_top)
{
    if (!widget) return;
    
    widget->setWindowFlag(Qt::WindowStaysOnTopHint, on_top);
    widget->show(); // Apply the flag change
}

bool WindowManager::isAlwaysOnTop(QWidget* widget) const
{
    if (!widget) return false;
    return widget->windowFlags() & Qt::WindowStaysOnTopHint;
}

void WindowManager::centerOnScreen(QWidget* widget, QScreen* screen)
{
    if (!widget) return;
    
    if (!screen) {
        screen = QApplication::primaryScreen();
    }
    
    QRect screenGeometry = screen->availableGeometry();
    widget->move(screenGeometry.center() - widget->rect().center());
}

void WindowManager::ensureVisible(QWidget* widget)
{
    if (!widget) return;
    
    QRect widgetRect = widget->geometry();
    QScreen* currentScreen = nullptr;
    
    // Find which screen contains the widget
    for (QScreen* screen : QApplication::screens()) {
        if (screen->geometry().contains(widgetRect.center())) {
            currentScreen = screen;
            break;
        }
    }
    
    // If not on any screen, move to primary
    if (!currentScreen) {
        currentScreen = QApplication::primaryScreen();
        centerOnScreen(widget, currentScreen);
        return;
    }
    
    // Ensure widget is within screen bounds
    QRect availableGeometry = currentScreen->availableGeometry();
    if (!availableGeometry.contains(widgetRect)) {
        widgetRect.moveLeft(qMax(availableGeometry.left(), 
                                 qMin(widgetRect.left(), 
                                      availableGeometry.right() - widgetRect.width())));
        widgetRect.moveTop(qMax(availableGeometry.top(), 
                                qMin(widgetRect.top(), 
                                     availableGeometry.bottom() - widgetRect.height())));
        widget->move(widgetRect.topLeft());
    }
}

std::vector<MonitorInfo> WindowManager::getMonitors() const
{
    if (monitors_cached) {
        return cached_monitors;
    }
    
    cached_monitors.clear();
    EnumDisplayMonitors(nullptr, nullptr, enumMonitorsProc, 
                       reinterpret_cast<LPARAM>(&cached_monitors));
    monitors_cached = true;
    
    return cached_monitors;
}

MonitorInfo WindowManager::getMonitorAt(const QPoint& point) const
{
    POINT pt = {point.x(), point.y()};
    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfo(hMonitor, &mi)) {
        MonitorInfo info;
        info.geometry = rectToQRect(mi.rcMonitor);
        info.available_geometry = rectToQRect(mi.rcWork);
        info.is_primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
        return info;
    }
    
    // Return primary monitor info as fallback
    MonitorInfo primary;
    primary.geometry = QApplication::primaryScreen()->geometry();
    primary.available_geometry = QApplication::primaryScreen()->availableGeometry();
    primary.is_primary = true;
    return primary;
}

MonitorInfo WindowManager::getMonitorForWindow(QWidget* widget) const
{
    if (!widget) {
        return getMonitorAt(QPoint(0, 0));
    }
    
    return getMonitorAt(widget->geometry().center());
}

void WindowManager::bringToFront(QWidget* widget, bool force)
{
    if (!widget) return;
    
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    
    if (force) {
        // Force window to front
        DWORD currentThreadId = GetCurrentThreadId();
        DWORD foregroundThreadId = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
        
        if (currentThreadId != foregroundThreadId) {
            AttachThreadInput(currentThreadId, foregroundThreadId, TRUE);
            SetForegroundWindow(hwnd);
            AttachThreadInput(currentThreadId, foregroundThreadId, FALSE);
        } else {
            SetForegroundWindow(hwnd);
        }
    } else {
        // Normal activation
        SetForegroundWindow(hwnd);
    }
    
    widget->raise();
    widget->activateWindow();
}

void WindowManager::flashWindow(QWidget* widget, int count)
{
    if (!widget) return;
    
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    
    FLASHWINFO fi;
    fi.cbSize = sizeof(FLASHWINFO);
    fi.hwnd = hwnd;
    fi.dwFlags = FLASHW_ALL | (count == 0 ? FLASHW_TIMERNOFG : FLASHW_CAPTION);
    fi.uCount = count;
    fi.dwTimeout = 0;
    
    FlashWindowEx(&fi);
}

void WindowManager::setWindowOpacity(QWidget* widget, qreal opacity)
{
    if (!widget) return;
    widget->setWindowOpacity(opacity);
}

void WindowManager::setWindowShadow(QWidget* widget, bool enabled)
{
    if (!widget) return;
    
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    if (enabled) {
        SetClassLongPtr(hwnd, GCL_STYLE, GetClassLongPtr(hwnd, GCL_STYLE) | CS_DROPSHADOW);
    } else {
        SetClassLongPtr(hwnd, GCL_STYLE, GetClassLongPtr(hwnd, GCL_STYLE) & ~CS_DROPSHADOW);
    }
#endif
}

void WindowManager::setClickThrough(QWidget* widget, bool click_through)
{
    if (!widget) return;
    
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    
    if (click_through) {
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
    } else {
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
    }
#endif
}

void WindowManager::saveGeometry(QWidget* widget, const QString& key)
{
    if (!widget) return;
    
    QSettings settings;
    settings.setValue(key + "/geometry", widget->saveGeometry());
}

bool WindowManager::restoreGeometry(QWidget* widget, const QString& key)
{
    if (!widget) return false;

    QSettings settings;
    QByteArray geometry = settings.value(key + "/geometry").toByteArray();
    if (geometry.isEmpty()) return false;

    return widget->restoreGeometry(geometry);
}

bool WindowManager::typeText(const std::wstring& text)
{
    if (text.empty()) return false;

#ifdef _WIN32
    // Get the currently active window
    HWND activeWindow = GetForegroundWindow();
    if (!activeWindow) return false;

    // Convert wide string to input events
    std::vector<INPUT> inputs;
    inputs.reserve(text.length() * 2); // Each character needs key down and key up

    for (wchar_t ch : text) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = 0;
        input.ki.wScan = ch;
        input.ki.dwFlags = KEYEVENTF_UNICODE;
        input.ki.time = 0;
        input.ki.dwExtraInfo = 0;

        // Key down
        inputs.push_back(input);

        // Key up
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
        inputs.push_back(input);
    }

    // Send the input events
    UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));

    return sent == inputs.size();
#else
    // Non-Windows platforms not supported
    return false;
#endif
}

// Static callbacks
BOOL CALLBACK WindowManager::enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    std::vector<WindowInfo>* windows = reinterpret_cast<std::vector<WindowInfo>*>(lParam);
    
    // Skip invisible windows
    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }
    
    // Skip windows without title
    WCHAR buffer[256];
    int length = GetWindowTextW(hwnd, buffer, 256);
    if (length == 0) {
        return TRUE;
    }
    
    // Get window info using a temporary WindowManager instance
    WindowManager temp(nullptr);
    WindowInfo info = temp.getWindowInfo(hwnd);
    
    windows->push_back(info);
    
    return TRUE;  // Continue enumeration
}

BOOL CALLBACK WindowManager::enumMonitorsProc(HMONITOR monitor, HDC hdc, 
                                            LPRECT rect, LPARAM data)
{
    std::vector<MonitorInfo>* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(data);
    
    MONITORINFOEX mi;
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfo(monitor, &mi)) {
        return TRUE;
    }
    
    MonitorInfo info;
    info.name = QString::fromWCharArray(mi.szDevice);
    info.geometry = QRect(mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top);
    info.available_geometry = QRect(mi.rcWork.left, mi.rcWork.top,
                                   mi.rcWork.right - mi.rcWork.left,
                                   mi.rcWork.bottom - mi.rcWork.top);
    info.is_primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
    
    // Get DPI scale
    HDC screenDC = GetDC(nullptr);
    int logicalDpi = GetDeviceCaps(screenDC, LOGPIXELSX);
    ReleaseDC(nullptr, screenDC);
    info.dpi_scale = logicalDpi / 96.0f;
    
    monitors->push_back(info);
    
    return TRUE;  // Continue enumeration
}

QString WindowManager::getWindowText(HWND hwnd) const
{
    if (!hwnd) return QString();
    
    int length = GetWindowTextLengthW(hwnd);
    if (length == 0) return QString();
    
    std::vector<WCHAR> buffer(length + 1);
    GetWindowTextW(hwnd, buffer.data(), length + 1);
    
    return QString::fromWCharArray(buffer.data());
}

QString WindowManager::getWindowClassName(HWND hwnd) const
{
    if (!hwnd) return QString();
    
    const int bufferSize = 256;
    WCHAR buffer[bufferSize];
    int length = GetClassNameW(hwnd, buffer, bufferSize);
    
    if (length > 0) {
        return QString::fromWCharArray(buffer, length);
    }
    
    return QString();
}

QRect WindowManager::rectToQRect(const RECT& rect) const
{
    return QRect(rect.left, rect.top, 
                 rect.right - rect.left, 
                 rect.bottom - rect.top);
}

QRect WindowManager::applyDpiScaling(const QRect& geometry, float dpi_scale) const
{
    return QRect(geometry.x() * dpi_scale,
                 geometry.y() * dpi_scale,
                 geometry.width() * dpi_scale,
                 geometry.height() * dpi_scale);
}