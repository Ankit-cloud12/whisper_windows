#include <QtTest/QtTest>
#include <QApplication> // Required for QTest::qExec with GUI elements

// Example includes for future test files (currently commented out)
#include "test_settings.h"
#include "test_mainwindow_logic.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv); // QApplication needed for QSettings and GUI tests

    int status = 0;

    TestSettings testSettingsObj;
    status |= QTest::qExec(&testSettingsObj, argc, argv);

    TestMainWindowLogic testMainWindowLogicObj;
    status |= QTest::qExec(&testMainWindowLogicObj, argc, argv);

    if (status != 0) {
        qWarning("Some auto tests failed!");
    } else {
        qInfo("All auto tests passed!");
    }

    return status;
}
