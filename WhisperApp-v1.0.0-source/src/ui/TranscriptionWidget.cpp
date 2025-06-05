#include "TranscriptionWidget.h"
#include "../core/Logger.h"
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QTextBlock>
#include <QTextCursor>
#include <QPainter>
#include <QScrollBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QRegularExpression>
#include <QTimer>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QInputDialog>
#include <QFontDialog>
#include <QClipboard>
#include <QApplication>

// Custom syntax highlighter for timestamps
class TranscriptionHighlighter : public QSyntaxHighlighter
{
public:
    TranscriptionHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent)
    {
        // Timestamp format
        timestampFormat.setForeground(QColor(0, 128, 255));
        timestampFormat.setFontWeight(QFont::Bold);
        
        // Speaker format
        speakerFormat.setForeground(QColor(0, 150, 0));
        speakerFormat.setFontWeight(QFont::Bold);
        
        // Emphasis format
        emphasisFormat.setFontItalic(true);
    }
    
protected:
    void highlightBlock(const QString& text) override
    {
        // Highlight timestamps [00:00:00]
        QRegularExpression timestampRegex("\\[\\d{2}:\\d{2}(:\\d{2})?\\]");
        QRegularExpressionMatchIterator i = timestampRegex.globalMatch(text);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            setFormat(match.capturedStart(), match.capturedLength(), timestampFormat);
        }
        
        // Highlight speaker names (Name:)
        QRegularExpression speakerRegex("^\\w+:");
        QRegularExpressionMatch speakerMatch = speakerRegex.match(text);
        if (speakerMatch.hasMatch()) {
            setFormat(speakerMatch.capturedStart(), speakerMatch.capturedLength(), speakerFormat);
        }
        
        // Highlight emphasized text *text*
        QRegularExpression emphasisRegex("\\*[^*]+\\*");
        QRegularExpressionMatchIterator j = emphasisRegex.globalMatch(text);
        while (j.hasNext()) {
            QRegularExpressionMatch match = j.next();
            setFormat(match.capturedStart(), match.capturedLength(), emphasisFormat);
        }
    }
    
private:
    QTextCharFormat timestampFormat;
    QTextCharFormat speakerFormat;
    QTextCharFormat emphasisFormat;
};

// Line number area implementation
LineNumberArea::LineNumberArea(QPlainTextEdit* editor) : QWidget(editor), m_editor(editor)
{
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(m_editor->fontMetrics().horizontalAdvance(QLatin1Char('9')) * 4 + 5, 0);
}

void LineNumberArea::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), QColor(240, 240, 240));
    
    QTextBlock block = m_editor->firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(m_editor->blockBoundingGeometry(block).translated(m_editor->contentOffset()).top());
    int bottom = top + qRound(m_editor->blockBoundingRect(block).height());
    
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(120, 120, 120));
            painter.drawText(0, top, width() - 3, m_editor->fontMetrics().height(),
                           Qt::AlignRight, number);
        }
        
        block = block.next();
        top = bottom;
        bottom = top + qRound(m_editor->blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// TranscriptionWidget implementation
TranscriptionWidget::TranscriptionWidget(QWidget* parent)
    : QWidget(parent)
    , m_showTimestamps(true)
    , m_autoScroll(true)
    , m_zoomLevel(100)
    , m_modified(false)
{
    setupUI();
    createActions();
    createContextMenu();
    connectSignals();
    
    // Initialize syntax highlighter
    m_highlighter = new TranscriptionHighlighter(m_textEdit->document());
    
    // Initialize auto-scroll timer
    m_autoScrollTimer = new QTimer(this);
    m_autoScrollTimer->setInterval(100);
    connect(m_autoScrollTimer, &QTimer::timeout, this, &TranscriptionWidget::scrollToBottom);
    
    Logger::instance().log(Logger::LogLevel::Debug, "TranscriptionWidget", "Widget initialized");
}

TranscriptionWidget::~TranscriptionWidget()
{
}

void TranscriptionWidget::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Create find toolbar (hidden by default)
    m_findToolBar = new QToolBar(this);
    m_findToolBar->setVisible(false);
    layout->addWidget(m_findToolBar);
    
    // Create text editor with line numbers
    QHBoxLayout* editorLayout = new QHBoxLayout();
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);
    
    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setFont(QFont("Consolas", 10));
    m_textEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    
    m_lineNumberArea = new LineNumberArea(m_textEdit);
    
    editorLayout->addWidget(m_lineNumberArea);
    editorLayout->addWidget(m_textEdit);
    
    layout->addLayout(editorLayout);
}

void TranscriptionWidget::createActions()
{
    m_undoAction = new QAction(tr("&Undo"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    
    m_redoAction = new QAction(tr("&Redo"), this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    
    m_cutAction = new QAction(tr("Cu&t"), this);
    m_cutAction->setShortcut(QKeySequence::Cut);
    
    m_copyAction = new QAction(tr("&Copy"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    
    m_pasteAction = new QAction(tr("&Paste"), this);
    m_pasteAction->setShortcut(QKeySequence::Paste);
    
    m_selectAllAction = new QAction(tr("Select &All"), this);
    m_selectAllAction->setShortcut(QKeySequence::SelectAll);
    
    m_findAction = new QAction(tr("&Find..."), this);
    m_findAction->setShortcut(QKeySequence::Find);
    
    m_insertTimestampAction = new QAction(tr("Insert &Timestamp"), this);
    m_insertTimestampAction->setShortcut(tr("Ctrl+T"));
}

void TranscriptionWidget::createContextMenu()
{
    m_contextMenu = new QMenu(this);
    m_contextMenu->addAction(m_undoAction);
    m_contextMenu->addAction(m_redoAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_cutAction);
    m_contextMenu->addAction(m_copyAction);
    m_contextMenu->addAction(m_pasteAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_selectAllAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_findAction);
    m_contextMenu->addAction(m_insertTimestampAction);
}

void TranscriptionWidget::connectSignals()
{
    // Text editor signals
    connect(m_textEdit, &QPlainTextEdit::textChanged, this, &TranscriptionWidget::onTextChanged);
    connect(m_textEdit, &QPlainTextEdit::cursorPositionChanged, this, &TranscriptionWidget::onCursorPositionChanged);
    connect(m_textEdit, &QPlainTextEdit::selectionChanged, this, &TranscriptionWidget::onSelectionChanged);
    connect(m_textEdit, &QPlainTextEdit::updateRequest, this, [this](const QRect& rect, int dy) {
        if (dy) {
            m_lineNumberArea->scroll(0, dy);
        } else {
            m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
        }
    });
    connect(m_textEdit, &QPlainTextEdit::blockCountChanged, this, &TranscriptionWidget::updateLineNumbers);
    
    // Action connections
    connect(m_undoAction, &QAction::triggered, m_textEdit, &QPlainTextEdit::undo);
    connect(m_redoAction, &QAction::triggered, m_textEdit, &QPlainTextEdit::redo);
    connect(m_cutAction, &QAction::triggered, m_textEdit, &QPlainTextEdit::cut);
    connect(m_copyAction, &QAction::triggered, m_textEdit, &QPlainTextEdit::copy);
    connect(m_pasteAction, &QAction::triggered, m_textEdit, &QPlainTextEdit::paste);
    connect(m_selectAllAction, &QAction::triggered, m_textEdit, &QPlainTextEdit::selectAll);
    connect(m_findAction, &QAction::triggered, this, &TranscriptionWidget::showFindDialog);
    connect(m_insertTimestampAction, &QAction::triggered, this, &TranscriptionWidget::insertTimestamp);
    
    // Update action states
    connect(m_textEdit, &QPlainTextEdit::undoAvailable, m_undoAction, &QAction::setEnabled);
    connect(m_textEdit, &QPlainTextEdit::redoAvailable, m_redoAction, &QAction::setEnabled);
    connect(m_textEdit, &QPlainTextEdit::copyAvailable, m_copyAction, &QAction::setEnabled);
    connect(m_textEdit, &QPlainTextEdit::copyAvailable, m_cutAction, &QAction::setEnabled);
    
    // Forward signals
    connect(m_textEdit, &QPlainTextEdit::undoAvailable, this, &TranscriptionWidget::undoAvailable);
    connect(m_textEdit, &QPlainTextEdit::redoAvailable, this, &TranscriptionWidget::redoAvailable);
    connect(m_textEdit, &QPlainTextEdit::copyAvailable, this, &TranscriptionWidget::copyAvailable);
}

void TranscriptionWidget::appendText(const QString& text, int timestamp)
{
    QString textToAppend = text;
    
    // Add timestamp if provided and timestamps are shown
    if (timestamp >= 0 && m_showTimestamps) {
        QString timestampStr = formatTimestamp(timestamp);
        textToAppend = QString("[%1] %2").arg(timestampStr, text);
        
        // Store timestamp entry
        TimestampEntry entry;
        entry.position = m_textEdit->textCursor().position();
        entry.milliseconds = timestamp;
        entry.text = timestampStr;
        m_timestamps.append(entry);
    }
    
    // Append text
    m_textEdit->moveCursor(QTextCursor::End);
    if (!m_textEdit->toPlainText().isEmpty()) {
        m_textEdit->insertPlainText("\n");
    }
    m_textEdit->insertPlainText(textToAppend);
    
    // Auto-scroll if enabled
    if (m_autoScroll) {
        scrollToBottom();
    }
}

void TranscriptionWidget::setText(const QString& text)
{
    m_textEdit->setPlainText(text);
    m_timestamps.clear();
    updateTimestampDisplay();
}

QString TranscriptionWidget::getText() const
{
    return m_textEdit->toPlainText();
}

void TranscriptionWidget::clear()
{
    m_textEdit->clear();
    m_timestamps.clear();
    setModified(false);
}

bool TranscriptionWidget::isModified() const
{
    return m_modified;
}

void TranscriptionWidget::setModified(bool modified)
{
    if (m_modified != modified) {
        m_modified = modified;
        emit modifiedChanged(modified);
    }
}

bool TranscriptionWidget::loadFile(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Cannot open file: %1").arg(file.errorString()));
        return false;
    }
    
    QTextStream stream(&file);
    setText(stream.readAll());
    setModified(false);
    
    Logger::instance().log(Logger::LogLevel::Info, "TranscriptionWidget", 
                          QString("Loaded file: %1").arg(fileName).toStdString());
    return true;
}

bool TranscriptionWidget::saveFile(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Cannot save file: %1").arg(file.errorString()));
        return false;
    }
    
    QTextStream stream(&file);
    stream << getText();
    setModified(false);
    
    Logger::instance().log(Logger::LogLevel::Info, "TranscriptionWidget", 
                          QString("Saved file: %1").arg(fileName).toStdString());
    return true;
}

bool TranscriptionWidget::exportToHtml(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    stream << "<!DOCTYPE html>\n<html>\n<head>\n";
    stream << "<meta charset=\"UTF-8\">\n";
    stream << "<title>Transcription</title>\n";
    stream << "<style>\n";
    stream << "body { font-family: Arial, sans-serif; line-height: 1.6; margin: 40px; }\n";
    stream << ".timestamp { color: #0080ff; font-weight: bold; }\n";
    stream << ".speaker { color: #009600; font-weight: bold; }\n";
    stream << "em { font-style: italic; }\n";
    stream << "</style>\n</head>\n<body>\n";
    
    // Convert text to HTML with formatting
    QString htmlText = getText();
    htmlText.replace("&", "&amp;");
    htmlText.replace("<", "&lt;");
    htmlText.replace(">", "&gt;");
    htmlText.replace("\n", "<br>\n");
    
    // Apply formatting
    QRegularExpression timestampRegex("\\[(\\d{2}:\\d{2}(:\\d{2})?)\\]");
    htmlText.replace(timestampRegex, "<span class=\"timestamp\">[\\1]</span>");
    
    QRegularExpression speakerRegex("^(\\w+:)", QRegularExpression::MultilineOption);
    htmlText.replace(speakerRegex, "<span class=\"speaker\">\\1</span>");
    
    QRegularExpression emphasisRegex("\\*([^*]+)\\*");
    htmlText.replace(emphasisRegex, "<em>\\1</em>");
    
    stream << htmlText;
    stream << "\n</body>\n</html>";
    
    return true;
}

bool TranscriptionWidget::exportToMarkdown(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    stream << "# Transcription\n\n";
    
    QString text = getText();
    QStringList lines = text.split('\n');
    
    for (const QString& line : lines) {
        if (line.contains(QRegularExpression("^\\w+:"))) {
            // Speaker line
            stream << "**" << line << "**\n";
        } else {
            stream << line << "\n";
        }
    }
    
    return true;
}

bool TranscriptionWidget::exportToSrt(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    
    // Extract timestamped segments
    QString text = getText();
    QRegularExpression timestampRegex("\\[(\\d{2}):(\\d{2}):(\\d{2})\\]\\s*(.+)");
    QRegularExpressionMatchIterator i = timestampRegex.globalMatch(text);
    
    int index = 1;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        
        int hours = match.captured(1).toInt();
        int minutes = match.captured(2).toInt();
        int seconds = match.captured(3).toInt();
        QString content = match.captured(4);
        
        // Write SRT entry
        stream << index++ << "\n";
        stream << QString("%1:%2:%3,000 --> %4:%5:%6,000\n")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds + 3, 2, 10, QChar('0')); // Default 3 second duration
        stream << content << "\n\n";
    }
    
    return true;
}

// Edit operations
void TranscriptionWidget::undo()
{
    m_textEdit->undo();
}

void TranscriptionWidget::redo()
{
    m_textEdit->redo();
}

void TranscriptionWidget::cut()
{
    m_textEdit->cut();
}

void TranscriptionWidget::copy()
{
    m_textEdit->copy();
}

void TranscriptionWidget::paste()
{
    m_textEdit->paste();
}

void TranscriptionWidget::selectAll()
{
    m_textEdit->selectAll();
}

// Find and replace
void TranscriptionWidget::showFindDialog()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Find"),
                                       tr("Find what:"), QLineEdit::Normal,
                                       m_findText, &ok);
    if (ok && !text.isEmpty()) {
        m_findText = text;
        findNext();
    }
}

void TranscriptionWidget::showReplaceDialog()
{
    // TODO: Implement replace dialog
    QMessageBox::information(this, tr("Replace"), tr("Replace functionality coming soon!"));
}

void TranscriptionWidget::findNext()
{
    if (m_findText.isEmpty()) {
        showFindDialog();
        return;
    }
    
    performFind(m_findText, true, false, false);
}

void TranscriptionWidget::findPrevious()
{
    if (m_findText.isEmpty()) {
        showFindDialog();
        return;
    }
    
    performFind(m_findText, false, false, false);
}

// View operations
void TranscriptionWidget::zoomIn()
{
    m_zoomLevel = qMin(m_zoomLevel + 10, 200);
    QFont font = m_textEdit->font();
    font.setPointSize(10 * m_zoomLevel / 100);
    m_textEdit->setFont(font);
    emit zoomChanged(m_zoomLevel);
}

void TranscriptionWidget::zoomOut()
{
    m_zoomLevel = qMax(m_zoomLevel - 10, 50);
    QFont font = m_textEdit->font();
    font.setPointSize(10 * m_zoomLevel / 100);
    m_textEdit->setFont(font);
    emit zoomChanged(m_zoomLevel);
}

void TranscriptionWidget::zoomReset()
{
    m_zoomLevel = 100;
    QFont font = m_textEdit->font();
    font.setPointSize(10);
    m_textEdit->setFont(font);
    emit zoomChanged(m_zoomLevel);
}

void TranscriptionWidget::setShowTimestamps(bool show)
{
    m_showTimestamps = show;
    updateTimestampDisplay();
}

void TranscriptionWidget::setWordWrap(bool wrap)
{
    m_textEdit->setWordWrapMode(wrap ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
}

void TranscriptionWidget::setAutoScroll(bool enabled)
{
    m_autoScroll = enabled;
    if (enabled && m_textEdit->document()->blockCount() > 0) {
        scrollToBottom();
    }
}

// Timestamp operations
void TranscriptionWidget::insertTimestamp()
{
    // Get current time in milliseconds (would be replaced with actual recording time)
    int milliseconds = QTime::currentTime().msecsSinceStartOfDay();
    QString timestamp = formatTimestamp(milliseconds);
    
    m_textEdit->insertPlainText(QString("[%1] ").arg(timestamp));
    
    TimestampEntry entry;
    entry.position = m_textEdit->textCursor().position() - timestamp.length() - 3;
    entry.milliseconds = milliseconds;
    entry.text = timestamp;
    m_timestamps.append(entry);
}

void TranscriptionWidget::goToTimestamp(int milliseconds)
{
    // Find nearest timestamp
    for (const TimestampEntry& entry : m_timestamps) {
        if (entry.milliseconds >= milliseconds) {
            QTextCursor cursor = m_textEdit->textCursor();
            cursor.setPosition(entry.position);
            m_textEdit->setTextCursor(cursor);
            m_textEdit->ensureCursorVisible();
            break;
        }
    }
}

void TranscriptionWidget::removeTimestamp(int position)
{
    m_timestamps.removeIf([position](const TimestampEntry& entry) {
        return entry.position == position;
    });
}

void TranscriptionWidget::removeAllTimestamps()
{
    m_timestamps.clear();
    
    QString text = getText();
    QRegularExpression timestampRegex("\\[\\d{2}:\\d{2}(:\\d{2})?\\]\\s*");
    text.remove(timestampRegex);
    setText(text);
}

// Protected event handlers
void TranscriptionWidget::contextMenuEvent(QContextMenuEvent* event)
{
    m_contextMenu->exec(event->globalPos());
}

void TranscriptionWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Find)) {
        showFindDialog();
        return;
    }
    
    QWidget::keyPressEvent(event);
}

// Private slots
void TranscriptionWidget::onTextChanged()
{
    setModified(true);
    emit textChanged();
    updateLineNumbers();
}

void TranscriptionWidget::onCursorPositionChanged()
{
    highlightCurrentLine();
}

void TranscriptionWidget::onSelectionChanged()
{
    // Handle selection changes if needed
}

void TranscriptionWidget::updateLineNumbers()
{
    m_lineNumberArea->setFixedWidth(m_lineNumberArea->sizeHint().width());
    m_lineNumberArea->update();
}

void TranscriptionWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    
    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(Qt::yellow).lighter(160);
    
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = m_textEdit->textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);
    
    m_textEdit->setExtraSelections(extraSelections);
}

void TranscriptionWidget::performFind(const QString& text, bool forward, bool caseSensitive, bool wholeWords)
{
    QTextDocument::FindFlags flags = QTextDocument::FindFlags();
    
    if (!forward) {
        flags |= QTextDocument::FindBackward;
    }
    
    if (caseSensitive) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    
    if (wholeWords) {
        flags |= QTextDocument::FindWholeWords;
    }
    
    bool found = m_textEdit->find(text, flags);
    
    if (!found) {
        // Wrap around
        QTextCursor cursor = m_textEdit->textCursor();
        cursor.movePosition(forward ? QTextCursor::Start : QTextCursor::End);
        m_textEdit->setTextCursor(cursor);
        found = m_textEdit->find(text, flags);
        
        if (!found) {
            QMessageBox::information(this, tr("Find"), tr("Text not found: %1").arg(text));
        }
    }
}

// Private methods
void TranscriptionWidget::updateTimestampDisplay()
{
    // Re-parse timestamps from text if needed
    if (m_showTimestamps) {
        // Update display
    }
}

QString TranscriptionWidget::formatTimestamp(int milliseconds) const
{
    int hours = milliseconds / 3600000;
    int minutes = (milliseconds % 3600000) / 60000;
    int seconds = (milliseconds % 60000) / 1000;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
}

int TranscriptionWidget::parseTimestamp(const QString& timestamp) const
{
    QStringList parts = timestamp.split(':');
    if (parts.size() == 2) {
        // MM:SS format
        return parts[0].toInt() * 60000 + parts[1].toInt() * 1000;
    } else if (parts.size() == 3) {
        // HH:MM:SS format
        return parts[0].toInt() * 3600000 + parts[1].toInt() * 60000 + parts[2].toInt() * 1000;
    }
    return 0;
}

void TranscriptionWidget::scrollToBottom()
{
    QScrollBar* scrollBar = m_textEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}