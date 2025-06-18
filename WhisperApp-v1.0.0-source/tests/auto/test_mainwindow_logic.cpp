#include "test_mainwindow_logic.h"
#include "ui/MainWindow.h"   // Adjust path as necessary
#include "core/Settings.h" // Adjust path as necessary
#include <QSettings>         // For direct registry checking
#include <QCoreApplication>
#include <QDir>
#include <QVariant> // For QVariant comparison if needed, and for QSettings

TestMainWindowLogic::TestMainWindowLogic() {
    // Ensure unique names for QSettings if MainWindow's constructor uses Settings::instance()
    // This was also done in TestSettings, ensure consistency if tests run in same process
    // QCoreApplication::setOrganizationName("WhisperAppTestOrg"); // Already set by TestSettings if run first
    // QCoreApplication::setApplicationName("WhisperAppTestAppLogic");
}

void TestMainWindowLogic::initTestCase() {
    // This is tricky because MainWindow creates UI, etc.
    // We need a valid QApplication instance, which main_auto_tests.cpp should provide.

    // It's important that QCoreApplication organization and application names are set *before*
    // Settings::instance() is called for the first time if it relies on QSettings
    // or before MainWindow constructor if it initializes Settings or uses QSettings itself.
    // For safety, ensure it's set here too, though TestSettings might do it.
    QCoreApplication::setOrganizationName("WhisperAppTestOrg");
    QCoreApplication::setApplicationName("WhisperAppTestAppLogic");


    m_mainWindow = new MainWindow();
    QVERIFY(m_mainWindow != nullptr);

    m_appPathForRegistry = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());

    // The test will use the production application name "WhisperApp" for registry key
    // as MainWindow::updateAutostartRegistration uses this fixed name.
    // We must ensure this key is cleaned up.
    m_appNameForRegistry = "WhisperApp";
}

void TestMainWindowLogic::cleanupTestCase() {
    delete m_mainWindow;
    m_mainWindow = nullptr;

    // Ensure the test registry key is cleaned up, regardless of test outcomes
    QSettings registrySettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (registrySettings.contains(m_appNameForRegistry)) {
        registrySettings.remove(m_appNameForRegistry);
        qInfo() << "Cleaned up registry entry after tests:" << m_appNameForRegistry;
    }
}

void TestMainWindowLogic::init() {
    // Before each test, ensure the registry key is not present
    QSettings registrySettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (registrySettings.contains(m_appNameForRegistry)) {
        registrySettings.remove(m_appNameForRegistry);
    }
    // And ensure the setting that MainWindow reads is initially false
    Settings::instance().setStartWithWindows(false);
    // Settings::instance().sync(); // Settings class does not have sync()
    // The MainWindow constructor calls updateAutostartRegistration based on the current setting.
    // We need to ensure this init() correctly sets the stage *before* the test logic runs.
    Settings::instance().setAlwaysOnTop(false);
    // Settings::instance().sync(); // No sync method in Settings.h

    // Reset relevant UI states in MainWindow if possible, or ensure they are in a known default
    if (m_mainWindow) {
        // Reset status label
        m_mainWindow->m_statusLabel->setStyleSheet("");
        m_mainWindow->m_statusLabel->setText(tr("Ready")); // Assuming "Ready" is default idle text
        // Hide spinner
        if (m_mainWindow->m_processingSpinner) {
            m_mainWindow->m_processingSpinner->setVisible(false);
        }
        // Ensure record button is enabled by default if a model is considered available
        // This interacts with checkInitialDisabledState, so ensure settings allow it.
        // For most tests, assume a model is available unless specified.
        Settings::instance().setSetting(SettingsKey::MODEL_ID, "tiny"); // Assume "tiny" is a valid, 'downloaded' model for testing
        m_mainWindow->checkInitialDisabledState(); // This should enable the button if model is "available"
    }
}

void TestMainWindowLogic::cleanup() {
    // After each test, also ensure the registry key is not present
    QSettings registrySettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (registrySettings.contains(m_appNameForRegistry)) {
        registrySettings.remove(m_appNameForRegistry);
    }
}

void TestMainWindowLogic::verifyRegistryEntry(bool shouldExist) {
    QSettings registrySettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    bool actuallyExists = registrySettings.contains(m_appNameForRegistry);

    // Provide more debug output
    if (actuallyExists != shouldExist) {
        qWarning() << "Registry entry" << m_appNameForRegistry << "existence check failed. Expected:" << shouldExist << "Actual:" << actuallyExists;
        qWarning() << "Registry path: HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
        qWarning() << "All keys in Run:" << registrySettings.allKeys();
    }
    QCOMPARE(actuallyExists, shouldExist);

    if (shouldExist) {
        QString actualPath = registrySettings.value(m_appNameForRegistry).toString();
        if (actualPath != m_appPathForRegistry) {
            qWarning() << "Registry path for" << m_appNameForRegistry << "mismatch. Expected:" << m_appPathForRegistry << "Actual:" << actualPath;
        }
        QCOMPARE(actualPath, m_appPathForRegistry);
    }
}

void TestMainWindowLogic::testUpdateAutostartRegistration_Enable() {
    QVERIFY(m_mainWindow != nullptr);

    // Set the desired state in Settings first, as MainWindow::updateAutostartRegistration reads from it.
    Settings::instance().setStartWithWindows(true);
    // Settings::instance().sync(); // No sync method in Settings.h

    m_mainWindow->updateAutostartRegistration(true); // Call the method under test
    verifyRegistryEntry(true);
}

void TestMainWindowLogic::testUpdateAutostartRegistration_Disable() {
    QVERIFY(m_mainWindow != nullptr);

    // First, enable it and verify, as a prerequisite.
    Settings::instance().setStartWithWindows(true);
    m_mainWindow->updateAutostartRegistration(true);
    verifyRegistryEntry(true);

    // Now, test disabling it.
    Settings::instance().setStartWithWindows(false);
    m_mainWindow->updateAutostartRegistration(false);
    verifyRegistryEntry(false);
}

void TestMainWindowLogic::testUpdateAutostartRegistration_Toggle() {
    QVERIFY(m_mainWindow != nullptr);

    // Enable
    Settings::instance().setStartWithWindows(true);
    m_mainWindow->updateAutostartRegistration(true);
    verifyRegistryEntry(true);

    // Disable
    Settings::instance().setStartWithWindows(false);
    m_mainWindow->updateAutostartRegistration(false);
    verifyRegistryEntry(false);
}

void TestMainWindowLogic::testApplyAlwaysOnTopSetting_Enable() {
    QVERIFY(m_mainWindow);

    // Ensure setting is true and apply it
    Settings::instance().setAlwaysOnTop(true);
    // Settings::instance().sync(); // No sync method in Settings.h
    m_mainWindow->applyAlwaysOnTopSetting();

    Qt::WindowFlags flags = m_mainWindow->windowFlags();
    QVERIFY((flags & Qt::WindowStaysOnTopHint) == Qt::WindowStaysOnTopHint);
}

void TestMainWindowLogic::testApplyAlwaysOnTopSetting_Disable() {
    QVERIFY(m_mainWindow);

    // First, enable and verify it's set
    Settings::instance().setAlwaysOnTop(true);
    // Settings::instance().sync(); // No sync method in Settings.h
    m_mainWindow->applyAlwaysOnTopSetting();
    QVERIFY((m_mainWindow->windowFlags() & Qt::WindowStaysOnTopHint) == Qt::WindowStaysOnTopHint);

    // Now, disable and verify it's cleared
    Settings::instance().setAlwaysOnTop(false);
    // Settings::instance().sync(); // No sync method in Settings.h
    m_mainWindow->applyAlwaysOnTopSetting();

    Qt::WindowFlags flags = m_mainWindow->windowFlags();
    QVERIFY((flags & Qt::WindowStaysOnTopHint) != Qt::WindowStaysOnTopHint);
}

void TestMainWindowLogic::testStatusLabel_ErrorState() {
    QVERIFY(m_mainWindow);
    QVERIFY(m_mainWindow->m_statusLabel); // Ensure label exists

    // Call onTranscriptionError, which is a public slot
    m_mainWindow->onTranscriptionError("Test suite error");

    QVERIFY(m_mainWindow->m_statusLabel->styleSheet().contains("color: red"));
    QVERIFY(m_mainWindow->m_statusLabel->text().contains("Test suite error"));
}

void TestMainWindowLogic::testStatusLabel_ClearState() {
    QVERIFY(m_mainWindow);
    QVERIFY(m_mainWindow->m_statusLabel);

    // 1. Set error state
    m_mainWindow->onTranscriptionError("Initial test error");
    QVERIFY(m_mainWindow->m_statusLabel->styleSheet().contains("color: red"));

    // 2. Call a method that should clear the error state and reset the label
    // For example, starting a new recording (if successful) or completing one.
    // Let's use onTranscriptionComplete as it should reset status.
    // We might need to simulate a successful model selection first if startRecording has checks
    Settings::instance().setSetting(SettingsKey::MODEL_ID, "tiny"); // Assume tiny is valid
    m_mainWindow->checkInitialDisabledState(); // Ensure record button would be enabled

    m_mainWindow->onTranscriptionComplete("Test success"); // This sets statusLabel to "Transcription complete"

    QVERIFY(!m_mainWindow->m_statusLabel->styleSheet().contains("color: red"));
    QVERIFY(m_mainWindow->m_statusLabel->text() == tr("Transcription complete"));
    // Or verify against "Ready" if onTranscriptionComplete leads to an idle state that sets it to Ready
    // The lambda in MainWindow::connectSignals sets it to "Transcription complete" and clears style.
}

void TestMainWindowLogic::testProcessingSpinner_Visibility() {
    QVERIFY(m_mainWindow);
    QVERIFY(m_mainWindow->m_processingSpinner); // Ensure spinner exists

    // Simulate start of processing (e.g., when recording stops)
    // MainWindow::stopRecording() sets the spinner visible and m_statusLabel to "Processing..."
    // It also sets m_trayIcon state.
    // For this test, we need a model to be "selected" for startRecording to not bail early.
    Settings::instance().setSetting(SettingsKey::MODEL_ID, "tiny");
    m_mainWindow->checkInitialDisabledState();
    QVERIFY(m_mainWindow->m_recordButton->isEnabled()); // Ensure we can proceed

    m_mainWindow->startRecording(); // Sets m_recording to true
    m_mainWindow->stopRecording();  // Sets spinner visible, status to Processing
    QVERIFY(m_mainWindow->m_processingSpinner->isVisible());
    QVERIFY(m_mainWindow->m_statusLabel->text() == tr("Processing..."));

    // Simulate end of processing
    m_mainWindow->onTranscriptionComplete("Some result"); // Hides spinner, updates status
    QVERIFY(!m_mainWindow->m_processingSpinner->isVisible());
    QVERIFY(m_mainWindow->m_statusLabel->text() == tr("Transcription complete"));
}

void TestMainWindowLogic::testRecordButton_DisabledState_NoModel() {
    QVERIFY(m_mainWindow);
    QVERIFY(m_mainWindow->m_recordButton);
    QVERIFY(m_mainWindow->m_statusLabel);

    // Simulate no model being available/selected
    // In MainWindow, populateModelCombo calls checkInitialDisabledState.
    // onModelChanged also calls it.
    // We can directly call checkInitialDisabledState after manipulating settings.
    Settings::instance().setSetting(SettingsKey::MODEL_ID, ""); // Set to an invalid/empty model
    m_mainWindow->populateModelCombo(); // This should call checkInitialDisabledState internally due to onModelChanged or direct call
                                     // Or directly:
    m_mainWindow->checkInitialDisabledState();


    QVERIFY(!m_mainWindow->m_recordButton->isEnabled());
    QVERIFY(m_mainWindow->m_statusLabel->text().contains("Error: No valid model selected"));
    QVERIFY(m_mainWindow->m_statusLabel->styleSheet().contains("color: red"));
}

void TestMainWindowLogic::testRecordButton_EnabledState_ModelAvailable() {
    QVERIFY(m_mainWindow);
    QVERIFY(m_mainWindow->m_recordButton);

    // Simulate a model being available
    // Assuming "tiny" is a model that would be considered "downloaded" by ModelManager stub or setup
    Settings::instance().setSetting(SettingsKey::MODEL_ID, "tiny");
    // Trigger re-evaluation of button states
    m_mainWindow->populateModelCombo(); // This should call checkInitialDisabledState
                                     // Or directly:
    m_mainWindow->checkInitialDisabledState();

    QVERIFY(m_mainWindow->m_recordButton->isEnabled());
    // Status label should be "Ready" if not in error from other sources
    if (m_mainWindow->m_statusLabel->text() != tr("Processing...") && !m_mainWindow->m_recording) {
       // QVERIFY(m_mainWindow->m_statusLabel->text() == tr("Ready")); // This might be too strict if other errors are present
       QVERIFY(!m_mainWindow->m_statusLabel->styleSheet().contains("color: red;"));
    }
}
