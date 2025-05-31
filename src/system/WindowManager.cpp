#include "WindowManager.h"
#include <iostream>
#include <windows.h>
#include <QDebug>

WindowManager::WindowManager(QObject* parent)
    : QObject(parent)
    , m_activeWindow(nullptr)
{
    // TODO: Initialize window manager
    // - Set up Windows hooks if needed
    // - Initialize window tracking
    
    std::cout << "WindowManager: Initialized" << std::endl;
}

WindowManager::~WindowManager()
{
    // TODO: Clean up resources
    // - Remove hooks
    // - Clean up window handles
}

HWND WindowManager::getActiveWindow() const
{
    // TODO: Get currently active window
    // - Use Windows API to get foreground window
    // - Cache result for performance
    
    HWND hwnd = GetForegroundWindow();
    if (hwnd != m_activeWindow) {
        m_activeWindow = hwnd;
        const_cast<WindowManager*>(this)->emit activeWindowChanged(hwnd);
    }
    
    return hwnd;
}

QString WindowManager::getActiveWindowTitle() const
{
    // TODO: Get title of active window
    // - Get window handle
    // - Retrieve window text
    // - Handle Unicode properly
    
    HWND hwnd = getActiveWindow();
    if (!hwnd) {
        return QString();
    }
    
    const int bufferSize = 256;
    WCHAR buffer[bufferSize];
    int length = GetWindowTextW(hwnd, buffer, bufferSize);
    
    if (length > 0) {
        return QString::fromWCharArray(buffer, length);
    }
    
    return QString();
}

QString WindowManager::getActiveWindowClass() const
{
    // TODO: Get class name of active window
    // - Get window handle
    // - Retrieve window class
    // - Useful for identifying application type
    
    HWND hwnd = getActiveWindow();
    if (!hwnd) {
        return QString();
    }
    
    const int bufferSize = 256;
    WCHAR buffer[bufferSize];
    int length = GetClassNameW(hwnd, buffer, bufferSize);
    
    if (length > 0) {
        return QString::fromWCharArray(buffer, length);
    }
    
    return QString();
}

QString WindowManager::getActiveProcessName() const
{
    // TODO: Get process name of active window
    // - Get window handle
    // - Get process ID
    // - Get process name from ID
    
    HWND hwnd = getActiveWindow();
    if (!hwnd) {
        return QString();
    }
    
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) {
        return QString();
    }
    
    WCHAR processName[MAX_PATH];
    DWORD size = MAX_PATH;
    
    QString result;
    if (QueryFullProcessImageNameW(hProcess, 0, processName, &size)) {
        result = QString::fromWCharArray(processName);
        // Extract just the executable name
        result = result.mid(result.lastIndexOf('\\') + 1);
    }
    
    CloseHandle(hProcess);
    return result;
}

void WindowManager::sendTextToActiveWindow(const QString& text)
{
    // TODO: Send text to active window
    // - Get active window
    // - Send keystrokes to simulate typing
    // - Handle special characters and Unicode
    
    HWND hwnd = getActiveWindow();
    if (!hwnd) {
        std::cout << "WindowManager: No active window to send text to" << std::endl;
        return;
    }
    
    std::cout << "WindowManager: Sending text to active window: " << text.toStdString() << std::endl;
    
    // Ensure window has focus
    SetForegroundWindow(hwnd);
    Sleep(100);  // Brief delay to ensure focus
    
    // Send each character
    for (const QChar& ch : text) {
        sendCharacter(ch);
    }
    
    emit textSent(text);
}

void WindowManager::sendKeyPress(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    // TODO: Send single key press to active window
    // - Convert Qt key to Windows virtual key
    // - Apply modifiers
    // - Send key down/up events
    
    HWND hwnd = getActiveWindow();
    if (!hwnd) {
        return;
    }
    
    std::cout << "WindowManager: Sending key press" << std::endl;
    
    // Convert Qt key to Windows VK
    WORD vk = qtKeyToVirtualKey(key);
    if (vk == 0) {
        return;
    }
    
    // Apply modifiers
    if (modifiers & Qt::ControlModifier) {
        keybd_event(VK_CONTROL, 0, 0, 0);
    }
    if (modifiers & Qt::ShiftModifier) {
        keybd_event(VK_SHIFT, 0, 0, 0);
    }
    if (modifiers & Qt::AltModifier) {
        keybd_event(VK_MENU, 0, 0, 0);
    }
    
    // Send key
    keybd_event(vk, 0, 0, 0);
    keybd_event(vk, 0, KEYEVENTF_KEYUP, 0);
    
    // Release modifiers
    if (modifiers & Qt::AltModifier) {
        keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    }
    if (modifiers & Qt::ShiftModifier) {
        keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
    }
    if (modifiers & Qt::ControlModifier) {
        keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    }
}

void WindowManager::bringWindowToFront(HWND window)
{
    // TODO: Bring specific window to front
    // - Use Windows API to activate window
    // - Handle minimized windows
    
    if (!window) {
        return;
    }
    
    std::cout << "WindowManager: Bringing window to front" << std::endl;
    
    // Restore if minimized
    if (IsIconic(window)) {
        ShowWindow(window, SW_RESTORE);
    }
    
    // Bring to front
    SetForegroundWindow(window);
}

QList<WindowInfo> WindowManager::getAllWindows() const
{
    // TODO: Get list of all windows
    // - Enumerate all top-level windows
    // - Filter out invisible/system windows
    // - Return window information
    
    QList<WindowInfo> windows;
    
    // Use static callback function
    EnumWindows(WindowManager::enumWindowsCallback, reinterpret_cast<LPARAM>(&windows));
    
    return windows;
}

WindowInfo WindowManager::getWindowInfo(HWND window) const
{
    // TODO: Get detailed window information
    // - Get window title, class, process
    // - Get window position and size
    // - Get window state
    
    WindowInfo info;
    info.handle = window;
    
    if (!window) {
        return info;
    }
    
    // Get title
    const int bufferSize = 256;
    WCHAR buffer[bufferSize];
    int length = GetWindowTextW(window, buffer, bufferSize);
    if (length > 0) {
        info.title = QString::fromWCharArray(buffer, length);
    }
    
    // Get class name
    length = GetClassNameW(window, buffer, bufferSize);
    if (length > 0) {
        info.className = QString::fromWCharArray(buffer, length);
    }
    
    // Get process info
    DWORD processId;
    GetWindowThreadProcessId(window, &processId);
    info.processId = processId;
    
    // Get window rect
    RECT rect;
    if (GetWindowRect(window, &rect)) {
        info.x = rect.left;
        info.y = rect.top;
        info.width = rect.right - rect.left;
        info.height = rect.bottom - rect.top;
    }
    
    // Get window state
    info.isMinimized = IsIconic(window);
    info.isMaximized = IsZoomed(window);
    info.isVisible = IsWindowVisible(window);
    
    return info;
}

void WindowManager::monitorActiveWindow(bool enable)
{
    // TODO: Enable/disable active window monitoring
    // - Set up timer or hook to track window changes
    // - Emit signals when active window changes
    
    if (enable) {
        std::cout << "WindowManager: Starting active window monitoring" << std::endl;
        // TODO: Start monitoring
    } else {
        std::cout << "WindowManager: Stopping active window monitoring" << std::endl;
        // TODO: Stop monitoring
    }
}

void WindowManager::sendCharacter(const QChar& ch)
{
    // TODO: Send single character to active window
    // - Handle Unicode characters
    // - Use SendInput for better compatibility
    
    INPUT input[2] = {0};
    
    // Key down
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wScan = ch.unicode();
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;
    
    // Key up
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wScan = ch.unicode();
    input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    
    SendInput(2, input, sizeof(INPUT));
    
    // Small delay between characters for reliability
    Sleep(10);
}

WORD WindowManager::qtKeyToVirtualKey(Qt::Key key) const
{
    // TODO: Convert Qt key code to Windows virtual key
    // - Map common keys
    // - Return 0 for unsupported keys
    
    switch (key) {
        case Qt::Key_A: return 'A';
        case Qt::Key_B: return 'B';
        case Qt::Key_C: return 'C';
        case Qt::Key_D: return 'D';
        case Qt::Key_E: return 'E';
        case Qt::Key_F: return 'F';
        case Qt::Key_G: return 'G';
        case Qt::Key_H: return 'H';
        case Qt::Key_I: return 'I';
        case Qt::Key_J: return 'J';
        case Qt::Key_K: return 'K';
        case Qt::Key_L: return 'L';
        case Qt::Key_M: return 'M';
        case Qt::Key_N: return 'N';
        case Qt::Key_O: return 'O';
        case Qt::Key_P: return 'P';
        case Qt::Key_Q: return 'Q';
        case Qt::Key_R: return 'R';
        case Qt::Key_S: return 'S';
        case Qt::Key_T: return 'T';
        case Qt::Key_U: return 'U';
        case Qt::Key_V: return 'V';
        case Qt::Key_W: return 'W';
        case Qt::Key_X: return 'X';
        case Qt::Key_Y: return 'Y';
        case Qt::Key_Z: return 'Z';
        
        case Qt::Key_0: return '0';
        case Qt::Key_1: return '1';
        case Qt::Key_2: return '2';
        case Qt::Key_3: return '3';
        case Qt::Key_4: return '4';
        case Qt::Key_5: return '5';
        case Qt::Key_6: return '6';
        case Qt::Key_7: return '7';
        case Qt::Key_8: return '8';
        case Qt::Key_9: return '9';
        
        case Qt::Key_Space: return VK_SPACE;
        case Qt::Key_Return: return VK_RETURN;
        case Qt::Key_Tab: return VK_TAB;
        case Qt::Key_Backspace: return VK_BACK;
        case Qt::Key_Delete: return VK_DELETE;
        case Qt::Key_Escape: return VK_ESCAPE;
        
        case Qt::Key_Left: return VK_LEFT;
        case Qt::Key_Right: return VK_RIGHT;
        case Qt::Key_Up: return VK_UP;
        case Qt::Key_Down: return VK_DOWN;
        
        case Qt::Key_Home: return VK_HOME;
        case Qt::Key_End: return VK_END;
        case Qt::Key_PageUp: return VK_PRIOR;
        case Qt::Key_PageDown: return VK_NEXT;
        
        case Qt::Key_F1: return VK_F1;
        case Qt::Key_F2: return VK_F2;
        case Qt::Key_F3: return VK_F3;
        case Qt::Key_F4: return VK_F4;
        case Qt::Key_F5: return VK_F5;
        case Qt::Key_F6: return VK_F6;
        case Qt::Key_F7: return VK_F7;
        case Qt::Key_F8: return VK_F8;
        case Qt::Key_F9: return VK_F9;
        case Qt::Key_F10: return VK_F10;
        case Qt::Key_F11: return VK_F11;
        case Qt::Key_F12: return VK_F12;
        
        default: return 0;
    }
}

BOOL CALLBACK WindowManager::enumWindowsCallback(HWND hwnd, LPARAM lParam)
{
    // TODO: Callback for window enumeration
    // - Filter windows
    // - Add to list
    
    QList<WindowInfo>* windows = reinterpret_cast<QList<WindowInfo>*>(lParam);
    
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
    
    // Create window info
    WindowInfo info;
    info.handle = hwnd;
    info.title = QString::fromWCharArray(buffer, length);
    
    // Get additional info
    length = GetClassNameW(hwnd, buffer, 256);
    if (length > 0) {
        info.className = QString::fromWCharArray(buffer, length);
    }
    
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    info.processId = processId;
    
    RECT rect;
    if (GetWindowRect(hwnd, &rect)) {
        info.x = rect.left;
        info.y = rect.top;
        info.width = rect.right - rect.left;
        info.height = rect.bottom - rect.top;
    }
    
    info.isMinimized = IsIconic(hwnd);
    info.isMaximized = IsZoomed(hwnd);
    info.isVisible = true;  // We already filtered invisible windows
    
    windows->append(info);
    
    return TRUE;  // Continue enumeration
}