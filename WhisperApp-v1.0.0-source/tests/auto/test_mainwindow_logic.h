#ifndef TEST_MAINWINDOW_LOGIC_H
#define TEST_MAINWINDOW_LOGIC_H

#include <QtTest/QtTest>
#include <QObject>

// Forward declarations
class MainWindow;
class Settings;

class TestMainWindowLogic : public QObject
{
    Q_OBJECT

public:
    TestMainWindowLogic();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testUpdateAutostartRegistration_Enable();
    void testUpdateAutostartRegistration_Disable();
    void testUpdateAutostartRegistration_Toggle();

    void testApplyAlwaysOnTopSetting_Enable();
    void testApplyAlwaysOnTopSetting_Disable();

    // UI state tests
    void testStatusLabel_ErrorState();
    void testStatusLabel_ClearState();
    void testProcessingSpinner_Visibility();
    void testRecordButton_DisabledState_NoModel();
    void testRecordButton_EnabledState_ModelAvailable();

private:
    MainWindow* m_mainWindow = nullptr;
    // Settings* m_testSettings = nullptr; // Settings are global via Settings::instance()
    QString m_appNameForRegistry = "WhisperAppTestEntry"; // Use a test-specific registry entry name
    QString m_appPathForRegistry;

    void verifyRegistryEntry(bool shouldExist);
};

#endif // TEST_MAINWINDOW_LOGIC_H
