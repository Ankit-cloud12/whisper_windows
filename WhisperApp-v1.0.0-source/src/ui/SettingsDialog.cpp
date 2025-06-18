#include "SettingsDialog.h"
#include "HotkeyEditWidget.h"
#include "../core/Settings.h"
#include "../core/Logger.h"
#include "../core/DeviceManager.h"
#include "../core/ModelManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFontDialog>
#include <QColorDialog>
#include <QStyle>
#include <QApplication>
#include <QProgressBar>
#include <QDesktopServices>
#include <QUrl>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUI();
    loadSettings();
    connectSignals();
    
    Logger::instance().log(Logger::LogLevel::Info, "SettingsDialog", "Settings dialog initialized");
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    setWindowTitle(tr("Settings"));
    setModal(true);
    resize(700, 500);
    
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Tab widget for settings categories
    m_tabWidget = new QTabWidget(this);
    
    // Create tabs
    createGeneralTab();
    createAudioTab();
    createModelTab();
    createHotkeyTab();
    createOutputTab();
    createAppearanceTab();
    createAdvancedTab();
    
    mainLayout->addWidget(m_tabWidget);
    
    // Button box
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply,
        this
    );
    
    m_applyButton = buttonBox->button(QDialogButtonBox::Apply);
    m_applyButton->setEnabled(false);
    
    // Add reset button
    m_resetButton = new QPushButton(tr("Reset to Defaults"), this);
    buttonBox->addButton(m_resetButton, QDialogButtonBox::ResetRole);
    
    mainLayout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &SettingsDialog::applySettings);
    connect(m_resetButton, &QPushButton::clicked, this, &SettingsDialog::resetToDefaults);
}

void SettingsDialog::createGeneralTab()
{
    QWidget* generalTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(generalTab);
    
    // Startup group
    QGroupBox* startupGroup = new QGroupBox(tr("Startup"), generalTab);
    QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);
    
    m_startMinimizedCheck = new QCheckBox(tr("Start minimized"), startupGroup);
    m_startWithWindowsCheck = new QCheckBox(tr("Start with Windows"), startupGroup);
    m_checkUpdatesCheck = new QCheckBox(tr("Check for updates on startup"), startupGroup);
    
    startupLayout->addWidget(m_startMinimizedCheck);
    startupLayout->addWidget(m_startWithWindowsCheck);
    startupLayout->addWidget(m_checkUpdatesCheck);
    
    // System tray group
    QGroupBox* trayGroup = new QGroupBox(tr("System Tray"), generalTab);
    QVBoxLayout* trayLayout = new QVBoxLayout(trayGroup);
    
    m_showTrayIconCheck = new QCheckBox(tr("Show system tray icon"), trayGroup);
    m_minimizeToTrayCheck = new QCheckBox(tr("Minimize to tray on close"), trayGroup);
    m_startInTrayCheck = new QCheckBox(tr("Start in system tray"), trayGroup);
    m_showNotificationsCheck = new QCheckBox(tr("Show tray notifications"), trayGroup);
    
    trayLayout->addWidget(m_showTrayIconCheck);
    trayLayout->addWidget(m_minimizeToTrayCheck);
    trayLayout->addWidget(m_startInTrayCheck);
    trayLayout->addWidget(m_showNotificationsCheck);
    
    // Language group
    QGroupBox* languageGroup = new QGroupBox(tr("Language"), generalTab);
    QFormLayout* languageLayout = new QFormLayout(languageGroup);
    
    m_uiLanguageCombo = new QComboBox(languageGroup);
    m_uiLanguageCombo->addItem(tr("English"), "en");
    m_uiLanguageCombo->addItem(tr("Spanish"), "es");
    m_uiLanguageCombo->addItem(tr("French"), "fr");
    m_uiLanguageCombo->addItem(tr("German"), "de");
    m_uiLanguageCombo->addItem(tr("Chinese"), "zh");
    m_uiLanguageCombo->addItem(tr("Japanese"), "ja");
    
    languageLayout->addRow(tr("Interface language:"), m_uiLanguageCombo);
    
    layout->addWidget(startupGroup);
    layout->addWidget(trayGroup);
    layout->addWidget(languageGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(generalTab, tr("General"));
}

void SettingsDialog::createAudioTab()
{
    QWidget* audioTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(audioTab);
    
    // Audio device group
    QGroupBox* deviceGroup = new QGroupBox(tr("Audio Device"), audioTab);
    QFormLayout* deviceLayout = new QFormLayout(deviceGroup);
    
    m_audioDeviceCombo = new QComboBox(deviceGroup);
    QPushButton* refreshDevicesButton = new QPushButton(tr("Refresh"), deviceGroup);
    refreshDevicesButton->setMaximumWidth(100);
    
    QHBoxLayout* deviceRow = new QHBoxLayout();
    deviceRow->addWidget(m_audioDeviceCombo);
    deviceRow->addWidget(refreshDevicesButton);
    
    deviceLayout->addRow(tr("Input device:"), deviceRow);
    
    // Audio level display
    m_audioLevelProgress = new QProgressBar(deviceGroup);
    m_audioLevelProgress->setRange(0, 100);
    m_audioLevelProgress->setTextVisible(false);
    
    QPushButton* testAudioButton = new QPushButton(tr("Test Audio"), deviceGroup);
    testAudioButton->setMaximumWidth(100);
    
    QHBoxLayout* levelRow = new QHBoxLayout();
    levelRow->addWidget(m_audioLevelProgress);
    levelRow->addWidget(testAudioButton);
    
    deviceLayout->addRow(tr("Audio level:"), levelRow);
    
    // Recording settings group
    QGroupBox* recordingGroup = new QGroupBox(tr("Recording Settings"), audioTab);
    QFormLayout* recordingLayout = new QFormLayout(recordingGroup);
    
    // Sample rate
    m_sampleRateCombo = new QComboBox(recordingGroup);
    m_sampleRateCombo->addItem("16000 Hz", 16000);
    m_sampleRateCombo->addItem("22050 Hz", 22050);
    m_sampleRateCombo->addItem("44100 Hz", 44100);
    m_sampleRateCombo->addItem("48000 Hz", 48000);
    
    // Voice activity detection
    m_vadEnabledCheck = new QCheckBox(tr("Enable voice activity detection"), recordingGroup);
    
    m_vadThresholdSlider = new QSlider(Qt::Horizontal, recordingGroup);
    m_vadThresholdSlider->setRange(0, 100);
    m_vadThresholdSlider->setTickPosition(QSlider::TicksBelow);
    m_vadThresholdSlider->setTickInterval(10);
    
    m_vadThresholdLabel = new QLabel(recordingGroup);
    
    QHBoxLayout* vadRow = new QHBoxLayout();
    vadRow->addWidget(m_vadThresholdSlider);
    vadRow->addWidget(m_vadThresholdLabel);
    
    // Noise suppression
    m_noiseSuppressCheck = new QCheckBox(tr("Enable noise suppression"), recordingGroup);
    
    recordingLayout->addRow(tr("Sample rate:"), m_sampleRateCombo);
    recordingLayout->addRow(m_vadEnabledCheck);
    recordingLayout->addRow(tr("VAD threshold:"), vadRow);
    recordingLayout->addRow(m_noiseSuppressCheck);
    
    // Connect signals
    connect(refreshDevicesButton, &QPushButton::clicked, this, &SettingsDialog::refreshAudioDevices);
    connect(testAudioButton, &QPushButton::clicked, this, &SettingsDialog::testAudioDevice);
    connect(m_vadThresholdSlider, &QSlider::valueChanged, this, [this](int value) {
        m_vadThresholdLabel->setText(QString("%1%").arg(value));
    });
    
    layout->addWidget(deviceGroup);
    layout->addWidget(recordingGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(audioTab, tr("Audio"));
}

void SettingsDialog::createModelTab()
{
    QWidget* modelTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(modelTab);
    
    // Model selection group
    QGroupBox* modelGroup = new QGroupBox(tr("Model Selection"), modelTab);
    QFormLayout* modelLayout = new QFormLayout(modelGroup);
    
    m_defaultModelCombo = new QComboBox(modelGroup);
    
    // Model info display
    m_modelInfoLabel = new QLabel(modelGroup);
    m_modelInfoLabel->setWordWrap(true);
    m_modelInfoLabel->setStyleSheet("QLabel { color: #666; }");
    
    modelLayout->addRow(tr("Default model:"), m_defaultModelCombo);
    modelLayout->addRow(tr("Model info:"), m_modelInfoLabel);
    
    // Model directory group
    QGroupBox* dirGroup = new QGroupBox(tr("Model Directory"), modelTab);
    QFormLayout* dirLayout = new QFormLayout(dirGroup);
    
    m_modelsPathEdit = new QLineEdit(dirGroup);
    m_modelsPathEdit->setReadOnly(true);
    
    QPushButton* browseButton = new QPushButton(tr("Browse..."), dirGroup);
    browseButton->setMaximumWidth(100);
    
    QHBoxLayout* pathRow = new QHBoxLayout();
    pathRow->addWidget(m_modelsPathEdit);
    pathRow->addWidget(browseButton);
    
    dirLayout->addRow(tr("Models path:"), pathRow);
    
    // Model management buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    QPushButton* downloadModelsButton = new QPushButton(tr("Download Models..."), modelTab);
    QPushButton* refreshModelsButton = new QPushButton(tr("Refresh Models"), modelTab);
    
    buttonLayout->addWidget(downloadModelsButton);
    buttonLayout->addWidget(refreshModelsButton);
    buttonLayout->addStretch();
    
    // Model-specific settings
    QGroupBox* settingsGroup = new QGroupBox(tr("Model Settings"), modelTab);
    QFormLayout* settingsLayout = new QFormLayout(settingsGroup);
    
    // Language override
    m_languageOverrideCombo = new QComboBox(settingsGroup);
    m_languageOverrideCombo->addItem(tr("Auto-detect"), "auto");
    m_languageOverrideCombo->addItem(tr("English"), "en");
    m_languageOverrideCombo->addItem(tr("Spanish"), "es");
    m_languageOverrideCombo->addItem(tr("French"), "fr");
    m_languageOverrideCombo->addItem(tr("German"), "de");
    m_languageOverrideCombo->addItem(tr("Chinese"), "zh");
    m_languageOverrideCombo->addItem(tr("Japanese"), "ja");
    
    // Translation
    m_translateCheck = new QCheckBox(tr("Enable translation to English"), settingsGroup);
    
    // Compute type
    m_computeTypeCombo = new QComboBox(settingsGroup);
    m_computeTypeCombo->addItem(tr("Auto"), "auto");
    m_computeTypeCombo->addItem(tr("CPU"), "cpu");
    m_computeTypeCombo->addItem(tr("CUDA"), "cuda");
    
    settingsLayout->addRow(tr("Language:"), m_languageOverrideCombo);
    settingsLayout->addRow(m_translateCheck);
    settingsLayout->addRow(tr("Compute type:"), m_computeTypeCombo);
    
    // Connect signals
    connect(browseButton, &QPushButton::clicked, this, &SettingsDialog::browseModelsPath);
    connect(downloadModelsButton, &QPushButton::clicked, this, &SettingsDialog::openModelManager);
    connect(refreshModelsButton, &QPushButton::clicked, this, &SettingsDialog::refreshModels);
    connect(m_defaultModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onModelChanged);
    
    layout->addWidget(modelGroup);
    layout->addWidget(dirGroup);
    layout->addLayout(buttonLayout);
    layout->addWidget(settingsGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(modelTab, tr("Models"));
}

void SettingsDialog::createHotkeyTab()
{
    QWidget* hotkeyTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(hotkeyTab);
    
    // Hotkey list
    QGroupBox* hotkeyGroup = new QGroupBox(tr("Keyboard Shortcuts"), hotkeyTab);
    QVBoxLayout* hotkeyLayout = new QVBoxLayout(hotkeyGroup);
    
    // Create hotkey editors
    QFormLayout* formLayout = new QFormLayout();
    
    m_recordHotkeyEdit = new HotkeyEditWidget(hotkeyGroup);
    m_stopHotkeyEdit = new HotkeyEditWidget(hotkeyGroup);
    m_pauseHotkeyEdit = new HotkeyEditWidget(hotkeyGroup);
    m_cancelHotkeyEdit = new HotkeyEditWidget(hotkeyGroup);
    m_toggleWindowHotkeyEdit = new HotkeyEditWidget(hotkeyGroup);
    
    formLayout->addRow(tr("Start/Stop recording:"), m_recordHotkeyEdit);
    formLayout->addRow(tr("Stop recording:"), m_stopHotkeyEdit);
    formLayout->addRow(tr("Pause/Resume recording:"), m_pauseHotkeyEdit);
    formLayout->addRow(tr("Cancel recording:"), m_cancelHotkeyEdit);
    formLayout->addRow(tr("Show/Hide window:"), m_toggleWindowHotkeyEdit);
    
    hotkeyLayout->addLayout(formLayout);
    
    // Hotkey options
    QGroupBox* optionsGroup = new QGroupBox(tr("Options"), hotkeyTab);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    
    m_globalHotkeysCheck = new QCheckBox(tr("Enable global hotkeys"), optionsGroup);
    m_hotkeyConflictLabel = new QLabel(optionsGroup);
    m_hotkeyConflictLabel->setStyleSheet("QLabel { color: red; }");
    m_hotkeyConflictLabel->setVisible(false);
    
    optionsLayout->addWidget(m_globalHotkeysCheck);
    optionsLayout->addWidget(m_hotkeyConflictLabel);
    
    // Reset hotkeys button
    QPushButton* resetHotkeysButton = new QPushButton(tr("Reset to Default Hotkeys"), hotkeyTab);
    resetHotkeysButton->setMaximumWidth(200);
    
    connect(resetHotkeysButton, &QPushButton::clicked, this, &SettingsDialog::resetHotkeys);
    
    layout->addWidget(hotkeyGroup);
    layout->addWidget(optionsGroup);
    layout->addWidget(resetHotkeysButton);
    layout->addStretch();
    
    m_tabWidget->addTab(hotkeyTab, tr("Hotkeys"));
}

void SettingsDialog::createOutputTab()
{
    QWidget* outputTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(outputTab);
    
    // Output behavior group
    QGroupBox* behaviorGroup = new QGroupBox(tr("Output Behavior"), outputTab);
    QVBoxLayout* behaviorLayout = new QVBoxLayout(behaviorGroup);
    
    m_autoCopyCheck = new QCheckBox(tr("Automatically copy transcription to clipboard"), behaviorGroup);
    m_autoTypeCheck = new QCheckBox(tr("Automatically type transcription in active window"), behaviorGroup);
    m_autoSaveCheck = new QCheckBox(tr("Automatically save transcriptions"), behaviorGroup);
    
    behaviorLayout->addWidget(m_autoCopyCheck);
    behaviorLayout->addWidget(m_autoTypeCheck);
    behaviorLayout->addWidget(m_autoSaveCheck);
    
    // Auto-save settings
    QWidget* autoSaveWidget = new QWidget(behaviorGroup);
    QFormLayout* autoSaveLayout = new QFormLayout(autoSaveWidget);
    autoSaveLayout->setContentsMargins(20, 0, 0, 0);
    
    m_autoSavePathEdit = new QLineEdit(autoSaveWidget);
    QPushButton* browseAutoSaveButton = new QPushButton(tr("Browse..."), autoSaveWidget);
    browseAutoSaveButton->setMaximumWidth(100);
    
    QHBoxLayout* autoSavePathRow = new QHBoxLayout();
    autoSavePathRow->addWidget(m_autoSavePathEdit);
    autoSavePathRow->addWidget(browseAutoSaveButton);
    
    autoSaveLayout->addRow(tr("Auto-save path:"), autoSavePathRow);
    
    behaviorLayout->addWidget(autoSaveWidget);
    
    // Output format group
    QGroupBox* formatGroup = new QGroupBox(tr("Output Format"), outputTab);
    QVBoxLayout* formatLayout = new QVBoxLayout(formatGroup);
    
    m_includeTimestampsCheck = new QCheckBox(tr("Include timestamps in output"), formatGroup);
    m_timestampFormatCombo = new QComboBox(formatGroup);
    m_timestampFormatCombo->addItem("[HH:MM:SS]", "[%H:%M:%S]");
    m_timestampFormatCombo->addItem("[MM:SS]", "[%M:%S]");
    m_timestampFormatCombo->addItem("HH:MM:SS -", "%H:%M:%S -");
    m_timestampFormatCombo->addItem("Custom...", "custom");
    
    QFormLayout* timestampLayout = new QFormLayout();
    timestampLayout->addRow(tr("Timestamp format:"), m_timestampFormatCombo);
    
    m_wordWrapCheck = new QCheckBox(tr("Word wrap long lines"), formatGroup);
    m_maxLineLengthSpin = new QSpinBox(formatGroup);
    m_maxLineLengthSpin->setRange(40, 200);
    m_maxLineLengthSpin->setSuffix(tr(" characters"));
    
    QFormLayout* wrapLayout = new QFormLayout();
    wrapLayout->addRow(tr("Maximum line length:"), m_maxLineLengthSpin);
    
    formatLayout->addWidget(m_includeTimestampsCheck);
    formatLayout->addLayout(timestampLayout);
    formatLayout->addWidget(m_wordWrapCheck);
    formatLayout->addLayout(wrapLayout);
    
    // Connect signals
    connect(m_autoSaveCheck, &QCheckBox::toggled, autoSaveWidget, &QWidget::setEnabled);
    connect(browseAutoSaveButton, &QPushButton::clicked, this, &SettingsDialog::browseAutoSavePath);
    connect(m_includeTimestampsCheck, &QCheckBox::toggled, m_timestampFormatCombo, &QComboBox::setEnabled);
    connect(m_wordWrapCheck, &QCheckBox::toggled, m_maxLineLengthSpin, &QSpinBox::setEnabled);
    
    layout->addWidget(behaviorGroup);
    layout->addWidget(formatGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(outputTab, tr("Output"));
}

void SettingsDialog::createAppearanceTab()
{
    QWidget* appearanceTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(appearanceTab);
    
    // Theme group
    QGroupBox* themeGroup = new QGroupBox(tr("Theme"), appearanceTab);
    QVBoxLayout* themeLayout = new QVBoxLayout(themeGroup);
    
    m_themeButtonGroup = new QButtonGroup(themeGroup);
    
    QRadioButton* autoThemeRadio = new QRadioButton(tr("Auto (follow system)"), themeGroup);
    QRadioButton* lightThemeRadio = new QRadioButton(tr("Light"), themeGroup);
    QRadioButton* darkThemeRadio = new QRadioButton(tr("Dark"), themeGroup);
    
    m_themeButtonGroup->addButton(autoThemeRadio, 0);
    m_themeButtonGroup->addButton(lightThemeRadio, 1);
    m_themeButtonGroup->addButton(darkThemeRadio, 2);
    
    themeLayout->addWidget(autoThemeRadio);
    themeLayout->addWidget(lightThemeRadio);
    themeLayout->addWidget(darkThemeRadio);
    
    // Font group
    QGroupBox* fontGroup = new QGroupBox(tr("Font"), appearanceTab);
    QFormLayout* fontLayout = new QFormLayout(fontGroup);
    
    m_fontLabel = new QLabel(fontGroup);
    QPushButton* changeFontButton = new QPushButton(tr("Change..."), fontGroup);
    changeFontButton->setMaximumWidth(100);
    
    QHBoxLayout* fontRow = new QHBoxLayout();
    fontRow->addWidget(m_fontLabel);
    fontRow->addWidget(changeFontButton);
    fontRow->addStretch();
    
    fontLayout->addRow(tr("Transcription font:"), fontRow);
    
    // Colors group
    QGroupBox* colorsGroup = new QGroupBox(tr("Colors"), appearanceTab);
    QFormLayout* colorsLayout = new QFormLayout(colorsGroup);
    
    m_timestampColorButton = new QPushButton(colorsGroup);
    m_timestampColorButton->setMaximumWidth(50);
    m_timestampColorButton->setFlat(true);
    m_timestampColorButton->setAutoFillBackground(true);
    
    m_speakerColorButton = new QPushButton(colorsGroup);
    m_speakerColorButton->setMaximumWidth(50);
    m_speakerColorButton->setFlat(true);
    m_speakerColorButton->setAutoFillBackground(true);
    
    colorsLayout->addRow(tr("Timestamp color:"), m_timestampColorButton);
    colorsLayout->addRow(tr("Speaker color:"), m_speakerColorButton);
    
    // Window group
    QGroupBox* windowGroup = new QGroupBox(tr("Window"), appearanceTab);
    QFormLayout* windowLayout = new QFormLayout(windowGroup);
    
    m_windowOpacitySpin = new QSpinBox(windowGroup);
    m_windowOpacitySpin->setRange(50, 100);
    m_windowOpacitySpin->setSuffix("%");
    
    m_alwaysOnTopCheckBox = new QCheckBox(tr("Always on Top"), windowGroup); // Corrected member name
    
    windowLayout->addRow(tr("Window opacity:"), m_windowOpacitySpin);
    windowLayout->addRow(m_alwaysOnTopCheckBox); // Added the new checkbox to layout
    
    // Connect signals
    connect(changeFontButton, &QPushButton::clicked, this, &SettingsDialog::selectFont);
    connect(m_timestampColorButton, &QPushButton::clicked, this, [this]() {
        selectColor(m_timestampColorButton);
    });
    connect(m_speakerColorButton, &QPushButton::clicked, this, [this]() {
        selectColor(m_speakerColorButton);
    });
    
    layout->addWidget(themeGroup);
    layout->addWidget(fontGroup);
    layout->addWidget(colorsGroup);
    layout->addWidget(windowGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(appearanceTab, tr("Appearance"));
}

void SettingsDialog::createAdvancedTab()
{
    QWidget* advancedTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(advancedTab);
    
    // Performance group
    QGroupBox* performanceGroup = new QGroupBox(tr("Performance"), advancedTab);
    QFormLayout* performanceLayout = new QFormLayout(performanceGroup);
    
    m_threadCountSpin = new QSpinBox(performanceGroup);
    m_threadCountSpin->setRange(1, 16);
    m_threadCountSpin->setSpecialValueText(tr("Auto"));
    
    m_gpuAccelerationCheck = new QCheckBox(tr("Enable GPU acceleration (if available)"), performanceGroup);
    
    performanceLayout->addRow(tr("Processing threads:"), m_threadCountSpin);
    performanceLayout->addRow(m_gpuAccelerationCheck);
    
    // Logging group
    QGroupBox* loggingGroup = new QGroupBox(tr("Logging"), advancedTab);
    QFormLayout* loggingLayout = new QFormLayout(loggingGroup);
    
    m_enableLoggingCheck = new QCheckBox(tr("Enable logging"), loggingGroup);
    
    m_logLevelCombo = new QComboBox(loggingGroup);
    m_logLevelCombo->addItem(tr("Error"), static_cast<int>(Logger::LogLevel::Error));
    m_logLevelCombo->addItem(tr("Warning"), static_cast<int>(Logger::LogLevel::Warning));
    m_logLevelCombo->addItem(tr("Info"), static_cast<int>(Logger::LogLevel::Info));
    m_logLevelCombo->addItem(tr("Debug"), static_cast<int>(Logger::LogLevel::Debug));
    
    QPushButton* openLogButton = new QPushButton(tr("Open Log File"), loggingGroup);
    openLogButton->setMaximumWidth(150);
    
    loggingLayout->addRow(m_enableLoggingCheck);
    loggingLayout->addRow(tr("Log level:"), m_logLevelCombo);
    loggingLayout->addRow(openLogButton);
    
    // Data group
    QGroupBox* dataGroup = new QGroupBox(tr("Data"), advancedTab);
    QVBoxLayout* dataLayout = new QVBoxLayout(dataGroup);
    
    QPushButton* exportSettingsButton = new QPushButton(tr("Export Settings..."), dataGroup);
    QPushButton* importSettingsButton = new QPushButton(tr("Import Settings..."), dataGroup);
    QPushButton* clearCacheButton = new QPushButton(tr("Clear Cache"), dataGroup);
    
    QHBoxLayout* dataButtonLayout = new QHBoxLayout();
    dataButtonLayout->addWidget(exportSettingsButton);
    dataButtonLayout->addWidget(importSettingsButton);
    dataButtonLayout->addWidget(clearCacheButton);
    dataButtonLayout->addStretch();
    
    dataLayout->addLayout(dataButtonLayout);
    
    // Connect signals
    connect(m_enableLoggingCheck, &QCheckBox::toggled, m_logLevelCombo, &QComboBox::setEnabled);
    connect(openLogButton, &QPushButton::clicked, this, &SettingsDialog::openLogFile);
    connect(exportSettingsButton, &QPushButton::clicked, this, &SettingsDialog::exportSettings);
    connect(importSettingsButton, &QPushButton::clicked, this, &SettingsDialog::importSettings);
    connect(clearCacheButton, &QPushButton::clicked, this, &SettingsDialog::clearCache);
    
    layout->addWidget(performanceGroup);
    layout->addWidget(loggingGroup);
    layout->addWidget(dataGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(advancedTab, tr("Advanced"));
}

void SettingsDialog::connectSignals()
{
    // Enable apply button when settings change
    auto enableApply = [this]() { 
        m_applyButton->setEnabled(true); 
        m_hasChanges = true;
    };
    
    // General tab
    connect(m_startMinimizedCheck, &QCheckBox::toggled, enableApply);
    connect(m_startWithWindowsCheck, &QCheckBox::toggled, enableApply);
    connect(m_checkUpdatesCheck, &QCheckBox::toggled, enableApply);
    connect(m_showTrayIconCheck, &QCheckBox::toggled, enableApply);
    connect(m_minimizeToTrayCheck, &QCheckBox::toggled, enableApply);
    connect(m_startInTrayCheck, &QCheckBox::toggled, enableApply);
    connect(m_showNotificationsCheck, &QCheckBox::toggled, enableApply);
    connect(m_uiLanguageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), enableApply);
    
    // Audio tab
    connect(m_audioDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), enableApply);
    connect(m_sampleRateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), enableApply);
defaultModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), enableApply);
    connect(m_languageOverrideCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), enableApply);
    connect(m_translateCheck, &QCheckBox::toggled, enableApply);
    connect(m_computeTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), enableApply);
    
    // Hotkey tab
    connect(m_recordHotkeyEdit, &HotkeyEditWidget::hotkeyChanged, enableApply);
    connect(m_stopHotkeyEdit, &HotkeyEditWidget::hotkeyChanged, enableApply);
    connect(m_pauseHotkeyEdit, &HotkeyEditWidget::hotkeyChanged, enableApply);
    connect(m_cancelHotkeyEdit, &HotkeyEditWidget::hotkeyChanged, enableApply);
    connect(m_toggleWindowHotkeyEdit, &HotkeyEditWidget::hotkeyChanged, enableApply);
    connect(m_globalHotkeysCheck, &QCheckBox::toggled, enableApply);
    
    // Output tab
    connect(m_autoCopyCheck, &QCheckBox::toggled, enableApply);
    connect(m_autoTypeCheck, &QCheckBox::toggled, enableApply);
    connect(m_autoSaveCheck, &QCheckBox::toggled, enableApply);
    connect(m_autoSavePathEdit, &QLineEdit::textChanged, enableApply);
    connect(m_includeTimestampsCheck, &QCheckBox::toggled, enableApply);
    connect(m_timestampFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), enableApply);
    connect(m_wordWrapCheck, &QCheckBox::toggled, enableApply);
    connect(m_maxLineLengthSpin, QOverload<int>::of(&QSpinBox::valueChanged), enableApply);
    
    // Appearance tab
    connect(m_themeButtonGroup, QOverload<int>::of(&QButtonGroup::idClicked), enableApply);
    connect(m_windowOpacitySpin, QOverload<int>::of(&QSpinBox::valueChanged), enableApply);
    if (m_alwaysOnTopCheckBox) { // Check if initialized
        connect(m_alwaysOnTopCheckBox, &QCheckBox::toggled, enableApply);
    }
    
    // Advanced tab
    connect(m_threadCountSpin, QOverload<int>::of(&QSpinBox::valueChanged), enableApply);
    connect(m_gpuAccelerationCheck, &QCheckBox::toggled, enableApply);
    connect(m_enableLoggingCheck, &QCheckBox::toggled, enableApply);
    connect(m_logLevelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), enableApply);
}

void SettingsDialog::loadSettings()
{
    Settings& settings = Settings::instance();
    
    // General tab
    m_startMinimizedCheck->setChecked(settings.getSetting(Settings::Key::StartMinimized).toBool());
    m_startWithWindowsCheck->setChecked(settings.getSetting(Settings::Key::StartWithWindows).toBool());
    m_checkUpdatesCheck->setChecked(settings.getSetting(Settings::Key::CheckForUpdates).toBool());
    m_showTrayIconCheck->setChecked(settings.getSetting(Settings::Key::ShowTrayIcon).toBool());
    m_minimizeToTrayCheck->setChecked(settings.getSetting(Settings::Key::MinimizeToTray).toBool());
    m_startInTrayCheck->setChecked(settings.getSetting(Settings::Key::StartInTray).toBool());
    m_showNotificationsCheck->setChecked(settings.getSetting(Settings::Key::ShowTrayNotifications).toBool());
    
    QString uiLanguage = settings.getSetting(Settings::Key::UILanguage).toString();
    int langIndex = m_uiLanguageCombo->findData(uiLanguage);
    if (langIndex >= 0) {
        m_uiLanguageCombo->setCurrentIndex(langIndex);
    }
    
    // Audio tab
    refreshAudioDevices();
    QString deviceName = settings.getSetting(Settings::Key::InputDevice).toString();
    int deviceIndex = m_audioDeviceCombo->findText(deviceName);
    if (deviceIndex >= 0) {
        m_audioDeviceCombo->setCurrentIndex(deviceIndex);
    }
    
    int sampleRate = settings.getSetting(Settings::Key::SampleRate).toInt();
    int sampleRateIndex = m_sampleRateCombo->findData(sampleRate);
    if (sampleRateIndex >= 0) {
        m_sampleRateCombo->setCurrentIndex(sampleRateIndex);
    }
    
    m_vadEnabledCheck->setChecked(settings.getSetting(Settings::Key::VADEnabled).toBool());
    m_vadThresholdSlider->setValue(settings.getSetting(Settings::Key::VADThreshold).toInt());
    m_noiseSuppressCheck->setChecked(settings.getSetting(Settings::Key::NoiseSuppressionEnabled).toBool());
    
    // Model tab
    refreshModels();
    QString modelName = settings.getSetting(Settings::Key::Model).toString();
    int modelIndex = m_defaultModelCombo->findData(modelName);
    if (modelIndex >= 0) {
        m_defaultModelCombo->setCurrentIndex(modelIndex);
    }
    
    m_modelsPathEdit->setText(settings.getSetting(Settings::Key::ModelsPath).toString());
    
    QString language = settings.getSetting(Settings::Key::Language).toString();
    int langOverrideIndex = m_languageOverrideCombo->findData(language);
    if (langOverrideIndex >= 0) {
        m_languageOverrideCombo->setCurrentIndex(langOverrideIndex);
    }
    
    m_translateCheck->setChecked(settings.getSetting(Settings::Key::TranslateToEnglish).toBool());
    
    QString computeType = settings.getSetting(Settings::Key::ComputeType).toString();
    int computeIndex = m_computeTypeCombo->findData(computeType);
    if (computeIndex >= 0) {
        m_computeTypeCombo->setCurrentIndex(computeIndex);
    }
    
    // Hotkey tab
    m_recordHotkeyEdit->setHotkey(settings.getSetting(Settings::Key::RecordHotkey).toString());
    m_stopHotkeyEdit->setHotkey(settings.getSetting(Settings::Key::StopHotkey).toString());
    m_pauseHotkeyEdit->setHotkey(settings.getSetting(Settings::Key::PauseHotkey).toString());
    m_cancelHotkeyEdit->setHotkey(settings.getSetting(Settings::Key::CancelHotkey).toString());
    m_toggleWindowHotkeyEdit->setHotkey(settings.getSetting(Settings::Key::ToggleWindowHotkey).toString());
    m_globalHotkeysCheck->setChecked(settings.getSetting(Settings::Key::GlobalHotkeysEnabled).toBool());
    
    // Output tab
    m_autoCopyCheck->setChecked(settings.getSetting(Settings::Key::CopyToClipboard).toBool());
    m_autoTypeCheck->setChecked(settings.getSetting(Settings::Key::TypeInActiveWindow).toBool());
    m_autoSaveCheck->setChecked(settings.getSetting(Settings::Key::AutoSaveTranscriptions).toBool());
    m_autoSavePathEdit->setText(settings.getSetting(Settings::Key::AutoSavePath).toString());
    m_includeTimestampsCheck->setChecked(settings.getSetting(Settings::Key::IncludeTimestamps).toBool());
    
    QString timestampFormat = settings.getSetting(Settings::Key::TimestampFormat).toString();
    int formatIndex = m_timestampFormatCombo->findData(timestampFormat);
    if (formatIndex >= 0) {
        m_timestampFormatCombo->setCurrentIndex(formatIndex);
    }
    
    m_wordWrapCheck->setChecked(settings.getSetting(Settings::Key::WordWrap).toBool());
    m_maxLineLengthSpin->setValue(settings.getSetting(Settings::Key::MaxLineLength).toInt());
    
    // Appearance tab
    int theme = settings.getSetting(Settings::Key::Theme).toInt();
    m_themeButtonGroup->button(theme)->setChecked(true);
    
    QFont font;
    font.fromString(settings.getSetting(Settings::Key::TranscriptionFont).toString());
    m_fontLabel->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
    m_fontLabel->setFont(font);
    
    QColor timestampColor(settings.getSetting(Settings::Key::TimestampColor).toString());
    QPalette timestampPalette = m_timestampColorButton->palette();
    timestampPalette.setColor(QPalette::Button, timestampColor);
    m_timestampColorButton->setPalette(timestampPalette);
    
    QColor speakerColor(settings.getSetting(Settings::Key::SpeakerColor).toString());
    QPalette speakerPalette = m_speakerColorButton->palette();
    speakerPalette.setColor(QPalette::Button, speakerColor);
    m_speakerColorButton->setPalette(speakerPalette);
    
    m_windowOpacitySpin->setValue(settings.getSetting(Settings::Key::WindowOpacity).toInt());
    if (m_alwaysOnTopCheckBox) { // Check if initialized
        m_alwaysOnTopCheckBox->setChecked(settings.isAlwaysOnTop());
    }
    
    // Advanced tab
    m_threadCountSpin->setValue(settings.getSetting(Settings::Key::ThreadCount).toInt());
    m_gpuAccelerationCheck->setChecked(settings.getSetting(Settings::Key::GPUAcceleration).toBool());
    m_enableLoggingCheck->setChecked(settings.getSetting(Settings::Key::EnableLogging).toBool());
    m_logLevelCombo->setCurrentIndex(m_logLevelCombo->findData(settings.getSetting(Settings::Key::LogLevel).toInt()));
    
    m_hasChanges = false;
    m_applyButton->setEnabled(false);
}

void SettingsDialog::saveSettings()
{
    Settings& settings = Settings::instance();
    
    // General tab
    settings.setSetting(Settings::Key::StartMinimized, m_startMinimizedCheck->isChecked());
    settings.setSetting(Settings::Key::StartWithWindows, m_startWithWindowsCheck->isChecked());
    settings.setSetting(Settings::Key::CheckForUpdates, m_checkUpdatesCheck->isChecked());
    settings.setSetting(Settings::Key::ShowTrayIcon, m_showTrayIconCheck->isChecked());
    settings.setSetting(Settings::Key::MinimizeToTray, m_minimizeToTrayCheck->isChecked());
    settings.setSetting(Settings::Key::StartInTray, m_startInTrayCheck->isChecked());
    settings.setSetting(Settings::Key::ShowTrayNotifications, m_showNotificationsCheck->isChecked());
    settings.setSetting(Settings::Key::UILanguage, m_uiLanguageCombo->currentData().toString());
    
    // Audio tab
    settings.setSetting(Settings::Key::InputDevice, m_audioDeviceCombo->currentText());
    settings.setSetting(Settings::Key::SampleRate, m_sampleRateCombo->currentData().toInt());
    settings.setSetting(Settings::Key::VADEnabled, m_vadEnabledCheck->isChecked());
    settings.setSetting(Settings::Key::VADThreshold, m_vadThresholdSlider->value());
    settings.setSetting(Settings::Key::NoiseSuppressionEnabled, m_noiseSuppressCheck->isChecked());
    
    // Model tab
    settings.setSetting(Settings::Key::Model, m_defaultModelCombo->currentData().toString());
    settings.setSetting(Settings::Key::Language, m_languageOverrideCombo->currentData().toString());
    settings.setSetting(Settings::Key::TranslateToEnglish, m_translateCheck->isChecked());
    settings.setSetting(Settings::Key::ComputeType, m_computeTypeCombo->currentData().toString());
    
    // Hotkey tab
    settings.setSetting(Settings::Key::RecordHotkey, m_recordHotkeyEdit->getHotkey());
    settings.setSetting(Settings::Key::StopHotkey, m_stopHotkeyEdit->getHotkey());
    settings.setSetting(Settings::Key::PauseHotkey, m_pauseHotkeyEdit->getHotkey());
    settings.setSetting(Settings::Key::CancelHotkey, m_cancelHotkeyEdit->getHotkey());
    settings.setSetting(Settings::Key::ToggleWindowHotkey, m_toggleWindowHotkeyEdit->getHotkey());
    settings.setSetting(Settings::Key::GlobalHotkeysEnabled, m_globalHotkeysCheck->isChecked());
    
    // Output tab
    settings.setSetting(Settings::Key::CopyToClipboard, m_autoCopyCheck->isChecked());
    settings.setSetting(Settings::Key::TypeInActiveWindow, m_autoTypeCheck->isChecked());
    settings.setSetting(Settings::Key::AutoSaveTranscriptions, m_autoSaveCheck->isChecked());
    settings.setSetting(Settings::Key::AutoSavePath, m_autoSavePathEdit->text());
    settings.setSetting(Settings::Key::IncludeTimestamps, m_includeTimestampsCheck->isChecked());
    settings.setSetting(Settings::Key::TimestampFormat, m_timestampFormatCombo->currentData().toString());
    settings.setSetting(Settings::Key::WordWrap, m_wordWrapCheck->isChecked());
    settings.setSetting(Settings::Key::MaxLineLength, m_maxLineLengthSpin->value());
    
    // Appearance tab
    settings.setSetting(Settings::Key::Theme, m_themeButtonGroup->checkedId());
    
    QFont font = m_fontLabel->font();
    settings.setSetting(Settings::Key::TranscriptionFont, font.toString());
    
    QPalette timestampPalette = m_timestampColorButton->palette();
    settings.setSetting(Settings::Key::TimestampColor, timestampPalette.color(QPalette::Button).name());
    
    QPalette speakerPalette = m_speakerColorButton->palette();
    settings.setSetting(Settings::Key::SpeakerColor, speakerPalette.color(QPalette::Button).name());
    
    settings.setSetting(Settings::Key::WindowOpacity, m_windowOpacitySpin->value());
    if (m_alwaysOnTopCheckBox) { // Check if initialized
        settings.setAlwaysOnTop(m_alwaysOnTopCheckBox->isChecked());
    }
    
    // Advanced tab
    settings.setSetting(Settings::Key::ThreadCount, m_threadCountSpin->value());
    settings.setSetting(Settings::Key::GPUAcceleration, m_gpuAccelerationCheck->isChecked());
    settings.setSetting(Settings::Key::EnableLogging, m_enableLoggingCheck->isChecked());
    settings.setSetting(Settings::Key::LogLevel, m_logLevelCombo->currentData().toInt());
    
    // Save to storage
    settings.saveSettings();
    
    m_hasChanges = false;
    m_applyButton->setEnabled(false);
    
    Logger::instance().log(Logger::LogLevel::Info, "SettingsDialog", "Settings saved");
}

void SettingsDialog::applySettings()
{
    saveSettings();
    emit settingsApplied();
}

void SettingsDialog::accept()
{
    if (m_hasChanges) {
        saveSettings();
    }
    QDialog::accept();
}

void SettingsDialog::reject()
{
    if (m_hasChanges) {
        int ret = QMessageBox::question(this, tr("Unsaved Changes"),
                                       tr("You have unsaved changes. Do you want to save them?"),
                                       QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (ret == QMessageBox::Save) {
            saveSettings();
            QDialog::accept();
        } else if (ret == QMessageBox::Discard) {
            QDialog::reject();
        }
        // Cancel - do nothing
    } else {
        QDialog::reject();
    }
}

void SettingsDialog::resetToDefaults()
{
    int ret = QMessageBox::question(this, tr("Reset Settings"),
                                   tr("Are you sure you want to reset all settings to their default values?"),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        Settings::instance().resetToDefaults();
        loadSettings();
        Logger::instance().log(Logger::LogLevel::Info, "SettingsDialog", "Settings reset to defaults");
    }
}

// Audio tab slots
void SettingsDialog::refreshAudioDevices()
{
    m_audioDeviceCombo->clear();
    
    DeviceManager deviceManager;
    auto devices = deviceManager.getInputDevices();
    
    m_audioDeviceCombo->addItem(tr("Default"), "");
    
    for (const auto& device : devices) {
        m_audioDeviceCombo->addItem(QString::fromStdString(device.getName()), 
                                   QString::fromStdString(device.getId()));
    }
}

void SettingsDialog::testAudioDevice()
{
    // TODO: Implement audio device testing
    QMessageBox::information(this, tr("Test Audio"), tr("Audio device testing coming soon!"));
}

void SettingsDialog::onAudioDeviceChanged(int index)
{
    // Update device info if needed
}

// Model tab slots
void SettingsDialog::refreshModels()
{
    m_defaultModelCombo->clear();
    
    ModelManager modelManager;
    auto models = modelManager.getAvailableModels();
    
    for (const auto& model : models) {
        m_defaultModelCombo->addItem(QString::fromStdString(model.getName()),
                                    QString::fromStdString(model.getId()));
    }
}

void SettingsDialog::onModelChanged(int index)
{
    if (index < 0) return;
    
    QString modelId = m_defaultModelCombo->itemData(index).toString();
    
    // Update model info
    ModelManager modelManager;
    auto models = modelManager.getAvailableModels();
    
    for (const auto& model : models) {
        if (QString::fromStdString(model.getId()) == modelId) {
            QString info = tr("Size: %1 MB\nType: %2\nLanguages: %3")
                .arg(model.getSize() / (1024 * 1024))
                .arg(QString::fromStdString(model.getDescription()))
                .arg(QString::fromStdString(model.getLanguages()));
            m_modelInfoLabel->setText(info);
            break;
        }
    }
}

void SettingsDialog::browseModelsPath()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Models Directory"),
                                                   m_modelsPathEdit->text());
    if (!dir.isEmpty()) {
        m_modelsPathEdit->setText(dir);
        Settings::instance().setSetting(Settings::Key::ModelsPath, dir);
        refreshModels();
    }
}

void SettingsDialog::openModelManager()
{
    emit modelManagerRequested();
}

// Hotkey tab slots
void SettingsDialog::resetHotkeys()
{
    m_recordHotkeyEdit->setHotkey("Ctrl+Shift+R");
    m_stopHotkeyEdit->setHotkey("Ctrl+Shift+S");
    m_pauseHotkeyEdit->setHotkey("Ctrl+Shift+P");
    m_cancelHotkeyEdit->setHotkey("Escape");
    m_toggleWindowHotkeyEdit->setHotkey("Ctrl+Shift+W");
}

// Output tab slots
void SettingsDialog::browseAutoSavePath()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Auto-Save Directory"),
                                                   m_autoSavePathEdit->text());
    if (!dir.isEmpty()) {
        m_autoSavePathEdit->setText(dir);
    }
}

// Appearance tab slots
void SettingsDialog::selectFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_fontLabel->font(), this);
    if (ok) {
        m_fontLabel->setFont(font);
        m_fontLabel->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
    }
}

void SettingsDialog::selectColor(QPushButton* button)
{
    QColor color = QColorDialog::getColor(button->palette().color(QPalette::Button), this);
    if (color.isValid()) {
        QPalette palette = button->palette();
        palette.setColor(QPalette::Button, color);
        button->setPalette(palette);
    }
}

// Advanced tab slots
void SettingsDialog::openLogFile()
{
    QString logPath = Logger::instance().getLogFilePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
}

void SettingsDialog::exportSettings()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Settings"),
                                                   "whisperapp_settings.json",
                                                   tr("JSON Files (*.json)"));
    if (!fileName.isEmpty()) {
        if (Settings::instance().exportSettings(fileName)) {
            QMessageBox::information(this, tr("Export Successful"),
                                   tr("Settings exported successfully."));
        } else {
            QMessageBox::critical(this, tr("Export Failed"),
                                tr("Failed to export settings."));
        }
    }
}

void SettingsDialog::importSettings()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Settings"),
                                                   "",
                                                   tr("JSON Files (*.json)"));
    if (!fileName.isEmpty()) {
        if (Settings::instance().importSettings(fileName)) {
            loadSettings();
            QMessageBox::information(this, tr("Import Successful"),
                                   tr("Settings imported successfully."));
        } else {
            QMessageBox::critical(this, tr("Import Failed"),
                                tr("Failed to import settings."));
        }
    }
}

void SettingsDialog::clearCache()
{
    int ret = QMessageBox::question(this, tr("Clear Cache"),
                                   tr("Are you sure you want to clear the application cache?"),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        // TODO: Implement cache clearing
        QMessageBox::information(this, tr("Cache Cleared"),
                               tr("Application cache has been cleared."));
    }
}
    connect(m_vadEnabledCheck, &QCheckBox::toggled, enableApply);
    connect(m_vadThresholdSlider, &QSlider::valueChanged, enableApply);
    connect(m_noiseSuppressCheck, &QCheckBox::toggled, enableApply);
    
    // Model tab
    connect(m_defaultModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), enableApply);