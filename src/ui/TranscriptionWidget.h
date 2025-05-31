#pragma once

#include <QWidget>
#include <QString>
#include <QList>
#include <QPair>

// Forward declarations
class QTextEdit;
class QPlainTextEdit;
class QSyntaxHighlighter;
class QTimer;
class QToolBar;
class QAction;
class QMenu;

/**
 * @brief Timestamp entry for transcription
 */
struct TimestampEntry {
    int position;       // Character position in text
    int milliseconds;   // Timestamp in milliseconds
    QString text;       // Timestamp text representation
};

/**
 * @brief Widget for displaying and editing transcriptions
 * 
 * Features:
 * - Rich text editing with syntax highlighting
 * - Timestamp display and navigation
 * - Find and replace functionality
 * - Export to various formats
 * - Auto-scroll during live transcription
 */
class TranscriptionWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit TranscriptionWidget(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~TranscriptionWidget();
    
    /**
     * @brief Append text to transcription
     * @param text Text to append
     * @param timestamp Optional timestamp in milliseconds
     */
    void appendText(const QString& text, int timestamp = -1);
    
    /**
     * @brief Set complete transcription text
     * @param text Complete text
     */
    void setText(const QString& text);
    
    /**
     * @brief Get current transcription text
     * @return Current text
     */
    QString getText() const;
    
    /**
     * @brief Clear all text
     */
    void clear();
    
    /**
     * @brief Check if text has been modified
     * @return true if modified
     */
    bool isModified() const;
    
    /**
     * @brief Set modified state
     * @param modified Modified state
     */
    void setModified(bool modified);
    
    /**
     * @brief Load transcription from file
     * @param fileName File path
     * @return true if successful
     */
    bool loadFile(const QString& fileName);
    
    /**
     * @brief Save transcription to file
     * @param fileName File path
     * @return true if successful
     */
    bool saveFile(const QString& fileName);
    
    /**
     * @brief Export to HTML format
     * @param fileName File path
     * @return true if successful
     */
    bool exportToHtml(const QString& fileName);
    
    /**
     * @brief Export to Markdown format
     * @param fileName File path
     * @return true if successful
     */
    bool exportToMarkdown(const QString& fileName);
    
    /**
     * @brief Export to SRT subtitle format
     * @param fileName File path
     * @return true if successful
     */
    bool exportToSrt(const QString& fileName);

public slots:
    // Edit operations
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    
    // Find and replace
    void showFindDialog();
    void showReplaceDialog();
    void findNext();
    void findPrevious();
    
    // View operations
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void setShowTimestamps(bool show);
    void setWordWrap(bool wrap);
    void setAutoScroll(bool enabled);
    
    // Timestamp operations
    void insertTimestamp();
    void goToTimestamp(int milliseconds);
    void removeTimestamp(int position);
    void removeAllTimestamps();

signals:
    /**
     * @brief Emitted when text changes
     */
    void textChanged();
    
    /**
     * @brief Emitted when modified state changes
     * @param modified New modified state
     */
    void modifiedChanged(bool modified);
    
    /**
     * @brief Emitted when undo becomes available/unavailable
     * @param available Availability state
     */
    void undoAvailable(bool available);
    
    /**
     * @brief Emitted when redo becomes available/unavailable
     * @param available Availability state
     */
    void redoAvailable(bool available);
    
    /**
     * @brief Emitted when copy becomes available/unavailable
     * @param available Availability state
     */
    void copyAvailable(bool available);
    
    /**
     * @brief Emitted when user clicks on timestamp
     * @param milliseconds Timestamp value
     */
    void timestampClicked(int milliseconds);
    
    /**
     * @brief Emitted when zoom level changes
     * @param level New zoom level (percentage)
     */
    void zoomChanged(int level);

protected:
    /**
     * @brief Handle context menu event
     * @param event Context menu event
     */
    void contextMenuEvent(QContextMenuEvent* event) override;
    
    /**
     * @brief Handle key press event
     * @param event Key event
     */
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onTextChanged();
    void onCursorPositionChanged();
    void onSelectionChanged();
    void updateLineNumbers();
    void highlightCurrentLine();
    void performFind(const QString& text, bool forward, bool caseSensitive, bool wholeWords);
    void performReplace(const QString& findText, const QString& replaceText, bool caseSensitive, bool wholeWords);
    void performReplaceAll(const QString& findText, const QString& replaceText, bool caseSensitive, bool wholeWords);

private:
    void setupUI();
    void createActions();
    void createContextMenu();
    void connectSignals();
    void updateTimestampDisplay();
    QString formatTimestamp(int milliseconds) const;
    int parseTimestamp(const QString& timestamp) const;
    void scrollToBottom();
    void applyHighlighting();
    
private:
    // UI components
    QPlainTextEdit* m_textEdit;
    QWidget* m_lineNumberArea;
    QToolBar* m_findToolBar;
    QMenu* m_contextMenu;
    
    // Actions
    QAction* m_undoAction;
    QAction* m_redoAction;
    QAction* m_cutAction;
    QAction* m_copyAction;
    QAction* m_pasteAction;
    QAction* m_selectAllAction;
    QAction* m_findAction;
    QAction* m_insertTimestampAction;
    
    // State
    bool m_showTimestamps;
    bool m_autoScroll;
    int m_zoomLevel;
    QString m_findText;
    bool m_modified;
    
    // Timestamps
    QList<TimestampEntry> m_timestamps;
    
    // Syntax highlighter
    QSyntaxHighlighter* m_highlighter;
    
    // Timers
    QTimer* m_autoScrollTimer;
};

/**
 * @brief Line number area widget for TranscriptionWidget
 */
class LineNumberArea : public QWidget
{
public:
    LineNumberArea(QPlainTextEdit* editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPlainTextEdit* m_editor;
};