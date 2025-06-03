/*
 * GlobalHotkeys.h
 * 
 * Global hotkey management for Windows.
 * Handles system-wide keyboard shortcuts for application control.
 * 
 * Features:
 * - Windows hotkey registration
 * - Conflict detection
 * - Push-to-talk support
 * - Customizable key combinations
 * - Multi-hotkey management
 */

#ifndef GLOBALHOTKEYS_H
#define GLOBALHOTKEYS_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QString>
#include <QKeySequence>
#include <memory>
#include <map>
#include <functional>
#include <vector>

// Windows types
#ifdef _WIN32
#include <windows.h>
#endif

/**
 * @brief Hotkey action identifiers
 */
enum class HotkeyAction {
    StartStopRecording,     // Toggle recording
    PushToTalk,            // Hold to record
    InsertText,            // Insert transcribed text
    ClearTranscription,    // Clear current transcription
    ShowHideWindow,        // Toggle window visibility
    CancelRecording,       // Cancel ongoing recording
    Custom                 // User-defined action
};

/**
 * @brief Hotkey information
 */
struct HotkeyInfo {
    HotkeyAction action;           // Action identifier
    QKeySequence key_sequence;     // Key combination
    QString description;           // Human-readable description
    bool is_enabled = true;        // Is hotkey enabled
    bool is_registered = false;    // Is currently registered
    int id = -1;                   // Windows hotkey ID
};

/**
 * @brief Global hotkey manager
 */
class GlobalHotkeys : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    explicit GlobalHotkeys(QObject* parent = nullptr);
    ~GlobalHotkeys();

    /**
     * @brief Initialize hotkey system
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Shutdown hotkey system
     */
    void shutdown();

    /**
     * @brief Register a hotkey
     * @param action Action identifier
     * @param key_sequence Key combination
     * @param description Action description
     * @return true if registered successfully
     */
    bool registerHotkey(HotkeyAction action,
                       const QKeySequence& key_sequence,
                       const QString& description = QString());

    /**
     * @brief Unregister a hotkey
     * @param action Action identifier
     * @return true if unregistered successfully
     */
    bool unregisterHotkey(HotkeyAction action);

    /**
     * @brief Unregister all hotkeys
     */
    void unregisterAllHotkeys();

    /**
     * @brief Update hotkey
     * @param action Action identifier
     * @param new_key_sequence New key combination
     * @return true if updated successfully
     */
    bool updateHotkey(HotkeyAction action,
                     const QKeySequence& new_key_sequence);

    /**
     * @brief Enable or disable a hotkey
     * @param action Action identifier
     * @param enabled true to enable, false to disable
     */
    void setHotkeyEnabled(HotkeyAction action, bool enabled);

    /**
     * @brief Check if hotkey is enabled
     * @param action Action identifier
     * @return true if enabled
     */
    bool isHotkeyEnabled(HotkeyAction action) const;

    /**
     * @brief Get all registered hotkeys
     * @return Vector of hotkey information
     */
    std::vector<HotkeyInfo> getRegisteredHotkeys() const;

    /**
     * @brief Get hotkey for action
     * @param action Action identifier
     * @return Hotkey info (empty if not found)
     */
    HotkeyInfo getHotkey(HotkeyAction action) const;

    /**
     * @brief Check for hotkey conflicts
     * @param key_sequence Key combination to check
     * @return Action that conflicts (nullopt if no conflict)
     */
    std::optional<HotkeyAction> checkConflict(const QKeySequence& key_sequence) const;

    /**
     * @brief Convert QKeySequence to Windows modifiers and virtual key
     * @param key_sequence Qt key sequence
     * @param modifiers Output Windows modifiers
     * @param vk Output virtual key code
     * @return true if conversion successful
     */
    static bool toWindowsHotkey(const QKeySequence& key_sequence,
                               UINT& modifiers,
                               UINT& vk);

    /**
     * @brief Convert Windows hotkey to QKeySequence
     * @param modifiers Windows modifiers
     * @param vk Virtual key code
     * @return Qt key sequence
     */
    static QKeySequence fromWindowsHotkey(UINT modifiers, UINT vk);

    /**
     * @brief Check if system supports global hotkeys
     * @return true if supported
     */
    static bool isSupported();

    /**
     * @brief Get list of reserved system hotkeys
     * @return Vector of reserved key sequences
     */
    static std::vector<QKeySequence> getReservedHotkeys();

signals:
    /**
     * @brief Emitted when a hotkey is triggered
     * @param action Action that was triggered
     */
    void hotkeyTriggered(HotkeyAction action);

    /**
     * @brief Emitted when push-to-talk is pressed
     */
    void pushToTalkPressed();

    /**
     * @brief Emitted when push-to-talk is released
     */
    void pushToTalkReleased();

    /**
     * @brief Emitted when hotkey registration fails
     * @param action Action that failed
     * @param reason Failure reason
     */
    void registrationFailed(HotkeyAction action, const QString& reason);

protected:
    /**
     * @brief Process Windows messages
     * @param message Windows message
     * @return true if message was handled
     */
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    /**
     * @brief Register hotkey with Windows
     * @param info Hotkey information
     * @return true if successful
     */
    bool registerWindowsHotkey(HotkeyInfo& info);

    /**
     * @brief Unregister hotkey with Windows
     * @param info Hotkey information
     * @return true if successful
     */
    bool unregisterWindowsHotkey(HotkeyInfo& info);

    /**
     * @brief Generate unique hotkey ID
     * @return Unique ID
     */
    int generateHotkeyId();

    /**
     * @brief Handle hotkey event
     * @param id Hotkey ID
     */
    void handleHotkeyEvent(int id);

    /**
     * @brief Check if key combination is valid
     * @param key_sequence Key sequence to check
     * @return true if valid
     */
    bool isValidKeyCombination(const QKeySequence& key_sequence) const;

    /**
     * @brief Get error message for last Windows error
     * @return Error message
     */
    QString getLastErrorMessage() const;

private:
    // Hotkey storage
    std::map<HotkeyAction, HotkeyInfo> m_hotkeys;
    std::map<int, HotkeyAction> id_to_action;
    
    // State
    bool initialized = false;
    int m_nextHotkeyId = 1000;
    bool push_to_talk_active = false;
    
    // Window handle for message processing
    HWND message_window = nullptr;
    
    // Private implementation
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // GLOBALHOTKEYS_H