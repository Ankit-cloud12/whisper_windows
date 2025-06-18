/*
 * ClipboardManager.h
 * 
 * Clipboard and text insertion management for Windows.
 * Handles copying text to clipboard and inserting into active applications.
 * 
 * Features:
 * - Clipboard read/write operations
 * - Direct text insertion via SendInput
 * - Format preservation
 * - Multi-format support
 * - History tracking
 */

#ifndef CLIPBOARDMANAGER_H
#define CLIPBOARDMANAGER_H

#ifdef _WIN32
#include <windows.h> // Moved to the top
#endif

#include <QObject>
#include <QString>
#include <memory>
#include <vector>
#include <functional>

// Forward declarations
class QClipboard;
class QMimeData;

/**
 * @brief Text insertion method
 */
enum class InsertionMethod {
    Clipboard,          // Copy to clipboard and paste
    DirectInput,        // Send keystrokes directly
    ClipboardRestore,   // Clipboard with restore
    Auto               // Automatically choose best method
};

/**
 * @brief Clipboard format types
 */
enum class ClipboardFormat {
    PlainText,         // Plain text format
    RichText,          // Rich text (RTF)
    Html,              // HTML format
    Unicode,           // Unicode text
    Custom             // Custom format
};

/**
 * @brief Clipboard history entry
 */
struct ClipboardEntry {
    QString text;                       // Text content
    ClipboardFormat format;             // Format type
    QDateTime timestamp;                // When copied
    QString source_application;         // Source app (if available)
    std::map<QString, QVariant> metadata;  // Additional metadata
};

/**
 * @brief Clipboard and text insertion manager
 */
class ClipboardManager : public QObject {
    Q_OBJECT

public:
    explicit ClipboardManager(QObject* parent = nullptr);
    ~ClipboardManager();

    /**
     * @brief Initialize clipboard manager
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Shutdown clipboard manager
     */
    void shutdown();

    /**
     * @brief Copy text to clipboard
     * @param text Text to copy
     * @param format Text format
     * @return true if successful
     */
    bool copyToClipboard(const QString& text,
                        ClipboardFormat format = ClipboardFormat::PlainText);

    /**
     * @brief Get text from clipboard
     * @param format Desired format
     * @return Clipboard text (empty if none)
     */
    QString getFromClipboard(ClipboardFormat format = ClipboardFormat::PlainText) const;

    /**
     * @brief Check if clipboard has text
     * @return true if clipboard contains text
     */
    bool hasText() const;

    /**
     * @brief Clear clipboard
     */
    void clearClipboard();

    /**
     * @brief Insert text into active application
     * @param text Text to insert
     * @param method Insertion method
     * @param restore_clipboard Restore original clipboard content
     * @return true if successful
     */
    bool insertText(const QString& text,
                   InsertionMethod method = InsertionMethod::Auto,
                   bool restore_clipboard = true);

    /**
     * @brief Set default insertion method
     * @param method Default method
     */
    void setDefaultInsertionMethod(InsertionMethod method);

    /**
     * @brief Get default insertion method
     * @return Current default method
     */
    InsertionMethod getDefaultInsertionMethod() const;

    /**
     * @brief Enable or disable clipboard monitoring
     * @param enabled true to enable monitoring
     */
    void setMonitoringEnabled(bool enabled);

    /**
     * @brief Check if monitoring is enabled
     * @return true if monitoring
     */
    bool isMonitoringEnabled() const;

    /**
     * @brief Enable or disable history tracking
     * @param enabled true to enable history
     */
    void setHistoryEnabled(bool enabled);

    /**
     * @brief Get clipboard history
     * @param max_entries Maximum entries to return (0 = all)
     * @return Vector of clipboard entries
     */
    std::vector<ClipboardEntry> getHistory(size_t max_entries = 0) const;

    /**
     * @brief Clear clipboard history
     */
    void clearHistory();

    /**
     * @brief Set maximum history size
     * @param max_size Maximum entries to keep
     */
    void setMaxHistorySize(size_t max_size);

    /**
     * @brief Get current active window info
     * @return Window title and class name
     */
    std::pair<QString, QString> getActiveWindowInfo() const;

    /**
     * @brief Check if application supports paste
     * @param window_class Window class name
     * @return true if paste is supported
     */
    bool supportsPaste(const QString& window_class) const;

    /**
     * @brief Set insertion delay
     * @param delay_ms Delay in milliseconds
     */
    void setInsertionDelay(int delay_ms);

    /**
     * @brief Get insertion delay
     * @return Delay in milliseconds
     */
    int getInsertionDelay() const;

    /**
     * @brief Register custom format
     * @param format_name Format name
     * @return Format ID
     */
    int registerCustomFormat(const QString& format_name);

    /**
     * @brief Set clipboard data with multiple formats
     * @param mime_data MIME data with multiple formats
     * @return true if successful
     */
    bool setClipboardData(const QMimeData* mime_data);

signals:
    /**
     * @brief Emitted when clipboard content changes
     */
    void clipboardChanged();

    /**
     * @brief Emitted when text is copied
     * @param text Copied text
     */
    void textCopied(const QString& text);

    /**
     * @brief Emitted when text insertion completes
     * @param success true if successful
     */
    void insertionCompleted(bool success);

    /**
     * @brief Emitted when monitoring detects new content
     * @param entry New clipboard entry
     */
    void newClipboardContent(const ClipboardEntry& entry);

private slots:
    /**
     * @brief Handle clipboard data change
     */
    void onClipboardChanged();

private:
    /**
     * @brief Insert text using clipboard method
     * @param text Text to insert
     * @param restore_clipboard Restore original content
     * @return true if successful
     */
    bool insertViaClipboard(const QString& text, bool restore_clipboard);

    /**
     * @brief Insert text using direct input
     * @param text Text to insert
     * @return true if successful
     */
    bool insertViaDirectInput(const QString& text);

    /**
     * @brief Send paste command (Ctrl+V)
     * @return true if successful
     */
    bool sendPasteCommand();

    /**
     * @brief Send key event
     * @param vk Virtual key code
     * @param down true for key down, false for key up
     */
    void sendKeyEvent(WORD vk, bool down);

    /**
     * @brief Convert text to input events
     * @param text Text to convert
     * @param inputs Output vector of input events
     */
    void textToInputEvents(const QString& text, std::vector<INPUT>& inputs);

    /**
     * @brief Add entry to history
     * @param entry Clipboard entry
     */
    void addToHistory(const ClipboardEntry& entry);

    /**
     * @brief Determine best insertion method
     * @param text Text to insert
     * @return Best method for current context
     */
    InsertionMethod determineBestMethod(const QString& text) const;

private:
    // Qt clipboard
    QClipboard* clipboard = nullptr;
    
    // Settings
    InsertionMethod default_method = InsertionMethod::Auto;
    bool monitoring_enabled = false;
    bool history_enabled = true;
    size_t max_history_size = 100;
    int insertion_delay_ms = 50;
    
    // History
    std::vector<ClipboardEntry> history;
    
    // State
    QString saved_clipboard_content;
    bool is_inserting = false;
    
    // Private implementation
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // CLIPBOARDMANAGER_H