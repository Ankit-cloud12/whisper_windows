#ifndef TEST_SETTINGS_H
#define TEST_SETTINGS_H

#include <QtTest/QtTest>
#include <QObject>

// Forward declare Settings to avoid including the full header if it's complex
// However, for direct member access or construction, full include is needed.
// Assuming Settings.h is lightweight or direct interaction is required.
#include "core/Settings.h" // Adjust path as needed

class TestSettings : public QObject
{
    Q_OBJECT

public:
    TestSettings();
    ~TestSettings();

private slots:
    void initTestCase();    // Called before the first test function
    void cleanupTestCase(); // Called after the last test function
    void init();            // Called before each test function
    void cleanup();         // Called after each test function

    // Test methods
    void testDefaultSettings();
    void testAlwaysOnTopSetting();
    void testStartWithWindowsSetting();
    void testSettingsPersistence(); // Combined test for multiple settings

private:
    Settings* m_settings = nullptr;
    // Helper to ensure a clean settings state for tests if possible
    void clearTestSettings();
};

#endif // TEST_SETTINGS_H
