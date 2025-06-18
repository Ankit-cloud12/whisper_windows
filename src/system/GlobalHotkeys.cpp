#include "GlobalHotkeys.h"
#include <iostream>
#include <windows.h>
#include <QCoreApplication> // For qApp

GlobalHotkeys::GlobalHotkeys(QObject* parent)
    : QObject(parent)
    , m_nextHotkeyId(1)
{
    // TODO: Initialize global hotkey system
    // - Set up Windows hook or register hotkeys
    // - Initialize hotkey mapping
    
    if (qApp) { // Ensure qApp is valid before using
        qApp->installNativeEventFilter(this);
    } else {
        std::cerr << "GlobalHotkeys: Error - QApplication instance not available, cannot install native event filter." << std::endl;
        // Consider how to handle this error, e.g., throw or set an error state.
    }

    std::cout << "GlobalHotkeys: Initialized" << std::endl;
}

GlobalHotkeys::~GlobalHotkeys()
{
    // TODO: Clean up global hotkeys
    // - Unregister all hotkeys
    // - Remove hooks
    
    unregisterAllHotkeys();
}

bool GlobalHotkeys::registerHotkey(const QString& id, const QString& combination)
{
    // TODO: Register a global hotkey
    // - Parse key combination string (e.g., "Ctrl+Shift+R")
    // - Convert to Windows virtual key codes
    // - Register with Windows
    // - Store mapping
    
    std::cout << "GlobalHotkeys: Registering hotkey - ID: " << id.toStdString() 
              << ", Combination: " << combination.toStdString() << std::endl;
    
    // Parse modifiers and key
    Qt::KeyboardModifiers modifiers;
    int key = 0;
    
    if (!parseHotkeyString(combination, modifiers, key)) {
        std::cout << "GlobalHotkeys: Failed to parse hotkey combination" << std::endl;
        return false;
    }
    
    // Convert Qt modifiers to Windows modifiers
    UINT winModifiers = 0;
    if (modifiers & Qt::ControlModifier) winModifiers |= MOD_CONTROL;
    if (modifiers & Qt::ShiftModifier) winModifiers |= MOD_SHIFT;
    if (modifiers & Qt::AltModifier) winModifiers |= MOD_ALT;
    
    // Register with Windows
    int hotkeyId = m_nextHotkeyId++;
    
    // TODO: Actually register with Windows
    // BOOL result = RegisterHotKey(NULL, hotkeyId, winModifiers, key);
    
    // Store mapping
    HotkeyInfo info;
    info.id = id;
    info.combination = combination;
    info.windowsId = hotkeyId;
    info.modifiers = modifiers;
    info.key = key;
    
    this->m_hotkeys[id] = info; // Changed to this->m_hotkeys for clarity
    
    return true;
}

bool GlobalHotkeys::unregisterHotkey(const QString& id)
{
    // TODO: Unregister a global hotkey
    // - Find hotkey by ID
    // - Unregister from Windows
    // - Remove from mapping
    
    auto it = this->m_hotkeys.find(id); // Changed to this->m_hotkeys
    if (it == this->m_hotkeys.end()) { // Changed to this->m_hotkeys
        return false;
    }
    
    std::cout << "GlobalHotkeys: Unregistering hotkey - ID: " << id.toStdString() << std::endl;
    
    // TODO: Actually unregister from Windows
    // UnregisterHotKey(NULL, it->second.windowsId);
    
    this->m_hotkeys.erase(it); // Changed to this->m_hotkeys
    return true;
}

void GlobalHotkeys::unregisterAllHotkeys()
{
    // TODO: Unregister all hotkeys
    // - Iterate through all registered hotkeys
    // - Unregister each one
    
    std::cout << "GlobalHotkeys: Unregistering all hotkeys" << std::endl;
    
    for (const auto& pair : this->m_hotkeys) { // Changed to this->m_hotkeys
        // TODO: Actually unregister from Windows
        // UnregisterHotKey(NULL, pair.second.windowsId);
    }
    
    this->m_hotkeys.clear(); // Changed to this->m_hotkeys
}

bool GlobalHotkeys::isHotkeyRegistered(const QString& id) const
{
    // TODO: Check if hotkey is registered
    // - Look up in mapping
    
    return this->m_hotkeys.find(id) != this->m_hotkeys.end(); // Changed to this->m_hotkeys
}

QString GlobalHotkeys::getHotkeyCombination(const QString& id) const
{
    // TODO: Get hotkey combination string
    // - Look up in mapping
    // - Return combination string
    
    auto it = this->m_hotkeys.find(id); // Changed to this->m_hotkeys
    if (it != this->m_hotkeys.end()) { // Changed to this->m_hotkeys
        return it->second.combination;
    }
    
    return QString();
}

QStringList GlobalHotkeys::registeredHotkeys() const
{
    // TODO: Get list of registered hotkey IDs
    // - Return all registered IDs
    
    QStringList ids;
    for (const auto& pair : this->m_hotkeys) { // Changed to this->m_hotkeys
        ids << pair.first;
    }
    
    return ids;
}

bool GlobalHotkeys::parseHotkeyString(const QString& combination, 
                                      Qt::KeyboardModifiers& modifiers, 
                                      int& key)
{
    // TODO: Parse hotkey combination string
    // - Split by '+' delimiter
    // - Identify modifiers (Ctrl, Shift, Alt, Win)
    // - Identify main key
    // - Convert to Qt key codes
    
    modifiers = Qt::NoModifier;
    key = 0;
    
    QStringList parts = combination.split('+', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return false;
    }
    
    // Process all parts
    for (const QString& part : parts) {
        QString trimmed = part.trimmed().toLower();
        
        // Check for modifiers
        if (trimmed == "ctrl" || trimmed == "control") {
            modifiers |= Qt::ControlModifier;
        } else if (trimmed == "shift") {
            modifiers |= Qt::ShiftModifier;
        } else if (trimmed == "alt") {
            modifiers |= Qt::AltModifier;
        } else if (trimmed == "win" || trimmed == "windows" || trimmed == "meta") {
            modifiers |= Qt::MetaModifier;
        } else {
            // This should be the main key
            // Convert to virtual key code
            key = stringToVirtualKey(trimmed);
            if (key == 0) {
                return false;
            }
        }
    }
    
    return key != 0;
}

int GlobalHotkeys::stringToVirtualKey(const QString& keyString)
{
    // TODO: Convert key string to Windows virtual key code
    // - Handle letters (A-Z)
    // - Handle numbers (0-9)
    // - Handle function keys (F1-F12)
    // - Handle special keys
    
    QString key = keyString.toUpper();
    
    // Letters
    if (key.length() == 1 && key[0].isLetter()) {
        return key[0].unicode();
    }
    
    // Numbers
    if (key.length() == 1 && key[0].isDigit()) {
        return key[0].unicode();
    }
    
    // Function keys
    if (key.startsWith("F") && key.length() <= 3) {
        bool ok;
        int fNum = key.mid(1).toInt(&ok);
        if (ok && fNum >= 1 && fNum <= 12) {
            return VK_F1 + (fNum - 1);
        }
    }
    
    // Special keys
    if (key == "SPACE") return VK_SPACE;
    if (key == "ENTER" || key == "RETURN") return VK_RETURN;
    if (key == "TAB") return VK_TAB;
    if (key == "ESCAPE" || key == "ESC") return VK_ESCAPE;
    if (key == "BACKSPACE") return VK_BACK;
    if (key == "DELETE" || key == "DEL") return VK_DELETE;
    if (key == "INSERT" || key == "INS") return VK_INSERT;
    if (key == "HOME") return VK_HOME;
    if (key == "END") return VK_END;
    if (key == "PAGEUP" || key == "PGUP") return VK_PRIOR;
    if (key == "PAGEDOWN" || key == "PGDN") return VK_NEXT;
    if (key == "LEFT") return VK_LEFT;
    if (key == "RIGHT") return VK_RIGHT;
    if (key == "UP") return VK_UP;
    if (key == "DOWN") return VK_DOWN;
    
    return 0;
}

void GlobalHotkeys::handleHotkeyEvent(int windowsHotkeyId)
{
    // TODO: Handle Windows hotkey event
    // - Find hotkey by Windows ID
    // - Emit appropriate signal
    
    for (const auto& pair : this->m_hotkeys) { // Changed to this->m_hotkeys
        if (pair.second.windowsId == windowsHotkeyId) {
            std::cout << "GlobalHotkeys: Hotkey triggered - ID: " << pair.first.toStdString() << std::endl;
            emit hotkeyTriggered(pair.first);
            return;
        }
    }
}

bool GlobalHotkeys::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    // TODO: Process Windows messages for hotkeys
    // - Check for WM_HOTKEY message
    // - Extract hotkey ID
    // - Call handleHotkeyEvent
    
    if (eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        
        if (msg->message == WM_HOTKEY) {
            int hotkeyId = static_cast<int>(msg->wParam);
            handleHotkeyEvent(hotkeyId);
            return true;
        }
    }
    
    return false;
}