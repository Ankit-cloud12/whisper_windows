#include "TranscriptionHistoryWidget.h"
#include "../core/Settings.h"
#include "../core/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>

TranscriptionHistoryWidget::TranscriptionHistoryWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
    loadHistory();
    
    Logger::instance().log(Logger::LogLevel::Debug, "TranscriptionHistoryWidget", "History widget initialized");
}

TranscriptionHistoryWidget::~TranscriptionHistoryWidget()
{
    saveHistory();
}

void TranscriptionHistoryWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Search and filter bar
    QGroupBox* filterGroup = new QGroupBox(tr("Search and Filter"), this);
    QHBoxLayout* filterLayout = new QHBoxLayout(filterGroup);
    
    // Search
    m_searchEdit = new QLineEdit(filterGroup);
    m_searchEdit->setPlaceholderText(tr("Search transcriptions..."));
    m_searchEdit->setClearButtonEnabled(true);
    
    // Language filter
    m_languageFilter = new QComboBox(filterGroup);
    m_languageFilter->addItem(tr("All Languages"), "");
    m_languageFilter->addItem(tr("English"), "en");
    m_languageFilter->addItem(tr("Spanish"), "es");
    m_languageFilter->addItem(tr("French"), "fr");
    m_languageFilter->addItem(tr("German"), "de");
    m_languageFilter->addItem(tr("Chinese"), "zh");
    m_languageFilter->addItem(tr("Japanese"), "ja");
    
    // Date range
    QLabel* fromLabel = new QLabel(tr("From:"), filterGroup);
    m_fromDateEdit = new QDateEdit(filterGroup);
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setDate(QDate::currentDate().addMonths(-1));
    m_fromDateEdit->setMaximumDate(QDate::currentDate());
    
    QLabel* toLabel = new QLabel(tr("To:"), filterGroup);
    m_toDateEdit = new QDateEdit(filterGroup);
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setDate(QDate::currentDate());
    m_toDateEdit->setMaximumDate(QDate::currentDate());
    
    filterLayout->addWidget(new QLabel(tr("Search:"), filterGroup));
    filterLayout->addWidget(m_searchEdit, 2);
    filterLayout->addWidget(new QLabel(tr("Language:"), filterGroup));
    filterLayout->addWidget(m_languageFilter);
    filterLayout->addWidget(fromLabel);
    filterLayout->addWidget(m_fromDateEdit);
    filterLayout->addWidget(toLabel);
    filterLayout->addWidget(m_toDateEdit);
    filterLayout->addStretch();
    
    mainLayout->addWidget(filterGroup);
    
    // Splitter for list and preview
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    
    // History list
    QWidget* listWidget = new QWidget(splitter);
    QVBoxLayout* listLayout = new QVBoxLayout(listWidget);
    
    m_historyList = new QListWidget(listWidget);
    m_historyList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_historyList->setAlternatingRowColors(true);
    
    listLayout->addWidget(m_historyList);
    
    // List controls
    QHBoxLayout* listControlLayout = new QHBoxLayout();
    
    m_exportButton = new QPushButton(tr("Export Selected"), listWidget);
    m_exportButton->setEnabled(false);
    
    m_deleteButton = new QPushButton(tr("Delete Selected"), listWidget);
    m_deleteButton->setEnabled(false);
    
    m_clearButton = new QPushButton(tr("Clear All"), listWidget);
    
    listControlLayout->addWidget(m_exportButton);
    listControlLayout->addWidget(m_deleteButton);
    listControlLayout->addWidget(m_clearButton);
    listControlLayout->addStretch();
    
    listLayout->addLayout(listControlLayout);
    
    // Preview pane
    QWidget* previewWidget = new QWidget(splitter);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewWidget);
    
    QLabel* previewLabel = new QLabel(tr("Preview"), previewWidget);
    previewLabel->setStyleSheet("QLabel { font-weight: bold; }");
    previewLayout->addWidget(previewLabel);
    
    m_previewText = new QTextEdit(previewWidget);
    m_previewText->setReadOnly(true);
    previewLayout->addWidget(m_previewText);
    
    // Add widgets to splitter
    splitter->addWidget(listWidget);
    splitter->addWidget(previewWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter);
    
    // Statistics
    m_statisticsLabel = new QLabel(this);
    m_statisticsLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    mainLayout->addWidget(m_statisticsLabel);
    
    updateStatistics();
}

void TranscriptionHistoryWidget::connectSignals()
{
    connect(m_searchEdit, &QLineEdit::textChanged, this, &TranscriptionHistoryWidget::onSearchTextChanged);
    connect(m_languageFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &TranscriptionHistoryWidget::onFilterChanged);
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, &TranscriptionHistoryWidget::onFilterChanged);
    connect(m_toDateEdit, &QDateEdit::dateChanged, this, &TranscriptionHistoryWidget::onFilterChanged);
    
    connect(m_historyList, &QListWidget::itemSelectionChanged, 
            this, &TranscriptionHistoryWidget::onSelectionChanged);
    connect(m_historyList, &QListWidget::itemDoubleClicked,
            this, &TranscriptionHistoryWidget::onItemDoubleClicked);
    
    connect(m_exportButton, &QPushButton::clicked, this, &TranscriptionHistoryWidget::onExportClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &TranscriptionHistoryWidget::onDeleteClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &TranscriptionHistoryWidget::onClearClicked);
}

void TranscriptionHistoryWidget::addEntry(const TranscriptionHistoryEntry& entry)
{
    m_history.append(entry);
    saveHistory();
    applyFilters();
    emit historyModified();
}

void TranscriptionHistoryWidget::loadHistory()
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString historyFile = QDir(dataPath).filePath("transcription_history.json");
    
    QFile file(historyFile);
    if (!file.open(QIODevice::ReadOnly)) {
        Logger::instance().log(Logger::LogLevel::Debug, "TranscriptionHistoryWidget", 
                             "No history file found, starting with empty history");
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        Logger::instance().log(Logger::LogLevel::Warning, "TranscriptionHistoryWidget", 
                             "Invalid history file format");
        return;
    }
    
    m_history.clear();
    QJsonArray array = doc.array();
    
    for (const QJsonValue& value : array) {
        QJsonObject obj = value.toObject();
        
        TranscriptionHistoryEntry entry;
        entry.id = obj["id"].toString();
        entry.text = obj["text"].toString();
        entry.audioFile = obj["audioFile"].toString();
        entry.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
        entry.duration = obj["duration"].toInt();
        entry.language = obj["language"].toString();
        entry.model = obj["model"].toString();
        
        m_history.append(entry);
    }
    
    applyFilters();
    
    Logger::instance().log(Logger::LogLevel::Info, "TranscriptionHistoryWidget", 
                         QString("Loaded %1 history entries").arg(m_history.size()).toStdString());
}

void TranscriptionHistoryWidget::saveHistory()
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    QString historyFile = QDir(dataPath).filePath("transcription_history.json");
    
    QJsonArray array;
    for (const TranscriptionHistoryEntry& entry : m_history) {
        QJsonObject obj;
        obj["id"] = entry.id;
        obj["text"] = entry.text;
        obj["audioFile"] = entry.audioFile;
        obj["timestamp"] = entry.timestamp.toString(Qt::ISODate);
        obj["duration"] = entry.duration;
        obj["language"] = entry.language;
        obj["model"] = entry.model;
        array.append(obj);
    }
    
    QJsonDocument doc(array);
    
    QFile file(historyFile);
    if (!file.open(QIODevice::WriteOnly)) {
        Logger::instance().log(Logger::LogLevel::Error, "TranscriptionHistoryWidget", 
                             "Failed to save history file");
        return;
    }
    
    file.write(doc.toJson());
    
    Logger::instance().log(Logger::LogLevel::Debug, "TranscriptionHistoryWidget", 
                         QString("Saved %1 history entries").arg(m_history.size()).toStdString());
}

void TranscriptionHistoryWidget::clearHistory()
{
    int ret = QMessageBox::question(this, tr("Clear History"),
                                   tr("Are you sure you want to clear all transcription history?"),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_history.clear();
        m_filteredHistory.clear();
        m_historyList->clear();
        m_previewText->clear();
        saveHistory();
        updateStatistics();
        emit historyModified();
        
        Logger::instance().log(Logger::LogLevel::Info, "TranscriptionHistoryWidget", 
                             "History cleared by user");
    }
}

TranscriptionHistoryEntry* TranscriptionHistoryWidget::getSelectedEntry()
{
    QListWidgetItem* item = m_historyList->currentItem();
    if (!item) {
        return nullptr;
    }
    
    int index = item->data(Qt::UserRole).toInt();
    if (index >= 0 && index < m_filteredHistory.size()) {
        return &m_filteredHistory[index];
    }
    
    return nullptr;
}

void TranscriptionHistoryWidget::search(const QString& text)
{
    m_searchEdit->setText(text);
}

void TranscriptionHistoryWidget::filterByDate(const QDate& from, const QDate& to)
{
    m_fromDateEdit->setDate(from);
    m_toDateEdit->setDate(to);
}

void TranscriptionHistoryWidget::filterByLanguage(const QString& language)
{
    int index = m_languageFilter->findData(language);
    if (index >= 0) {
        m_languageFilter->setCurrentIndex(index);
    }
}

void TranscriptionHistoryWidget::exportSelected()
{
    QList<QListWidgetItem*> selectedItems = m_historyList->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Transcriptions"),
                                                   "transcriptions.txt",
                                                   tr("Text Files (*.txt);;CSV Files (*.csv)"));
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export Failed"),
                            tr("Failed to create export file."));
        return;
    }
    
    QTextStream stream(&file);
    
    if (fileName.endsWith(".csv")) {
        // CSV format
        stream << "Timestamp,Duration,Language,Model,Text\n";
        
        for (QListWidgetItem* item : selectedItems) {
            int index = item->data(Qt::UserRole).toInt();
            if (index >= 0 && index < m_filteredHistory.size()) {
                const TranscriptionHistoryEntry& entry = m_filteredHistory[index];
                stream << entry.timestamp.toString(Qt::ISODate) << ","
                       << entry.duration << ","
                       << entry.language << ","
                       << entry.model << ","
                       << "\"" << entry.text.replace("\"", "\"\"") << "\"\n";
            }
        }
    } else {
        // Text format
        for (QListWidgetItem* item : selectedItems) {
            int index = item->data(Qt::UserRole).toInt();
            if (index >= 0 && index < m_filteredHistory.size()) {
                const TranscriptionHistoryEntry& entry = m_filteredHistory[index];
                stream << "=== " << entry.timestamp.toString() << " ===\n";
                stream << "Duration: " << formatDuration(entry.duration) << "\n";
                stream << "Language: " << entry.language << "\n";
                stream << "Model: " << entry.model << "\n";
                stream << "\n" << entry.text << "\n\n";
                stream << "----------------------------------------\n\n";
            }
        }
    }
    
    QMessageBox::information(this, tr("Export Complete"),
                           tr("Successfully exported %1 transcriptions.").arg(selectedItems.size()));
    
    Logger::instance().log(Logger::LogLevel::Info, "TranscriptionHistoryWidget", 
                         QString("Exported %1 transcriptions").arg(selectedItems.size()).toStdString());
}

void TranscriptionHistoryWidget::deleteSelected()
{
    QList<QListWidgetItem*> selectedItems = m_historyList->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    int ret = QMessageBox::question(this, tr("Delete Transcriptions"),
                                   tr("Are you sure you want to delete %1 selected transcription(s)?")
                                   .arg(selectedItems.size()),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        // Collect IDs to delete
        QStringList idsToDelete;
        for (QListWidgetItem* item : selectedItems) {
            int index = item->data(Qt::UserRole).toInt();
            if (index >= 0 && index < m_filteredHistory.size()) {
                idsToDelete.append(m_filteredHistory[index].id);
            }
        }
        
        // Remove from history
        m_history.removeIf([&idsToDelete](const TranscriptionHistoryEntry& entry) {
            return idsToDelete.contains(entry.id);
        });
        
        saveHistory();
        applyFilters();
        emit historyModified();
        
        Logger::instance().log(Logger::LogLevel::Info, "TranscriptionHistoryWidget", 
                             QString("Deleted %1 transcriptions").arg(idsToDelete.size()).toStdString());
    }
}

void TranscriptionHistoryWidget::onSelectionChanged()
{
    QList<QListWidgetItem*> selectedItems = m_historyList->selectedItems();
    bool hasSelection = !selectedItems.isEmpty();
    
    m_exportButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);
    
    if (hasSelection && selectedItems.size() == 1) {
        // Show preview for single selection
        QListWidgetItem* item = selectedItems.first();
        int index = item->data(Qt::UserRole).toInt();
        
        if (index >= 0 && index < m_filteredHistory.size()) {
            const TranscriptionHistoryEntry& entry = m_filteredHistory[index];
            
            QString preview;
            preview += QString("<b>Date:</b> %1<br>").arg(entry.timestamp.toString());
            preview += QString("<b>Duration:</b> %1<br>").arg(formatDuration(entry.duration));
            preview += QString("<b>Language:</b> %1<br>").arg(entry.language);
            preview += QString("<b>Model:</b> %1<br>").arg(entry.model);
            if (!entry.audioFile.isEmpty()) {
                preview += QString("<b>Audio File:</b> %1<br>").arg(QFileInfo(entry.audioFile).fileName());
            }
            preview += "<br><hr><br>";
            preview += entry.text.toHtmlEscaped().replace("\n", "<br>");
            
            m_previewText->setHtml(preview);
            
            emit entrySelected(entry);
        }
    } else {
        m_previewText->clear();
    }
}

void TranscriptionHistoryWidget::onItemDoubleClicked()
{
    TranscriptionHistoryEntry* entry = getSelectedEntry();
    if (entry) {
        emit entryActivated(*entry);
    }
}

void TranscriptionHistoryWidget::onSearchTextChanged(const QString& text)
{
    m_searchText = text;
    applyFilters();
}

void TranscriptionHistoryWidget::onFilterChanged()
{
    m_selectedLanguageFilterValue = m_languageFilter->currentData().toString(); // Renamed variable
    m_fromDate = m_fromDateEdit->date();
    m_toDate = m_toDateEdit->date();
    applyFilters();
}

void TranscriptionHistoryWidget::onExportClicked()
{
    exportSelected();
}

void TranscriptionHistoryWidget::onDeleteClicked()
{
    deleteSelected();
}

void TranscriptionHistoryWidget::onClearClicked()
{
    clearHistory();
}

void TranscriptionHistoryWidget::updateStatistics()
{
    int totalCount = m_history.size();
    int filteredCount = m_filteredHistory.size();
    
    qint64 totalDuration = 0;
    for (const TranscriptionHistoryEntry& entry : m_history) {
        totalDuration += entry.duration;
    }
    
    QString stats = tr("Total: %1 transcriptions | Showing: %2 | Total duration: %3")
        .arg(totalCount)
        .arg(filteredCount)
        .arg(formatDuration(totalDuration));
    
    m_statisticsLabel->setText(stats);
}

void TranscriptionHistoryWidget::refreshList()
{
    m_historyList->clear();
    
    int index = 0;
    for (const TranscriptionHistoryEntry& entry : m_filteredHistory) {
        QString itemText = QString("%1 - %2 - %3")
            .arg(entry.timestamp.toString("yyyy-MM-dd hh:mm"))
            .arg(formatDuration(entry.duration))
            .arg(entry.text.left(100).replace('\n', ' '));
        
        QListWidgetItem* item = new QListWidgetItem(itemText, m_historyList);
        item->setData(Qt::UserRole, index++);
        
        // Add icon based on language
        if (entry.language == "en") {
            item->setIcon(QIcon(":/icons/flag-us.png"));
        } else if (entry.language == "es") {
            item->setIcon(QIcon(":/icons/flag-es.png"));
        } else if (entry.language == "fr") {
            item->setIcon(QIcon(":/icons/flag-fr.png"));
        } else if (entry.language == "de") {
            item->setIcon(QIcon(":/icons/flag-de.png"));
        } else if (entry.language == "zh") {
            item->setIcon(QIcon(":/icons/flag-cn.png"));
        } else if (entry.language == "ja") {
            item->setIcon(QIcon(":/icons/flag-jp.png"));
        }
    }
    
    updateStatistics();
}

void TranscriptionHistoryWidget::applyFilters()
{
    m_filteredHistory.clear();
    
    for (const TranscriptionHistoryEntry& entry : m_history) {
        if (matchesFilters(entry)) {
            m_filteredHistory.append(entry);
        }
    }
    
    refreshList();
}

QString TranscriptionHistoryWidget::formatDuration(int seconds) const
{
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes)
            .arg(secs, 2, 10, QChar('0'));
    }
}

QString TranscriptionHistoryWidget::formatFileSize(qint64 bytes) const
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    
    if (bytes >= GB) {
        return QString::number(bytes / double(GB), 'f', 2) + " GB";
    } else if (bytes >= MB) {
        return QString::number(bytes / double(MB), 'f', 1) + " MB";
    } else if (bytes >= KB) {
        return QString::number(bytes / double(KB), 'f', 1) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
}

bool TranscriptionHistoryWidget::matchesFilters(const TranscriptionHistoryEntry& entry) const
{
    // Check search text
    if (!m_searchText.isEmpty()) {
        if (!entry.text.contains(m_searchText, Qt::CaseInsensitive) &&
            !entry.audioFile.contains(m_searchText, Qt::CaseInsensitive)) {
            return false;
        }
    }
    
    // Check language filter
    if (!m_selectedLanguageFilterValue.isEmpty() && entry.language != m_selectedLanguageFilterValue) { // Renamed variable
        return false;
    }
    
    // Check date range
    QDate entryDate = entry.timestamp.date();
    if (entryDate < m_fromDate || entryDate > m_toDate) {
        return false;
    }
    
    return true;
}