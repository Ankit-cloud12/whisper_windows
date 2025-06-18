#pragma once

#include <QMainWindow>
#include <QString>
#include <memory>

// Forward declarations
class TranscriptionWidget;
class AudioLevelWidget;
class StatusBarWidget;
class TranscriptionHistoryWidget;
class ModelManager;
class QAction;
class QMenu;
class QToolBar;
class QLabel;
class QPushButton;
class QComboBox;
class QTimer;
class QEvent;
class QCloseEvent;
class QShowEvent;
class QDropEvent;
class QDragEnterEvent;
class TrayIcon; // Forward declaration for TrayIcon


/**
 * @brief Main application window
 * 
 * The MainWindow class provides the primary user interface for WhisperApp.
 * It manages transcription display, recording controls, and all UI interactions.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
    friend class TestMainWindowLogic; // Grant access for testing private members/slots

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit MainWindow(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~MainWindow();

signals:
    /**
     * @brief Emitted when recording starts
     */
    void recordingStarted();
    
    /**
     * @brief Emitted when recording stops
     */
    void recordingStopped();
    
    /**
     * @brief Emitted when settings change
     */
    void settingsChanged();
    
    /**
     * @brief Emitted when model selection changes
     * @param modelName New model name
     */
    void modelChanged(const QString& modelName);
    
    /**
     * @brief Emitted when language selection changes
     * @param language New language code
     */
    void languageChanged(const QString& language);
    
    /**
     * @brief Emitted when text should be typed in active window
     * @param text Text to type
     */
    void typeTextRequested(const QString& text);
    
    /**
     * @brief Emitted when tray notification is requested
     * @param title Notification title
     * @param message Notification message
     */
    void trayNotificationRequested(const QString& title, const QString& message);

public slots:
    /**
     * @brief Start recording
     */
    void startRecording();
    
    /**
     * @brief Stop recording
     */
    void stopRecording();
    
    /**
     * @brief Toggle recording state
     */
    void toggleRecording();
    
    /**
     * @brief Show settings dialog
     */
    void showSettings();
    
    /**
     * @brief Show model manager dialog
     */
    void showModelManager();
    
    /**
     * @brief Show about dialog
     */
    void showAbout();
    
    /**
     * @brief Update transcription text
     * @param text New transcription text
     */
    void updateTranscription(const QString& text);
    
    /**
     * @brief Clear transcription
     */
    void clearTranscription();
    
    /**
     * @brief Copy transcription to clipboard
     */
    void copyTranscription();
    
    /**
     * @brief Update audio level display
     * @param level Audio level (0.0 to 1.0)
     */
    void updateAudioLevel(float level);
    
    /**
     * @brief Handle transcription completion
     * @param text Transcribed text
     */
    void onTranscriptionComplete(const QString& text);
    
    /**
     * @brief Handle transcription error
     * @param error Error message
     */
    void onTranscriptionError(const QString& error);

protected:
    /**
     * @brief Handle close event
     * @param event Close event
     */
    void closeEvent(QCloseEvent* event) override;
    
    /**
     * @brief Handle show event
     * @param event Show event
     */
    void showEvent(QShowEvent* event) override;
    
    /**
     * @brief Event filter for drag and drop
     * @param obj Object
     * @param event Event
     * @return true if event was handled
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    // File menu actions
    void newFile();
    void openFile(const QString& fileName = QString());
    void saveFile();
    void saveFileAs();
    void exportTranscription();
    
    // History actions
    void showHistory();
    void showHelp();
    
    // UI update slots
    void updateRecordingTime();
    void updateRecentFilesMenu();
    void addToRecentFiles(const QString& fileName);
    void clearRecentFiles();
    void toggleFullScreen();
    void onModelChanged(int index);
    void onLanguageChanged(int index);
    void onTranscriptionChanged();
    // void applyAlwaysOnTopSetting(); // Moved to public slots
    void checkInitialDisabledState(); // For status updates

public slots: // Made applyAlwaysOnTopSetting public for testing
    void applyAlwaysOnTopSetting();

private:
    /**
     * @brief Set up the user interface
     */
    void setupUI();
    
    /**
     * @brief Create all actions
     */
    void createActions();
    
    /**
     * @brief Create menus
     */
    void createMenus();
    
    /**
     * @brief Create toolbars
     */
    void createToolBars();
    
    /**
     * @brief Create dock windows
     */
    void createDockWindows();
    
    /**
     * @brief Create status bar
     */
    void createStatusBar();
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Update recording state UI
     */
    void updateRecordingState();
    
    /**
     * @brief Update record button state
     */
    void updateRecordButtonState();
    
    /**
     * @brief Update status bar
     */
    void updateStatusBar();
    
    /**
     * @brief Save window state
     */
    void saveWindowState();
    
    /**
     * @brief Restore window state
     */
    void restoreWindowState();
    
    /**
     * @brief Populate model combo box
     */
    void populateModelCombo();
    
    /**
     * @brief Populate language combo box
     */
    void populateLanguageCombo();

private:
    // UI Components
    TranscriptionWidget* m_transcriptionWidget;
    AudioLevelWidget* m_audioLevelWidget;
    StatusBarWidget* m_statusBarWidget;
    TranscriptionHistoryWidget* m_historyWidget;
    QPushButton* m_recordButton;
    QLabel* m_recordingTimeLabel;
    QLabel* m_statusLabel;
    QLabel* m_modelStatusLabel;
    QLabel* m_deviceStatusLabel;
    QComboBox* m_modelCombo;
    QComboBox* m_languageCombo;
    
    // Menus
    QMenu* m_viewMenu;
    QMenu* m_recentFilesMenu;
    
    // Actions - File
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_exportAction;
    QAction* m_exitAction;
    
    // Actions - Edit
    QAction* m_undoAction;
    QAction* m_redoAction;
    QAction* m_cutAction;
    QAction* m_copyAction;
    QAction* m_pasteAction;
    QAction* m_selectAllAction;
    QAction* m_findAction;
    QAction* m_replaceAction;
    
    // Actions - View
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_zoomResetAction;
    QAction* m_showTimestampsAction;
    QAction* m_wordWrapAction;
    QAction* m_fullScreenAction;
    
    // Actions - Tools
    QAction* m_recordAction;
    QAction* m_settingsAction;
    QAction* m_modelManagerAction;
    QAction* m_historyAction;
    
    // Actions - Help
    QAction* m_helpAction;
    QAction* m_aboutAction;
    QAction* m_aboutQtAction;
    
    // Core components
    ModelManager* m_modelManager;
    
    // State
    bool m_recording;
    QString m_currentFile;
    
    // Timer
    QTimer* m_recordingTimer;
    int m_recordingDuration;

    // System Tray Icon
    TrayIcon* m_trayIcon = nullptr;

    // Processing Visual Cue
    QLabel* m_processingSpinner = nullptr;
};