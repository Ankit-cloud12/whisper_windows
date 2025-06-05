#pragma once

#include <QWidget>
#include <QString>
#include <QKeySequence>

// Forward declarations
class QLineEdit;
class QPushButton;

/**
 * @brief Custom widget for editing keyboard shortcuts
 * 
 * Allows users to capture and edit hotkey combinations with
 * conflict detection and validation.
 */
class HotkeyEditWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit HotkeyEditWidget(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~HotkeyEditWidget();
    
    /**
     * @brief Set the current hotkey
     * @param hotkey Hotkey string (e.g., "Ctrl+Shift+R")
     */
    void setHotkey(const QString& hotkey);
    
    /**
     * @brief Get the current hotkey
     * @return Current hotkey string
     */
    QString getHotkey() const;
    
    /**
     * @brief Set the key sequence
     * @param sequence Key sequence
     */
    void setKeySequence(const QKeySequence& sequence);
    
    /**
     * @brief Get the key sequence
     * @return Current key sequence
     */
    QKeySequence getKeySequence() const;
    
    /**
     * @brief Clear the hotkey
     */
    void clear();
    
    /**
     * @brief Check if hotkey is valid
     * @return true if valid
     */
    bool isValid() const;
    
    /**
     * @brief Enable or disable the widget
     * @param enabled Enabled state
     */
    void setEnabled(bool enabled);

signals:
    /**
     * @brief Emitted when hotkey changes
     * @param hotkey New hotkey string
     */
    void hotkeyChanged(const QString& hotkey);
    
    /**
     * @brief Emitted when recording starts
     */
    void recordingStarted();
    
    /**
     * @brief Emitted when recording stops
     */
    void recordingStopped();

public slots:
    /**
     * @brief Start recording a new hotkey
     */
    void startRecording();
    
    /**
     * @brief Stop recording
     */
    void stopRecording();

protected:
    /**
     * @brief Handle key press events
     * @param event Key event
     * @return true if handled
     */
    bool eventFilter(QObject* obj, QEvent* event) override;
    
    /**
     * @brief Handle focus in event
     * @param event Focus event
     */
    void focusInEvent(QFocusEvent* event) override;
    
    /**
     * @brief Handle focus out event
     * @param event Focus event
     */
    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void onRecordButtonClicked();
    void onClearButtonClicked();
    void updateDisplay();

private:
    void setupUI();
    void processKeyEvent(QKeyEvent* keyEvent);
    QString keySequenceToString(const QKeySequence& sequence) const;
    bool isModifierKey(int key) const;
    bool isValidHotkey(const QKeySequence& sequence) const;

private:
    // UI elements
    QLineEdit* m_keyEdit;
    QPushButton* m_recordButton;
    QPushButton* m_clearButton;
    
    // State
    bool m_isRecording;
    QKeySequence m_keySequence;
    int m_modifiers;
    int m_key;
};