#include <QSignalSpy>
#include <QVariant>
#include <catch2/catch.hpp>
#include <catch2/trompeloeil.hpp>
#include <memory>
#include <string>
#include <vector>
#include <QAbstractItemModel>

#include "BackupConfig.h"
#include "BackupListModel.h"

using namespace trompeloeil;

TEST_CASE("BackupListModel static properties", "[logic]") {
  BackupListModel model;

  SECTION("column count") {
    REQUIRE(model.columnCount() == 2);
  }

  SECTION("header data") {
    REQUIRE(model.headerData(0, Qt::Orientation::Horizontal, Qt::DisplayRole) == QVariant("Backup Name"));
    REQUIRE(model.headerData(1, Qt::Orientation::Horizontal, Qt::DisplayRole) == QVariant("Datum"));

    REQUIRE(model.headerData(2, Qt::Orientation::Horizontal, Qt::DisplayRole) == QVariant());
    REQUIRE(model.headerData(0, Qt::Orientation::Vertical, Qt::DisplayRole) == QVariant());
    REQUIRE(model.headerData(0, Qt::Orientation::Horizontal, Qt::ToolTipRole) == QVariant());
  }
}

TEST_CASE("BackupListModel dynamic properties", "[logic]") {
  BackupListModel model;
  std::vector<backup::helper::ListItem> modelData{{"id1", "name1", "2000-10-05 10:15:30.500", false},
                                                  {"id2", "name2", "2000-10-06 10:15:30.500", true, "/some/dir"}};
  model.updateBackups(modelData);

  SECTION("row count") { REQUIRE(model.rowCount() == modelData.size()); }

  SECTION("data") {
    REQUIRE(model.data(model.index(0, 0), Qt::DisplayRole) == QVariant(modelData[0].name.c_str()));
    REQUIRE(model.data(model.index(0, 1), Qt::DisplayRole) ==
            QVariant(QDateTime::fromString(modelData[0].start.c_str(), Qt::ISODateWithMs)));

    REQUIRE(model.data(model.index(2, 0), Qt::DisplayRole) == QVariant());
    REQUIRE(model.data(model.index(0, 2), Qt::DisplayRole) == QVariant());
  }

  SECTION("background color") {
    REQUIRE(model.data(model.index(0, 0), Qt::BackgroundRole) == QVariant());
    REQUIRE(model.data(model.index(1, 0), Qt::BackgroundRole).typeName() == std::string("QBrush"));
  }

  SECTION("rowData returns original backing data") {
    REQUIRE(model.rowData(0) == modelData[0]);
    REQUIRE(model.rowData(1) == modelData[1]);
  }

  SECTION("setMountInfos") {
    model.setMountInfos(0, true, "mount_point");

    REQUIRE(model.rowData(0).is_mounted == true);
    REQUIRE(model.rowData(0).mount_path == "mount_point");

    model.setMountInfos(1, false, "");

    REQUIRE(model.rowData(1).is_mounted == false);
    REQUIRE(model.rowData(1).mount_path == "");
  }
}

TEST_CASE("BackupListModel update behavior", "[logic]") {
  BackupListModel model;

  SECTION("updateBackups emits modelReset") {
    QSignalSpy spy{&model, &QAbstractItemModel::modelReset};
    std::vector<backup::helper::ListItem> modelData{{"id1", "name1", "2000-10-05 10:15:30.500", false},
                                                    {"id2", "name2", "2000-10-06 10:15:30.500", true, "/some/dir"}};

    model.updateBackups(modelData);

    REQUIRE(spy.count() == 1);
  }

  SECTION("setMountInfos emits dataChanged for affected row") {
    QSignalSpy spy{&model, &QAbstractItemModel::dataChanged};
    std::vector<backup::helper::ListItem> modelData{{"id1", "name1", "2000-10-05 10:15:30.500", false},
                                                    {"id2", "name2", "2000-10-06 10:15:30.500", true, "/some/dir"}};
    model.updateBackups(modelData);

    model.setMountInfos(1, true, "/other/mount/point");

    REQUIRE(spy.count() == 1);
    auto signalArguments = spy.takeFirst();
    REQUIRE(signalArguments.at(0).toModelIndex() == model.index(1, 0));
    REQUIRE(signalArguments.at(1).toModelIndex() == model.index(1, 1));
  }
}
