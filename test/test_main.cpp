#include <QApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <catch2/catch_all.hpp>

int main(int argc, char* argv[]) {
  QApplication app{argc, argv};
  QCoreApplication::setOrganizationName("test_holzner");
  QCoreApplication::setApplicationName("test-borgmatic-ui");

  // Keep the tests' settings out of the user's configuration.
  QTemporaryDir settingsDir;
  QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settingsDir.path());
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());

  return Catch::Session().run(argc, argv);
}
