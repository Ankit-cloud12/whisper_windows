#pragma once

#include <QWidget>
#include <QDateTime>
#include <QString>
#include <QList>

// Forward declarations
class QListWidget;
class QTextEdit;
class QPushButton;
class QLineEdit;
class QComboBox;
class QDateEdit;

/**
 * @brief History entry for a transcription
 */
struct TranscriptionHistoryEntry {
    QString id;
    QString text;
    QString audioFile;
    QDateTime timestamp;
    int duration; // in seconds
    QString language;
    QString model;
};

/**
 * @brief Widget for viewing and managing transcription history
 * 
 * Displays past transcriptions with search, filter, and export capabilities.
 */
class TranscriptionHistoryWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit TranscriptionHistoryWidget(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~TranscriptionHistoryWidget();
    
    /**
     * @brief Add a new history entry
     * @param entry History entry
     */
    void addEntry(const TranscriptionHistoryEntry& entry);
    
    /**
     * @brief Load history from storage
     */
    void loadHistory();
    
    /**
     * @brief Save history to storage
     */
    void saveHistory();
    
    /**
     * @brief Clear all history
     */
    void clearHistory();
    
    /**
     * @brief Get selected entry
     * @return Selected entry or nullptr
     */
    TranscriptionHistoryEntry* getSelectedEntry();

signals:
    /**
     * @brief Emitted when an entry is selected
     * @param entry Selected entry
     */
    void entrySelected(const TranscriptionHistoryEntry& entry);
    
    /**
     * @brief Emitted when an entry is double-clicked
     * @param entry Clicked entry
     */
    void entryActivated(const TranscriptionHistoryEntry& entry);
    
    /**
     * @brief Emitted when history is modified
     */
    void historyModified();

public slots:
    /**
     * @brief Search history
     * @param text Search text
     */
    void search(const QString& text);
    
    /**
     * @brief Filter by date range
     * @param from Start date
     * @param to End date
     */
    void filterByDate(const QDate& from, const QDate& to);
    
    /**
     * @brief Filter by language
     * @param language Language code
     */
    void filterByLanguage(const QString& language);
    
    /**
     * @brief Export selected entries
     */
    void exportSelected();
    
    /**
     * @brief Delete selected entries
     */
    void deleteSelected();

private slots:
    void onSelectionChanged();
    void onItemDoubleClicked();
    void onSearchTextChanged(const QString& text);
    void onFilterChanged();
    void onExportClicked();
    void onDeleteClicked();
    void onClearClicked();
    void updateStatistics();

private:
    void setupUI();
    void connectSignals();
    void refreshList();
    void applyFilters();
    QString formatDuration(int seconds) const;
    QString formatFileSize(qint64 bytes) const;
    bool matchesFilters(const TranscriptionHistoryEntry& entry) const;

private:
    // UI elements
    QListWidget* m_historyList;
    QTextEdit* m_previewText;
    QLineEdit* m_searchEdit;
    QComboBox* m_languageFilter;
    QDateEdit* m_fromDateEdit;
    QDateEdit* m_toDateEdit;
    QPushButton* m_exportButton;
    QPushButton* m_deleteButton;
    QPushButton* m_clearButton;
    QLabel* m_statisticsLabel;
    
    // Data
    QList<TranscriptionHistoryEntry> m_history;
    QList<TranscriptionHistoryEntry> m_filteredHistory;
    
    // Filter state
    QString m_searchText;
    QString m_languageFilter;
    QDate m_fromDate;
    QDate m_toDate;
};