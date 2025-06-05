#include "ClipboardManager.h"
#include <iostream>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>

ClipboardManager::ClipboardManager(QObject* parent)
    : QObject(parent)
    , m_clipboard(nullptr)
{
    // TODO: Initialize clipboard manager
    // - Get system clipboard instance
    // - Connect clipboard change signals
    
    m_clipboard = QApplication::clipboard();
    if (m_clipboard) {
        connect(m_clipboard, &QClipboard::dataChanged,
                this, &ClipboardManager::onClipboardChanged);
        std::cout << "ClipboardManager: Initialized" << std::endl;
    } else {
        std::cout << "ClipboardManager: Failed to get clipboard instance" << std::endl;
    }
}

ClipboardManager::~ClipboardManager()
{
    // TODO: Clean up resources
    // No explicit cleanup needed for clipboard
}

void ClipboardManager::setText(const QString& text)
{
    // TODO: Set clipboard text
    // - Set text to system clipboard
    // - Handle platform-specific clipboard modes
    
    if (m_clipboard) {
        m_clipboard->setText(text);
        std::cout << "ClipboardManager: Text set to clipboard" << std::endl;
        emit textCopied(text);
    }
}

QString ClipboardManager::text() const
{
    // TODO: Get clipboard text
    // - Retrieve text from system clipboard
    // - Handle empty or non-text clipboard content
    
    if (m_clipboard) {
        return m_clipboard->text();
    }
    
    return QString();
}

void ClipboardManager::clear()
{
    // TODO: Clear clipboard
    // - Clear clipboard content
    
    if (m_clipboard) {
        m_clipboard->clear();
        std::cout << "ClipboardManager: Clipboard cleared" << std::endl;
        emit clipboardCleared();
    }
}

bool ClipboardManager::hasText() const
{
    // TODO: Check if clipboard has text
    // - Check clipboard content type
    
    if (m_clipboard) {
        const QMimeData* mimeData = m_clipboard->mimeData();
        return mimeData && mimeData->hasText();
    }
    
    return false;
}

void ClipboardManager::appendText(const QString& text)
{
    // TODO: Append text to existing clipboard content
    // - Get current text
    // - Append new text
    // - Set combined text
    
    if (m_clipboard) {
        QString currentText = m_clipboard->text();
        QString newText = currentText.isEmpty() ? text : currentText + "\n" + text;
        setText(newText);
        std::cout << "ClipboardManager: Text appended to clipboard" << std::endl;
    }
}

void ClipboardManager::setRichText(const QString& html)
{
    // TODO: Set rich text (HTML) to clipboard
    // - Create mime data with HTML content
    // - Set to clipboard
    
    if (m_clipboard) {
        QMimeData* mimeData = new QMimeData();
        mimeData->setHtml(html);
        mimeData->setText(stripHtml(html));  // Also set plain text version
        m_clipboard->setMimeData(mimeData);
        std::cout << "ClipboardManager: Rich text set to clipboard" << std::endl;
    }
}

QString ClipboardManager::richText() const
{
    // TODO: Get rich text from clipboard
    // - Check if clipboard has HTML content
    // - Return HTML or empty string
    
    if (m_clipboard) {
        const QMimeData* mimeData = m_clipboard->mimeData();
        if (mimeData && mimeData->hasHtml()) {
            return mimeData->html();
        }
    }
    
    return QString();
}

QStringList ClipboardManager::formats() const
{
    // TODO: Get available clipboard formats
    // - List all MIME types available
    
    if (m_clipboard) {
        const QMimeData* mimeData = m_clipboard->mimeData();
        if (mimeData) {
            return mimeData->formats();
        }
    }
    
    return QStringList();
}

void ClipboardManager::copyWithTimestamp(const QString& text)
{
    // TODO: Copy text with timestamp prefix
    // - Get current timestamp
    // - Format text with timestamp
    // - Set to clipboard
    
    QDateTime now = QDateTime::currentDateTime();
    QString timestampedText = QString("[%1] %2")
        .arg(now.toString("yyyy-MM-dd hh:mm:ss"))
        .arg(text);
    
    setText(timestampedText);
    std::cout << "ClipboardManager: Text copied with timestamp" << std::endl;
}

void ClipboardManager::copyAsMarkdown(const QString& text)
{
    // TODO: Copy text formatted as markdown
    // - Convert plain text to markdown
    // - Handle code blocks, quotes, etc.
    // - Set both plain and HTML versions
    
    // Simple markdown formatting
    QString markdown = text;
    
    // Convert code blocks (text between backticks)
    markdown.replace(QRegularExpression("`([^`]+)`"), "<code>\\1</code>");
    
    // Convert bold (text between **)
    markdown.replace(QRegularExpression("\\*\\*([^\\*]+)\\*\\*"), "<b>\\1</b>");
    
    // Convert italic (text between *)
    markdown.replace(QRegularExpression("\\*([^\\*]+)\\*"), "<i>\\1</i>");
    
    setRichText(markdown);
    std::cout << "ClipboardManager: Text copied as markdown" << std::endl;
}

void ClipboardManager::onClipboardChanged()
{
    // TODO: Handle clipboard content change
    // - Check what changed
    // - Emit appropriate signals
    
    std::cout << "ClipboardManager: Clipboard content changed" << std::endl;
    
    if (hasText()) {
        emit clipboardChanged(text());
    }
}

QString ClipboardManager::stripHtml(const QString& html) const
{
    // TODO: Strip HTML tags from text
    // - Remove all HTML tags
    // - Convert entities to plain text
    // - Return clean text
    
    QString plain = html;
    
    // Remove HTML tags
    plain.remove(QRegularExpression("<[^>]*>"));
    
    // Convert common HTML entities
    plain.replace("&amp;", "&");
    plain.replace("&lt;", "<");
    plain.replace("&gt;", ">");
    plain.replace("&quot;", "\"");
    plain.replace("&apos;", "'");
    plain.replace("&nbsp;", " ");
    
    return plain.trimmed();
}

bool ClipboardManager::supportsSelection() const
{
    // TODO: Check if platform supports selection clipboard
    // - Linux/X11 has selection clipboard
    // - Windows/macOS typically don't
    
    return m_clipboard && m_clipboard->supportsSelection();
}

void ClipboardManager::setSelectionText(const QString& text)
{
    // TODO: Set selection clipboard (Linux/X11)
    // - Set text to selection clipboard if supported
    
    if (m_clipboard && m_clipboard->supportsSelection()) {
        m_clipboard->setText(text, QClipboard::Selection);
        std::cout << "ClipboardManager: Text set to selection clipboard" << std::endl;
    }
}

QString ClipboardManager::selectionText() const
{
    // TODO: Get selection clipboard text
    // - Get text from selection clipboard if supported
    
    if (m_clipboard && m_clipboard->supportsSelection()) {
        return m_clipboard->text(QClipboard::Selection);
    }
    
    return QString();
}