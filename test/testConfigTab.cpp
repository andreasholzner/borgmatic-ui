#include <QCheckBox>
#include <QCoreApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QTableView>
#include <QtTest>
#include <catch2/catch.hpp>
#include <catch2/trompeloeil.hpp>
#include <memory>
#include <string>
#include <vector>

#include "BackupConfig.h"
#include "ConfigTab.h"
#include "DesktopServicesWrapper.h"
#include "test_helper.h"

using namespace trompeloeil;

auto prepareInfo() {
  backup::helper::Info info{};
  info.location = "location";
  info.originalSize = 1024000;
  info.compressedSize = 0;
  return info;
}

auto prepareList() {
  return std::vector<backup::helper::ListItem>{{"id1", "name1", "2000-10-05 10:15:30.500", false},
                                               {"id2", "name2", "2000-10-06 10:15:30.500", true}};
}

TEST_CASE("ConfigTab construction", "[ui]") {
  auto config = std::make_shared<BackupConfigMock>();
  std::shared_ptr<DesktopServicesWrapper> wrapperMock{std::make_shared<DesktopServicesWrapperMock>()};
  std::string configFileName{"file1"};

  SECTION("initializes all displayed data from BackupConfig via info and list") {
    REQUIRE_CALL(*config, borgmaticConfigFile(eq(configFileName)));
    REQUIRE_CALL(*config, borgmaticConfigFile()).RETURN(configFileName);
    ALLOW_CALL(*config, isBackupPurging()).RETURN(false);
    ALLOW_CALL(*config, isMountPointToBeOpened()).RETURN(false);
    REQUIRE_CALL(*config, info()).RETURN(prepareInfo());
    REQUIRE_CALL(*config, list()).RETURN(prepareList());

    auto configTab = ConfigTab{config, wrapperMock};
    wait_for_qthreads_to_finish();
    QApplication::processEvents();

    REQUIRE(configTab.findChild<QCheckBox *>("purgeCheckBox")->isEnabled() == true);
    REQUIRE(configTab.findChild<QCheckBox *>("openMountPointCheckBox")->isEnabled() == true);
    REQUIRE(configTab.findChild<QPushButton *>("startBackupButton")->isEnabled() == true);
    REQUIRE(configTab.findChild<QPushButton *>("cancelBackupButton")->isEnabled() == false);

    // data from info
    REQUIRE(configTab.findChild<QLineEdit *>("configEdit")->text().toStdString() == configFileName);
    REQUIRE(configTab.findChild<QLabel *>("infoLocationLabel")->text().toStdString() == prepareInfo().location);
    REQUIRE(configTab.findChild<QLabel *>("infoOriginalSizeLabel")->text().toStdString() == "1.000,00 KiB");
    REQUIRE(configTab.findChild<QLabel *>("infoCompressedSizeLabel")->text().toStdString() == "-");

    // data from list
    auto model = configTab.findChild<QTableView *>("backupsTableView")->model();
    REQUIRE(model->data(model->index(0, 0)).value<QString>().toStdString() == prepareList()[0].name);
    REQUIRE(model->data(model->index(1, 0)).value<QString>().toStdString() == prepareList()[1].name);
  }
}

TEST_CASE("ConfigTab", "[ui]") {
  auto config = std::make_shared<BackupConfigMock>();
  auto wrapperMock = std::make_shared<DesktopServicesWrapperMock>();
  std::shared_ptr<ConfigTab> configTab = nullptr;
  {
    ALLOW_CALL(*config, borgmaticConfigFile()).RETURN(std::string("file"));
    ALLOW_CALL(*config, borgmaticConfigFile(_));
    ALLOW_CALL(*config, isBackupPurging()).RETURN(false);
    ALLOW_CALL(*config, isMountPointToBeOpened()).RETURN(false);
    ALLOW_CALL(*config, info()).RETURN(prepareInfo());
    ALLOW_CALL(*config, list()).RETURN(prepareList());
    configTab = std::make_shared<ConfigTab>(config, wrapperMock);
    wait_for_qthreads_to_finish();
    QApplication::processEvents();
  }

  SECTION("new borgmatic config fetches info and list data") {
    std::string newBorgmaticConfig = "new_config";
    REQUIRE_CALL(*config, borgmaticConfigFile(eq(newBorgmaticConfig)));
    REQUIRE_CALL(*config, info()).RETURN(prepareInfo());
    REQUIRE_CALL(*config, list()).RETURN(prepareList());
    ALLOW_CALL(*config, isBackupPurging()).RETURN(true);
    ALLOW_CALL(*config, isBackupPurging(eq(1)));
    ALLOW_CALL(*config, isMountPointToBeOpened()).RETURN(true);
    ALLOW_CALL(*config, isMountPointToBeOpened(eq(1)));

    configTab->findChild<QLineEdit *>("configEdit")->setText(newBorgmaticConfig.c_str());
    wait_for_qthreads_to_finish();
  }

  SECTION("show config button opens borgmatic config file in a editor") {
    std::string filename{"file"};
    REQUIRE_CALL(*config, borgmaticConfigFile()).RETURN(filename);
    REQUIRE_CALL(*wrapperMock, openLocation(QString::fromStdString(filename)));
    configTab->findChild<QPushButton *>("configShowFileButton")->click();
  }

  SECTION("start backup button starts a backup, refreshes ui after completion") {
    auto startButton = configTab->findChild<QPushButton *>("startBackupButton");
    auto cancelButton = configTab->findChild<QPushButton *>("cancelBackupButton");
    REQUIRE(startButton->isEnabled() == true);
    REQUIRE(cancelButton->isEnabled() == false);

    std::function<void()> usedFinishedHandler;
    REQUIRE_CALL(*config, startBackup(_, _)).LR_SIDE_EFFECT(usedFinishedHandler = _1);

    startButton->click();

    REQUIRE(startButton->isEnabled() == false);
    REQUIRE(cancelButton->isEnabled() == true);

    ALLOW_CALL(*config, isBackupPurging()).RETURN(false);
    ALLOW_CALL(*config, isMountPointToBeOpened()).RETURN(false);
    REQUIRE_CALL(*config, info()).RETURN(prepareInfo());
    REQUIRE_CALL(*config, list()).RETURN(prepareList());

    usedFinishedHandler();
    wait_for_qthreads_to_finish();

    REQUIRE(startButton->isEnabled() == true);
    REQUIRE(cancelButton->isEnabled() == false);
  }

  SECTION("cancel backup button cancels a running backup and refreshes the ui") {
    auto startButton = configTab->findChild<QPushButton *>("startBackupButton");
    auto cancelButton = configTab->findChild<QPushButton *>("cancelBackupButton");
    REQUIRE(startButton->isEnabled() == true);
    REQUIRE(cancelButton->isEnabled() == false);

    REQUIRE_CALL(*config, startBackup(_, _));
    startButton->click();

    REQUIRE(startButton->isEnabled() == false);
    REQUIRE(cancelButton->isEnabled() == true);

    REQUIRE_CALL(*config, cancelBackup());
    ALLOW_CALL(*config, isBackupPurging()).RETURN(false);
    ALLOW_CALL(*config, isMountPointToBeOpened()).RETURN(false);
    REQUIRE_CALL(*config, info()).RETURN(prepareInfo());
    REQUIRE_CALL(*config, list()).RETURN(prepareList());

    cancelButton->click();
    wait_for_qthreads_to_finish();

    REQUIRE(startButton->isEnabled() == true);
    REQUIRE(cancelButton->isEnabled() == false);
  }

  SECTION("mount and umount button") {
    auto mountButton = configTab->findChild<QPushButton *>("backupMountButton");
    auto umountButton = configTab->findChild<QPushButton *>("backupUmountButton");
    auto backupsTable = configTab->findChild<QTableView *>("backupsTableView");
    std::string mountPoint = "some_directory";
    REQUIRE(mountButton->isEnabled() == false);

    auto model = qobject_cast<BackupListModel *>(backupsTable->model());
    QItemSelectionModel *selectionModel = backupsTable->selectionModel();
    size_t row = 0;
    backupsTable->selectRow(row);
    REQUIRE(mountButton->isEnabled() == true);
    REQUIRE(umountButton->isEnabled() == false);

    REQUIRE_CALL(*wrapperMock, selectMountPoint(_)).RETURN(QString::fromStdString(mountPoint));
    REQUIRE_CALL(*config, mountArchive(eq(prepareList()[row].name), eq(mountPoint)));
    REQUIRE_CALL(*config, isMountPointToBeOpened()).RETURN(true);
    REQUIRE_CALL(*wrapperMock, openLocation(eq(QString::fromStdString(mountPoint))));
    mountButton->click();

    REQUIRE(selectionModel->hasSelection() == false);
    REQUIRE(model->rowData(row).is_mounted == true);
    REQUIRE(model->rowData(row).mount_path == mountPoint);

    backupsTable->selectRow(row);
    REQUIRE(mountButton->isEnabled() == false);
    REQUIRE(umountButton->isEnabled() == true);

    REQUIRE_CALL(*config, umountArchive(eq(mountPoint)));
    umountButton->click();

    REQUIRE(selectionModel->hasSelection() == false);
    REQUIRE(model->rowData(row).is_mounted == false);
    REQUIRE(model->rowData(row).mount_path.empty());
  }

  SECTION("mount button but no mount point selected") {
    auto mountButton = configTab->findChild<QPushButton *>("backupMountButton");
    auto umountButton = configTab->findChild<QPushButton *>("backupUmountButton");
    auto backupsTable = configTab->findChild<QTableView *>("backupsTableView");
    REQUIRE(mountButton->isEnabled() == false);

    auto model = qobject_cast<BackupListModel *>(backupsTable->model());
    QItemSelectionModel *selectionModel = backupsTable->selectionModel();
    size_t row = 0;
    backupsTable->selectRow(row);
    REQUIRE(mountButton->isEnabled() == true);
    REQUIRE(umountButton->isEnabled() == false);

    REQUIRE_CALL(*wrapperMock, selectMountPoint(_)).RETURN(QString());
    mountButton->click();

    REQUIRE(selectionModel->hasSelection() == false);
    REQUIRE(model->rowData(row).is_mounted == false);
    REQUIRE(model->rowData(row).mount_path.empty());
  }

  SECTION("mount button does not open directory with unchecked checkbox") {
    auto mountButton = configTab->findChild<QPushButton *>("backupMountButton");
    auto backupsTable = configTab->findChild<QTableView *>("backupsTableView");
    std::string mountPoint = "some_directory";
    size_t row = 0;
    backupsTable->selectRow(row);

    REQUIRE_CALL(*wrapperMock, selectMountPoint(_)).RETURN(QString::fromStdString(mountPoint));
    REQUIRE_CALL(*config, mountArchive(eq(prepareList()[row].name), eq(mountPoint)));
    REQUIRE_CALL(*config, isMountPointToBeOpened()).RETURN(false);
    FORBID_CALL(*wrapperMock, openLocation(_));
    mountButton->click();
  }
}
