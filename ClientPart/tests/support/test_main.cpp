#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QSettings>
#include <QTemporaryDir>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    QTemporaryDir settingsDir;
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settingsDir.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());

    if (qEnvironmentVariableIntValue("MESSENGER_TEST_LOGS") != 1)
        QLoggingCategory::setFilterRules("app.*=false");

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
