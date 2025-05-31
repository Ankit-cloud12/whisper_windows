#include "HotkeyEditWidget.h"
#include "../core/Logger.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QApplication>

HotkeyEditWidget::HotkeyEditWidget(QWidget* parent)
    : QWidget(parent)
    , m_isRecording(false)
    , m_modifiers(0)
    , m_key(0)
{
    setupUI();
    updateDisplay();
}

HotkeyEditWidget::~HotkeyEditWidget()
{
}

void HotkeyEditWidget::setupUI()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Key display
    m_keyEdit = new QLineEdit(this);
    m_keyEdit->setReadOnly(true);
    m_keyEdit->setPlaceholderText(tr("Click Record to set hotkey"));
    m_keyEdit->installEventFilter(this);
    
    // Record button
    m_recordButton = new QPushButton(tr("Record"), this);
    m_recordButton->setCheckable(true);
    m_recordButton->setMaximumWidth(80);
    
    // Clear button
    m_clearButton = new QPushButton(tr("Clear"), this);
    m_clearButton->setMaximumWidth(60);
    
    layout->addWidget(m_keyEdit);
    layout->addWidget(m_recordButton);
    layout->addWidget(m_clearButton);
    
    // Connect signals
    connect(m_recordButton, &QPushButton::clicked, this, &HotkeyEditWidget::onRecordButtonClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &HotkeyEditWidget::onClearButtonClicked);
}

void HotkeyEditWidget::setHotkey(const QString& hotkey)
{
    m_keySequence = QKeySequence(hotkey);
    updateDisplay();
    emit hotkeyChanged(hotkey);
}

QString HotkeyEditWidget::getHotkey() const
{
    return m_keySequence.toString();
}

void HotkeyEditWidget::setKeySequence(const QKeySequence& sequence)
{
    m_keySequence = sequence;
    updateDisplay();
    emit hotkeyChanged(sequence.toString());
}

QKeySequence HotkeyEditWidget::getKeySequence() const
{
    return m_keySequence;
}

void HotkeyEditWidget::clear()
{
    m_keySequence = QKeySequence();
    m_modifiers = 0;
    m_key = 0;
    updateDisplay();
    emit hotkeyChanged(QString());
}

bool HotkeyEditWidget::isValid() const
{
    return !m_keySequence.isEmpty() && isValidHotkey(m_keySequence);
}

void HotkeyEditWidget::setEnabled(bool enabled)
{
    QWidget::setEnabled(enabled);
    m_keyEdit->setEnabled(enabled);
    m_recordButton->setEnabled(enabled);
    m_clearButton->setEnabled(enabled);
    
    if (!enabled && m_isRecording) {
        stopRecording();
    }
}

void HotkeyEditWidget::startRecording()
{
    if (m_isRecording) {
        return;
    }
    
    m_isRecording = true;
    m_modifiers = 0;
    m_key = 0;
    
    m_recordButton->setChecked(true);
    m_recordButton->setText(tr("Stop"));
    m_keyEdit->setText(tr("Press hotkey combination..."));
    m_keyEdit->setFocus();
    
    // Install global event filter
    qApp->installEventFilter(this);
    
    emit recordingStarted();
    
    Logger::instance().log(Logger::LogLevel::Debug, "HotkeyEditWidget", "Started recording hotkey");
}

void HotkeyEditWidget::stopRecording()
{
    if (!m_isRecording) {
        return;
    }
    
    m_isRecording = false;
    
    m_recordButton->setChecked(false);
    m_recordButton->setText(tr("Record"));
    
    // Remove global event filter
    qApp->removeEventFilter(this);
    
    // Create key sequence from captured keys
    if (m_key != 0) {
        m_keySequence = QKeySequence(m_modifiers | m_key);
        
        if (isValidHotkey(m_keySequence)) {
            updateDisplay();
            emit hotkeyChanged(m_keySequence.toString());
        } else {
            clear();
            m_keyEdit->setText(tr("Invalid hotkey"));
        }
    } else {
        updateDisplay();
    }
    
    emit recordingStopped();
    
    Logger::instance().log(Logger::LogLevel::Debug, "HotkeyEditWidget", "Stopped recording hotkey");
}

bool HotkeyEditWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (m_isRecording && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        processKeyEvent(keyEvent);
        return true;
    } else if (m_isRecording && event->type() == QEvent::KeyRelease) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        
        // Stop recording on key release if we have a complete combination
        if (m_key != 0 && !isModifierKey(keyEvent->key())) {
            stopRecording();
        }
        return true;
    } else if (obj == m_keyEdit && event->type() == QEvent::MouseButtonDblClick) {
        // Start recording on double-click
        if (!m_isRecording) {
            startRecording();
        }
        return true;
    }
    
    return QWidget::eventFilter(obj, event);
}

void HotkeyEditWidget::focusInEvent(QFocusEvent* event)
{
    QWidget::focusInEvent(event);
    
    if (!m_isRecording && event->reason() == Qt::MouseFocusReason) {
        // Optionally start recording when clicked
    }
}

void HotkeyEditWidget::focusOutEvent(QFocusEvent* event)
{
    QWidget::focusOutEvent(event);
    
    if (m_isRecording) {
        // Cancel recording if focus is lost
        m_isRecording = false;
        m_recordButton->setChecked(false);
        m_recordButton->setText(tr("Record"));
        qApp->removeEventFilter(this);
        updateDisplay();
        emit recordingStopped();
    }
}

void HotkeyEditWidget::onRecordButtonClicked()
{
    if (m_recordButton->isChecked()) {
        startRecording();
    } else {
        stopRecording();
    }
}

void HotkeyEditWidget::onClearButtonClicked()
{
    clear();
}

void HotkeyEditWidget::updateDisplay()
{
    if (m_keySequence.isEmpty()) {
        m_keyEdit->clear();
        m_keyEdit->setPlaceholderText(tr("Click Record to set hotkey"));
    } else {
        m_keyEdit->setText(keySequenceToString(m_keySequence));
    }
    
    m_clearButton->setEnabled(!m_keySequence.isEmpty());
}

void HotkeyEditWidget::processKeyEvent(QKeyEvent* keyEvent)
{
    int key = keyEvent->key();
    Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
    
    // Update modifiers
    m_modifiers = 0;
    if (modifiers & Qt::ControlModifier) m_modifiers |= Qt::CTRL;
    if (modifiers & Qt::ShiftModifier) m_modifiers |= Qt::SHIFT;
    if (modifiers & Qt::AltModifier) m_modifiers |= Qt::ALT;
    if (modifiers & Qt::MetaModifier) m_modifiers |= Qt::META;
    
    // Check if it's a modifier key
    if (isModifierKey(key)) {
        // Update display with current modifiers
        QString text = tr("Press hotkey combination...");
        if (m_modifiers != 0) {
            QStringList mods;
            if (m_modifiers & Qt::CTRL) mods << "Ctrl";
            if (m_modifiers & Qt::SHIFT) mods << "Shift";
            if (m_modifiers & Qt::ALT) mods << "Alt";
            if (m_modifiers & Qt::META) mods << "Meta";
            text = mods.join("+") + "+...";
        }
        m_keyEdit->setText(text);
    } else {
        // Regular key pressed
        m_key = key;
        
        // Update display with complete combination
        QKeySequence tempSeq(m_modifiers | m_key);
        m_keyEdit->setText(keySequenceToString(tempSeq));
    }
}

QString HotkeyEditWidget::keySequenceToString(const QKeySequence& sequence) const
{
    if (sequence.isEmpty()) {
        return QString();
    }
    
    // Use portable string representation
    QString str = sequence.toString(QKeySequence::PortableText);
    
    // Make it more user-friendly
    str.replace("Meta", "Win");
    
    return str;
}

bool HotkeyEditWidget::isModifierKey(int key) const
{
    return key == Qt::Key_Control ||
           key == Qt::Key_Shift ||
           key == Qt::Key_Alt ||
           key == Qt::Key_Meta ||
           key == Qt::Key_AltGr ||
           key == Qt::Key_Super_L ||
           key == Qt::Key_Super_R ||
           key == Qt::Key_Hyper_L ||
           key == Qt::Key_Hyper_R;
}

bool HotkeyEditWidget::isValidHotkey(const QKeySequence& sequence) const
{
    if (sequence.isEmpty()) {
        return false;
    }
    
    // Get the key combination
    int key = sequence[0];
    
    // Must have at least one modifier
    if (!(key & (Qt::CTRL | Qt::SHIFT | Qt::ALT | Qt::META))) {
        return false;
    }
    
    // Extract the actual key
    int actualKey = key & ~(Qt::CTRL | Qt::SHIFT | Qt::ALT | Qt::META);
    
    // Check if it's a valid key
    if (actualKey == 0 || actualKey == Qt::Key_unknown) {
        return false;
    }
    
    // Don't allow certain keys
    if (actualKey == Qt::Key_Escape ||
        actualKey == Qt::Key_Tab ||
        actualKey == Qt::Key_Backtab ||
        actualKey == Qt::Key_Backspace ||
        actualKey == Qt::Key_Return ||
        actualKey == Qt::Key_Enter) {
        return false;
    }
    
    return true;
}