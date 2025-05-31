#include "MainWindow.h"
#include "TranscriptionWidget.h"
#include "AudioLevelWidget.h"
#include "SettingsDialog.h"
#include "ModelDownloader.h"
#include "AboutDialog.h"
#include "StatusBarWidget.h"
#include "TranscriptionHistoryWidget.h"
#include "../core/Settings.h"
#include "../core/Logger.h"
#include <iostream>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QDockWidget>
#include <QSettings>
#include <QCloseEvent>
#include <QApplication>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QActionGroup>
#include <QComboBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QUuid>
#include <QClipboard>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_recording(false)
    , m_transcriptionWidget(nullptr)
    , m_audioLevelWidget(nullptr)
    , m_recordButton(nullptr)
    , m_statusBarWidget(nullptr)
    , m_recordingTimer(nullptr)
    , m_recordingDuration(0)
    , m_historyWidget(nullptr)
{
    setupUI();
    createActions();
    createMenus();
    createToolBars();
    createDockWindows();
    createStatusBar();
    connectSignals();
    restoreWindowState();
    
    Logger::instance().log(Logger::LogLevel::Info, "MainWindow", "Main window initialized");
}

MainWindow::~MainWindow()
{
    saveWindowState();
    Logger::instance().log(Logger::LogLevel::Info, "MainWindow", "Main window destroyed");
}

void MainWindow::setupUI()
{
    setWindowTitle("WhisperApp - Speech to Text");
    setWindowIcon(QIcon(":/icons/app.ico"));
    resize(1000, 700);
    
    // Create central widget with main layout
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create control panel
    QWidget* controlPanel = new QWidget;
    QHBoxLayout* controlLayout = new QHBoxLayout(controlPanel);
    controlLayout->setContentsMargins(10, 10, 10, 10);
    
    // Record button
    m_recordButton = new QPushButton(this);
    m_recordButton->setIconSize(QSize(32, 32));
    m_recordButton->setMinimumHeight(50);
    m_recordButton->setCheckable(true);
    m_recordButton->setToolTip(tr("Click to start or stop recording (Ctrl+R)"));
    updateRecordButtonState();
    
    // Audio level widget
    m_audioLevelWidget = new AudioLevelWidget(this);
    m_audioLevelWidget->setMinimumHeight(30);
    m_audioLevelWidget->setMaximumHeight(30);
    m_audioLevelWidget->setToolTip(tr("Real-time audio level indicator"));
    
    // Recording time label
    m_recordingTimeLabel = new QLabel("00:00", this);
    m_recordingTimeLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; }");
    m_recordingTimeLabel->setVisible(false);
    
    // Model selection combo
    m_modelCombo = new QComboBox(this);
    m_modelCombo->setMinimumWidth(150);
    m_modelCombo->setToolTip(tr("Select the Whisper model for transcription\nLarger models are more accurate but slower"));
    populateModelCombo();
    
    // Language selection combo
    m_languageCombo = new QComboBox(this);
    m_languageCombo->setMinimumWidth(100);
    m_languageCombo->setToolTip(tr("Select the language for transcription\nAuto will detect the language automatically"));
    populateLanguageCombo();
    
    controlLayout->addWidget(new QLabel("Model:", this));
    controlLayout->addWidget(m_modelCombo);
    controlLayout->addWidget(new QLabel("Language:", this));
    controlLayout->addWidget(m_languageCombo);
    controlLayout->addStretch();
    controlLayout->addWidget(m_recordingTimeLabel);
    controlLayout->addWidget(m_recordButton);
    controlLayout->addWidget(m_audioLevelWidget);
    
    // Create transcription widget
    m_transcriptionWidget = new TranscriptionWidget(this);
    
    // Add to main layout
    mainLayout->addWidget(controlPanel);
    mainLayout->addWidget(m_transcriptionWidget, 1);
    
    // Recording timer
    m_recordingTimer = new QTimer(this);
    m_recordingTimer->setInterval(1000);
    
    Logger::instance().log(Logger::LogLevel::Debug, "MainWindow", "UI setup complete");
}

void MainWindow::createActions()
{
    // File actions
    m_newAction = new QAction(QIcon(":/icons/new.png"), tr("&New"), this);
    m_newAction->setShortcut(QKeySequence::New);
    m_newAction->setStatusTip(tr("Clear transcription"));
    m_newAction->setToolTip(tr("Clear the current transcription"));
    
    m_openAction = new QAction(QIcon(":/icons/open.png"), tr("&Open..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip(tr("Open transcription file"));
    m_openAction->setToolTip(tr("Open an existing transcription file"));
    
    m_saveAction = new QAction(QIcon(":/icons/save.png"), tr("&Save"), this);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setStatusTip(tr("Save transcription"));
    m_saveAction->setToolTip(tr("Save the current transcription"));
    
    m_saveAsAction = new QAction(tr("Save &As..."), this);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    m_saveAsAction->setStatusTip(tr("Save transcription as"));
    m_saveAsAction->setToolTip(tr("Save the transcription with a new name"));
    
    m_exportAction = new QAction(tr("&Export..."), this);
    m_exportAction->setShortcut(tr("Ctrl+E"));
    m_exportAction->setStatusTip(tr("Export transcription"));
    m_exportAction->setToolTip(tr("Export transcription to different formats"));
    
    m_exitAction = new QAction(tr("E&xit"), this);
    m_exitAction->setShortcut(tr("Ctrl+Q"));
    m_exitAction->setStatusTip(tr("Exit the application"));
    
    // Edit actions
    m_undoAction = new QAction(QIcon(":/icons/undo.png"), tr("&Undo"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setStatusTip(tr("Undo last action"));
    m_undoAction->setToolTip(tr("Undo the last editing action"));
    
    m_redoAction = new QAction(QIcon(":/icons/redo.png"), tr("&Redo"), this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setStatusTip(tr("Redo last action"));
    m_redoAction->setToolTip(tr("Redo the last undone action"));
    
    m_cutAction = new QAction(QIcon(":/icons/cut.png"), tr("Cu&t"), this);
    m_cutAction->setShortcut(QKeySequence::Cut);
    m_cutAction->setStatusTip(tr("Cut selected text"));
    
    m_copyAction = new QAction(QIcon(":/icons/copy.png"), tr("&Copy"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setStatusTip(tr("Copy selected text"));
    
    m_pasteAction = new QAction(QIcon(":/icons/paste.png"), tr("&Paste"), this);
    m_pasteAction->setShortcut(QKeySequence::Paste);
    m_pasteAction->setStatusTip(tr("Paste from clipboard"));
    
    m_selectAllAction = new QAction(tr("Select &All"), this);
    m_selectAllAction->setShortcut(QKeySequence::SelectAll);
    m_selectAllAction->setStatusTip(tr("Select all text"));
    
    m_findAction = new QAction(QIcon(":/icons/find.png"), tr("&Find..."), this);
    m_findAction->setShortcut(QKeySequence::Find);
    m_findAction->setStatusTip(tr("Find text"));
    
    m_replaceAction = new QAction(tr("&Replace..."), this);
    m_replaceAction->setShortcut(tr("Ctrl+H"));
    m_replaceAction->setStatusTip(tr("Find and replace text"));
    
    // View actions
    m_zoomInAction = new QAction(QIcon(":/icons/zoom-in.png"), tr("Zoom &In"), this);
    m_zoomInAction->setShortcut(QKeySequence::ZoomIn);
    m_zoomInAction->setStatusTip(tr("Zoom in"));
    
    m_zoomOutAction = new QAction(QIcon(":/icons/zoom-out.png"), tr("Zoom &Out"), this);
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    m_zoomOutAction->setStatusTip(tr("Zoom out"));
    
    m_zoomResetAction = new QAction(tr("&Reset Zoom"), this);
    m_zoomResetAction->setShortcut(tr("Ctrl+0"));
    m_zoomResetAction->setStatusTip(tr("Reset zoom to default"));
    
    m_showTimestampsAction = new QAction(tr("Show &Timestamps"), this);
    m_showTimestampsAction->setCheckable(true);
    m_showTimestampsAction->setChecked(true);
    m_showTimestampsAction->setStatusTip(tr("Show or hide timestamps"));
    
    m_wordWrapAction = new QAction(tr("&Word Wrap"), this);
    m_wordWrapAction->setCheckable(true);
    m_wordWrapAction->setChecked(true);
    m_wordWrapAction->setStatusTip(tr("Enable or disable word wrap"));
    
    m_fullScreenAction = new QAction(tr("&Full Screen"), this);
    m_fullScreenAction->setShortcut(tr("F11"));
    m_fullScreenAction->setCheckable(true);
    m_fullScreenAction->setStatusTip(tr("Toggle full screen mode"));
    
    // Tools actions
    m_recordAction = new QAction(QIcon(":/icons/record.png"), tr("&Record"), this);
    m_recordAction->setShortcut(tr("Ctrl+R"));
    m_recordAction->setCheckable(true);
    m_recordAction->setStatusTip(tr("Start/stop recording"));
    m_recordAction->setToolTip(tr("Toggle audio recording on/off"));
    
    m_settingsAction = new QAction(QIcon(":/icons/settings.png"), tr("&Settings..."), this);
    m_settingsAction->setShortcut(tr("Ctrl+,"));
    m_settingsAction->setStatusTip(tr("Open settings dialog"));
    m_settingsAction->setToolTip(tr("Configure application settings"));
    
    m_modelManagerAction = new QAction(tr("&Model Manager..."), this);
    m_modelManagerAction->setStatusTip(tr("Manage Whisper models"));
    m_modelManagerAction->setToolTip(tr("Download and manage Whisper AI models"));
    
    m_historyAction = new QAction(tr("&History..."), this);
    m_historyAction->setShortcut(tr("Ctrl+H"));
    m_historyAction->setStatusTip(tr("View transcription history"));
    
    // Help actions
    m_helpAction = new QAction(QIcon(":/icons/help.png"), tr("&Help"), this);
    m_helpAction->setShortcut(QKeySequence::HelpContents);
    m_helpAction->setStatusTip(tr("Show help"));
    
    m_aboutAction = new QAction(tr("&About WhisperApp"), this);
    m_aboutAction->setStatusTip(tr("Show information about WhisperApp"));
    
    m_aboutQtAction = new QAction(tr("About &Qt"), this);
    m_aboutQtAction->setStatusTip(tr("Show information about Qt"));
}

void MainWindow::createMenus()
{
    QMenuBar* menuBar = this->menuBar();
    
    // File menu
    QMenu* fileMenu = menuBar->addMenu(tr("&File"));
    fileMenu->addAction(m_newAction);
    fileMenu->addAction(m_openAction);
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_saveAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exportAction);
    fileMenu->addSeparator();
    m_recentFilesMenu = fileMenu->addMenu(tr("Recent Files"));
    updateRecentFilesMenu();
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);
    
    // Edit menu
    QMenu* editMenu = menuBar->addMenu(tr("&Edit"));
    editMenu->addAction(m_undoAction);
    editMenu->addAction(m_redoAction);
    editMenu->addSeparator();
    editMenu->addAction(m_cutAction);
    editMenu->addAction(m_copyAction);
    editMenu->addAction(m_pasteAction);
    editMenu->addSeparator();
    editMenu->addAction(m_selectAllAction);
    editMenu->addSeparator();
    editMenu->addAction(m_findAction);
    editMenu->addAction(m_replaceAction);
    
    // View menu
    m_viewMenu = menuBar->addMenu(tr("&View"));
    m_viewMenu->addAction(m_zoomInAction);
    m_viewMenu->addAction(m_zoomOutAction);
    m_viewMenu->addAction(m_zoomResetAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_showTimestampsAction);
    m_viewMenu->addAction(m_wordWrapAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_fullScreenAction);
    
    // Tools menu
    QMenu* toolsMenu = menuBar->addMenu(tr("&Tools"));
    toolsMenu->addAction(m_recordAction);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_settingsAction);
    toolsMenu->addAction(m_modelManagerAction);
    toolsMenu->addAction(m_historyAction);
    
    // Help menu
    QMenu* helpMenu = menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(m_helpAction);
    helpMenu->addSeparator();
    helpMenu->addAction(m_aboutAction);
    helpMenu->addAction(m_aboutQtAction);
}

void MainWindow::createToolBars()
{
    // Main toolbar
    QToolBar* mainToolBar = addToolBar(tr("Main"));
    mainToolBar->setObjectName("MainToolBar");
    mainToolBar->setIconSize(QSize(24, 24));
    
    mainToolBar->addAction(m_newAction);
    mainToolBar->addAction(m_openAction);
    mainToolBar->addAction(m_saveAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_cutAction);
    mainToolBar->addAction(m_copyAction);
    mainToolBar->addAction(m_pasteAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_undoAction);
    mainToolBar->addAction(m_redoAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_recordAction);
    
    // Format toolbar
    QToolBar* formatToolBar = addToolBar(tr("Format"));
    formatToolBar->setObjectName("FormatToolBar");
    
    formatToolBar->addAction(m_zoomInAction);
    formatToolBar->addAction(m_zoomOutAction);
    formatToolBar->addSeparator();
    formatToolBar->addAction(m_findAction);
}

void MainWindow::createDockWindows()
{
    // Audio levels dock
    QDockWidget* audioDock = new QDockWidget(tr("Audio Levels"), this);
    audioDock->setObjectName("AudioDock");
    audioDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    QWidget* audioWidget = new QWidget;
    QVBoxLayout* audioLayout = new QVBoxLayout(audioWidget);
    
    // Add multiple audio level widgets for different frequencies
    for (int i = 0; i < 5; ++i) {
        AudioLevelWidget* levelWidget = new AudioLevelWidget(this);
        audioLayout->addWidget(new QLabel(QString("%1 Hz").arg(100 * (i + 1))));
        audioLayout->addWidget(levelWidget);
    }
    audioLayout->addStretch();
    
    audioDock->setWidget(audioWidget);
    addDockWidget(Qt::RightDockWidgetArea, audioDock);
    
    // View menu additions for docks
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(audioDock->toggleViewAction());
    
    // History dock
    QDockWidget* historyDock = new QDockWidget(tr("Transcription History"), this);
    historyDock->setObjectName("HistoryDock");
    historyDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    
    m_historyWidget = new TranscriptionHistoryWidget(this);
    historyDock->setWidget(m_historyWidget);
    addDockWidget(Qt::BottomDockWidgetArea, historyDock);
    
    m_viewMenu->addAction(historyDock->toggleViewAction());
}

void MainWindow::createStatusBar()
{
    m_statusBarWidget = new StatusBarWidget(this);
    statusBar()->addPermanentWidget(m_statusBarWidget);
    
    // Add additional status widgets
    m_statusLabel = new QLabel(tr("Ready"), this);
    statusBar()->addWidget(m_statusLabel);
    
    // Add permanent widgets
    m_modelStatusLabel = new QLabel(this);
    m_modelStatusLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusBar()->addPermanentWidget(m_modelStatusLabel);
    
    m_deviceStatusLabel = new QLabel(this);
    m_deviceStatusLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusBar()->addPermanentWidget(m_deviceStatusLabel);
    
    updateStatusBar();
}

void MainWindow::connectSignals()
{
    // File actions
    connect(m_newAction, &QAction::triggered, this, &MainWindow::newFile);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(m_exportAction, &QAction::triggered, this, &MainWindow::exportTranscription);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    
    // Edit actions
    connect(m_undoAction, &QAction::triggered, m_transcriptionWidget, &TranscriptionWidget::undo);
    connect(m_redoAction, &QAction::triggered, m_transcriptionWidget, &TranscriptionWidget::redo);
    connect(m_cutAction, &QAction::triggered, m_transcriptionWidget, &TranscriptionWidget::cut);
    connect(m_copyAction, &QAction::triggered, m_transcriptionWidget, &TranscriptionWidget::copy);
    connect(m_pasteAction, &QAction::triggered, m_transcriptionWidget, &TranscriptionWidget::paste);
    connect(m_selectAllAction, &QAction::triggered, m_transcriptionWidget, &TranscriptionWidget::selectAll);
    connect(m_findAction, &QAction::triggered, m_transcriptionWidget, &TranscriptionWidget::showFindDialog);
    connect(m_replaceAction, &QAction::triggered, m_transcriptionWidget, &TranscriptionWidget::showReplaceDialog);
    
    // View actions
    connect(m_zoomInAction, &QAction::triggered, m_transcriptionWidget, &TranscriptionWidget::zoomIn);
    connect(m_zoomOutAction, &QAction::triggered, m_transcriptionWidget, &TranscriptionWidget::zoomOut);
    connect(m_zoomResetAction, &QAction::triggered, m_transcriptionWidget, &TranscriptionWidget::zoomReset);
    connect(m_showTimestampsAction, &QAction::toggled, m_transcriptionWidget, &TranscriptionWidget::setShowTimestamps);
    connect(m_wordWrapAction, &QAction::toggled, m_transcriptionWidget, &TranscriptionWidget::setWordWrap);
    connect(m_fullScreenAction, &QAction::toggled, this, &MainWindow::toggleFullScreen);
    
    // Tools actions
    connect(m_recordAction, &QAction::toggled, this, &MainWindow::toggleRecording);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
    connect(m_modelManagerAction, &QAction::triggered, this, &MainWindow::showModelManager);
    connect(m_historyAction, &QAction::triggered, this, &MainWindow::showHistory);
    
    // Help actions
    connect(m_helpAction, &QAction::triggered, this, &MainWindow::showHelp);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    connect(m_aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
    
    // UI controls
    connect(m_recordButton, &QPushButton::toggled, this, &MainWindow::toggleRecording);
    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onModelChanged);
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLanguageChanged);
    
    // Transcription widget signals
    connect(m_transcriptionWidget, &TranscriptionWidget::textChanged, 
            this, &MainWindow::onTranscriptionChanged);
    connect(m_transcriptionWidget, &TranscriptionWidget::undoAvailable,
            m_undoAction, &QAction::setEnabled);
    connect(m_transcriptionWidget, &TranscriptionWidget::redoAvailable,
            m_redoAction, &QAction::setEnabled);
    connect(m_transcriptionWidget, &TranscriptionWidget::copyAvailable,
            m_copyAction, &QAction::setEnabled);
    connect(m_transcriptionWidget, &TranscriptionWidget::copyAvailable,
            m_cutAction, &QAction::setEnabled);
    
    // Timer
    connect(m_recordingTimer, &QTimer::timeout, this, &MainWindow::updateRecordingTime);
    
    // History widget connections
    if (m_historyWidget) {
        connect(m_historyWidget, &TranscriptionHistoryWidget::entrySelected,
                [this](const TranscriptionHistoryEntry& entry) {
            // Update main transcription widget with selected entry
            m_transcriptionWidget->setText(entry.text);
        });
        
        connect(m_historyWidget, &TranscriptionHistoryWidget::entryActivated,
                [this](const TranscriptionHistoryEntry& entry) {
            // Load the full transcription
            m_transcriptionWidget->setText(entry.text);
            m_currentFile = entry.audioFile;
        });
    }
    
    // Status bar widget connections
    if (m_statusBarWidget) {
        connect(m_statusBarWidget, &StatusBarWidget::modelInfoClicked,
                this, &MainWindow::showModelManager);
        connect(m_statusBarWidget, &StatusBarWidget::deviceInfoClicked,
                this, &MainWindow::showSettings);
        connect(m_statusBarWidget, &StatusBarWidget::networkInfoClicked,
                [this]() {
            // Show network status info
            QMessageBox::information(this, tr("Network Status"),
                tr("Network connectivity is required for downloading models and updates."));
        });
    }
    
    Logger::instance().log(Logger::LogLevel::Debug, "MainWindow", "Signals connected");
}

void MainWindow::startRecording()
{
    if (m_recording) {
        return;
    }
    
    m_recording = true;
    m_recordingDuration = 0;
    updateRecordingState();
    
    emit recordingStarted();
    
    Logger::instance().log(Logger::LogLevel::Info, "MainWindow", "Started recording");
    m_statusLabel->setText(tr("Recording..."));
    
    // Update status bar widget
    if (m_statusBarWidget) {
        m_statusBarWidget->setRecordingStatus(true, 0);
    }
}

void MainWindow::stopRecording()
{
    if (!m_recording) {
        return;
    }
    
    m_recording = false;
    updateRecordingState();
    
    emit recordingStopped();
    
    Logger::instance().log(Logger::LogLevel::Info, "MainWindow", "Stopped recording");
    m_statusLabel->setText(tr("Processing..."));
    
    // Update status bar widget
    if (m_statusBarWidget) {
        m_statusBarWidget->setRecordingStatus(false, m_recordingDuration);
    }
}

void MainWindow::toggleRecording()
{
    if (m_recording) {
        stopRecording();
    } else {
        startRecording();
    }
}

void MainWindow::showSettings()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // Apply settings changes
        emit settingsChanged();
        updateStatusBar();
        Logger::instance().log(Logger::LogLevel::Info, "MainWindow", "Settings updated");
    }
}

void MainWindow::showModelManager()
{
    ModelDownloader dialog(this);
    dialog.exec();
    
    // Refresh model list after dialog closes
    populateModelCombo();
}

void MainWindow::showAbout()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::showHistory()
{
    // Show or focus the history dock
    QDockWidget* historyDock = findChild<QDockWidget*>("HistoryDock");
    if (historyDock) {
        historyDock->show();
        historyDock->raise();
        if (m_historyWidget) {
            m_historyWidget->setFocus();
        }
    }
}

void MainWindow::showHelp()
{
    // TODO: Implement help viewer
    QMessageBox::information(this, tr("Help"), tr("Help documentation coming soon!"));
}

void MainWindow::newFile()
{
    if (m_transcriptionWidget->isModified()) {
        int ret = QMessageBox::warning(this, tr("WhisperApp"),
                     tr("The transcription has been modified.\n"
                        "Do you want to save your changes?"),
                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (ret == QMessageBox::Save) {
            saveFile();
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }
    
    m_transcriptionWidget->clear();
    m_currentFile.clear();
    setWindowTitle(tr("WhisperApp - Speech to Text"));
}

void MainWindow::openFile(const QString& fileName)
{
    QString fileToOpen = fileName;
    
    if (fileToOpen.isEmpty()) {
        fileToOpen = QFileDialog::getOpenFileName(this,
            tr("Open Transcription"), "",
            tr("Text Files (*.txt);;All Files (*)"));
    }
    
    if (!fileToOpen.isEmpty()) {
        if (m_transcriptionWidget->loadFile(fileToOpen)) {
            m_currentFile = fileToOpen;
            setWindowTitle(tr("WhisperApp - %1").arg(QFileInfo(fileToOpen).fileName()));
            addToRecentFiles(fileToOpen);
        }
    }
}

void MainWindow::saveFile()
{
    if (m_currentFile.isEmpty()) {
        saveFileAs();
    } else {
        if (m_transcriptionWidget->saveFile(m_currentFile)) {
            m_statusLabel->setText(tr("File saved"));
        }
    }
}

void MainWindow::saveFileAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Transcription"), "",
        tr("Text Files (*.txt);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        if (m_transcriptionWidget->saveFile(fileName)) {
            m_currentFile = fileName;
            setWindowTitle(tr("WhisperApp - %1").arg(QFileInfo(fileName).fileName()));
            addToRecentFiles(fileName);
            m_statusLabel->setText(tr("File saved"));
        }
    }
}

void MainWindow::exportTranscription()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Transcription"), "",
        tr("HTML Files (*.html);;Markdown Files (*.md);;PDF Files (*.pdf)"));
    
    if (!fileName.isEmpty()) {
        // TODO: Implement export functionality
        QMessageBox::information(this, tr("Export"), tr("Export functionality coming soon!"));
    }
}

void MainWindow::updateTranscription(const QString& text)
{
    m_transcriptionWidget->appendText(text);
    Logger::instance().log(Logger::LogLevel::Debug, "MainWindow", 
                          QString("Transcription updated: %1").arg(text).toStdString());
}

void MainWindow::clearTranscription()
{
    m_transcriptionWidget->clear();
    Logger::instance().log(Logger::LogLevel::Debug, "MainWindow", "Transcription cleared");
}

void MainWindow::copyTranscription()
{
    m_transcriptionWidget->copy();
    Logger::instance().log(Logger::LogLevel::Debug, "MainWindow", "Transcription copied to clipboard");
}

void MainWindow::updateRecordingState()
{
    if (m_recording) {
        m_recordButton->setText(tr("Stop Recording"));
        m_recordButton->setIcon(QIcon(":/icons/stop.png"));
        m_recordAction->setText(tr("&Stop Recording"));
        m_recordAction->setIcon(QIcon(":/icons/stop.png"));
        m_recordingTimeLabel->setVisible(true);
        m_recordingDuration = 0;
        m_recordingTimer->start();
        
        // Disable certain actions during recording
        m_newAction->setEnabled(false);
        m_openAction->setEnabled(false);
        
        Logger::instance().log(Logger::LogLevel::Debug, "MainWindow", "UI updated for recording state");
    } else {
        m_recordButton->setText(tr("Start Recording"));
        m_recordButton->setIcon(QIcon(":/icons/record.png"));
        m_recordAction->setText(tr("&Record"));
        m_recordAction->setIcon(QIcon(":/icons/record.png"));
        m_recordingTimeLabel->setVisible(false);
        m_recordingTimer->stop();
        
        // Re-enable actions
        m_newAction->setEnabled(true);
        m_openAction->setEnabled(true);
        
        Logger::instance().log(Logger::LogLevel::Debug, "MainWindow", "UI updated for idle state");
    }
    
    updateRecordButtonState();
}

void MainWindow::updateRecordButtonState()
{
    m_recordButton->setChecked(m_recording);
    m_recordAction->setChecked(m_recording);
}

void MainWindow::updateStatusBar()
{
    // Update model status
    Settings& settings = Settings::instance();
    QString modelName = settings.getSetting(Settings::Key::Model).toString();
    m_modelStatusLabel->setText(tr("Model: %1").arg(modelName));
    
    // Update device status
    QString deviceName = settings.getSetting(Settings::Key::InputDevice).toString();
    m_deviceStatusLabel->setText(tr("Device: %1").arg(deviceName.isEmpty() ? tr("Default") : deviceName));
    
    // Update main status
    if (m_recording) {
        m_statusLabel->setText(tr("Recording..."));
    } else {
        m_statusLabel->setText(tr("Ready"));
    }
    
    // Update status bar widget
    if (m_statusBarWidget) {
        m_statusBarWidget->setModelStatus(modelName, !modelName.isEmpty());
        m_statusBarWidget->setDeviceStatus(deviceName.isEmpty() ? tr("Default") : deviceName, true);
        m_statusBarWidget->setNetworkStatus(true, 0); // Assume online for now
    }
}

void MainWindow::updateRecordingTime()
{
    m_recordingDuration++;
    int minutes = m_recordingDuration / 60;
    int seconds = m_recordingDuration % 60;
    m_recordingTimeLabel->setText(QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0')));
    
    // Update status bar widget
    if (m_statusBarWidget) {
        m_statusBarWidget->setRecordingStatus(true, m_recordingDuration);
    }
}

void MainWindow::updateRecentFilesMenu()
{
    m_recentFilesMenu->clear();
    
    Settings& settings = Settings::instance();
    QStringList recentFiles = settings.getSetting(Settings::Key::RecentFiles).toStringList();
    
    if (recentFiles.isEmpty()) {
        m_recentFilesMenu->addAction(tr("No recent files"))->setEnabled(false);
        return;
    }
    
    for (int i = 0; i < recentFiles.size() && i < 10; ++i) {
        QString fileName = QFileInfo(recentFiles[i]).fileName();
        QAction* action = m_recentFilesMenu->addAction(QString("&%1 %2").arg(i + 1).arg(fileName));
        action->setData(recentFiles[i]);
        connect(action, &QAction::triggered, this, [this, action]() {
            openFile(action->data().toString());
        });
    }
    
    m_recentFilesMenu->addSeparator();
    m_recentFilesMenu->addAction(tr("Clear Recent Files"), this, &MainWindow::clearRecentFiles);
}

void MainWindow::addToRecentFiles(const QString& fileName)
Settings& settings = Settings::instance();
    QStringList recentFiles = settings.getSetting(Settings::Key::RecentFiles).toStringList();
    
    // Remove if already exists
    recentFiles.removeAll(fileName);
    
    // Add to front
    recentFiles.prepend(fileName);
    
    // Keep only last 10
    while (recentFiles.size() > 10) {
        recentFiles.removeLast();
    }
    
    settings.setSetting(Settings::Key::RecentFiles, recentFiles);
    updateRecentFilesMenu();
}

void MainWindow::clearRecentFiles()
{
    Settings& settings = Settings::instance();
    settings.setSetting(Settings::Key::RecentFiles, QStringList());
    updateRecentFilesMenu();
}

void MainWindow::saveWindowState()
{
    QSettings settings("WhisperApp", "MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("fullScreen", isFullScreen());
    
    Logger::instance().log(Logger::LogLevel::Debug, "MainWindow", "Window state saved");
}

void MainWindow::restoreWindowState()
{
    QSettings settings("WhisperApp", "MainWindow");
    
    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
    }
    
    if (settings.contains("windowState")) {
        restoreState(settings.value("windowState").toByteArray());
    }
    
    if (settings.value("fullScreen", false).toBool()) {
        showFullScreen();
        m_fullScreenAction->setChecked(true);
    }
    
    Logger::instance().log(Logger::LogLevel::Debug, "MainWindow", "Window state restored");
}

void MainWindow::toggleFullScreen()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void MainWindow::onModelChanged(int index)
{
    if (index < 0) return;
    
    QString modelName = m_modelCombo->itemData(index).toString();
    Settings::instance().setSetting(Settings::Key::Model, modelName);
    emit modelChanged(modelName);
    updateStatusBar();
    
    // Update status bar widget
    if (m_statusBarWidget) {
        m_statusBarWidget->setModelStatus(modelName, true);
    }
}

void MainWindow::onLanguageChanged(int index)
{
    if (index < 0) return;
    
    QString language = m_languageCombo->itemData(index).toString();
    Settings::instance().setSetting(Settings::Key::Language, language);
    emit languageChanged(language);
}

void MainWindow::onTranscriptionChanged()
{
    // Update save action state
    m_saveAction->setEnabled(m_transcriptionWidget->isModified());
}

void MainWindow::updateAudioLevel(float level)
{
    m_audioLevelWidget->setLevel(level);
    
    // Update status bar widget
    if (m_statusBarWidget) {
        m_statusBarWidget->setAudioLevel(level);
    }
}

void MainWindow::onTranscriptionComplete(const QString& text)
{
    updateTranscription(text);
    m_statusLabel->setText(tr("Transcription complete"));
    
    Settings& settings = Settings::instance();
    
    // Add to history
    if (m_historyWidget) {
        TranscriptionHistoryEntry entry;
        entry.id = QUuid::createUuid().toString();
        entry.text = text;
        entry.timestamp = QDateTime::currentDateTime();
        entry.duration = m_recordingDuration;
        entry.language = m_languageCombo->currentData().toString();
        entry.model = m_modelCombo->currentData().toString();
        entry.audioFile = m_currentFile;
        
        m_historyWidget->addEntry(entry);
    }
    
    // Copy to clipboard if enabled
    if (settings.getSetting(Settings::Key::CopyToClipboard).toBool()) {
        QApplication::clipboard()->setText(text);
    }
    
    // Type in active window if enabled
    if (settings.getSetting(Settings::Key::TypeInActiveWindow).toBool()) {
        emit typeTextRequested(text);
    }
    
    // Auto-save if enabled
    if (settings.getSetting(Settings::Key::AutoSaveTranscriptions).toBool()) {
        QString autoSavePath = settings.getSetting(Settings::Key::AutoSavePath).toString();
        if (!autoSavePath.isEmpty()) {
            QString fileName = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".txt";
            QString fullPath = QDir(autoSavePath).filePath(fileName);
            QFile file(fullPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << text;
                file.close();
            }
        }
    }
    
    Logger::instance().log(Logger::LogLevel::Info, "MainWindow",
                          QString("Transcription complete: %1").arg(text).toStdString());
}

void MainWindow::onTranscriptionError(const QString& error)
{
    m_statusLabel->setText(tr("Error: %1").arg(error));
    QMessageBox::critical(this, tr("Transcription Error"), error);
    
    Logger::instance().log(Logger::LogLevel::Error, "MainWindow", 
                          QString("Transcription error: %1").arg(error).toStdString());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    Settings& settings = Settings::instance();
    
    if (settings.getSetting(Settings::Key::MinimizeToTray).toBool()) {
        hide();
        event->ignore();
        
        if (settings.getSetting(Settings::Key::ShowTrayNotifications).toBool()) {
            emit trayNotificationRequested(tr("WhisperApp"), 
                                         tr("Application minimized to tray"));
        }
    } else {
        if (m_recording) {
            int ret = QMessageBox::warning(this, tr("WhisperApp"),
                         tr("Recording is in progress.\n"
                            "Do you want to stop recording and exit?"),
                         QMessageBox::Yes | QMessageBox::No);
            
            if (ret == QMessageBox::No) {
                event->ignore();
                return;
            }
            
            stopRecording();
        }
        
        saveWindowState();
        event->accept();
    }
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    updateStatusBar();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    // Handle drag and drop
    if (event->type() == QEvent::DragEnter) {
        QDragEnterEvent* dragEvent = static_cast<QDragEnterEvent*>(event);
        if (dragEvent->mimeData()->hasUrls()) {
            dragEvent->acceptProposedAction();
            return true;
        }
    } else if (event->type() == QEvent::Drop) {
        QDropEvent* dropEvent = static_cast<QDropEvent*>(event);
        const QMimeData* mimeData = dropEvent->mimeData();
        
        if (mimeData->hasUrls()) {
            QList<QUrl> urls = mimeData->urls();
            if (!urls.isEmpty()) {
                QString fileName = urls.first().toLocalFile();
                if (QFile::exists(fileName)) {
                    openFile(fileName);
                    return true;
                }
            }
        }
    }
    
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::populateModelCombo()
{
    m_modelCombo->clear();
    
    // Add available models
    m_modelCombo->addItem("Tiny", "tiny");
    m_modelCombo->addItem("Base", "base");
    m_modelCombo->addItem("Small", "small");
    m_modelCombo->addItem("Medium", "medium");
    m_modelCombo->addItem("Large", "large");
    
    // Set current model from settings
    Settings& settings = Settings::instance();
    QString currentModel = settings.getSetting(Settings::Key::Model).toString();
    int index = m_modelCombo->findData(currentModel);
    if (index >= 0) {
        m_modelCombo->setCurrentIndex(index);
    }
}

void MainWindow::populateLanguageCombo()
{
    m_languageCombo->clear();
    
    // Add supported languages
    m_languageCombo->addItem("Auto", "auto");
    m_languageCombo->addItem("English", "en");
    m_languageCombo->addItem("Spanish", "es");
    m_languageCombo->addItem("French", "fr");
    m_languageCombo->addItem("German", "de");
    m_languageCombo->addItem("Italian", "it");
    m_languageCombo->addItem("Portuguese", "pt");
    m_languageCombo->addItem("Russian", "ru");
    m_languageCombo->addItem("Chinese", "zh");
    m_languageCombo->addItem("Japanese", "ja");
    m_languageCombo->addItem("Korean", "ko");
    
    // Set current language from settings
    Settings& settings = Settings::instance();
    QString currentLanguage = settings.getSetting(Settings::Key::Language).toString();
    int index = m_languageCombo->findData(currentLanguage);
    if (index >= 0) {
        m_languageCombo->setCurrentIndex(index);
    } else {
        m_languageCombo->setCurrentIndex(0); // Default to auto
    }
}