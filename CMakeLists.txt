cmake_minimum_required(VERSION 3.12)

project(borgmatic-ui LANGUAGES CXX)

find_package(Qt5 COMPONENTS Core Widgets Concurrent REQUIRED)
find_package(cereal REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Boost REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADERS
        include/MainWindow.h
        include/ConfigTab.h
        include/BackupConfig.h
        include/BackupConfig.inc
        include/BorgmaticManager.h
        include/BackupListModel.h
        include/BorgmaticBackupWorker.h
        include/DesktopServicesWrapper.h)

set(SRC
        src/MainWindow.cpp
        src/ConfigTab.cpp
        src/BorgmaticManager.cpp
        src/BackupListModel.cpp
        src/BorgmaticBackupWorker.cpp)

set(UIS
        ui/mainwindow.ui
        ui/tabContent.ui)

add_executable(${PROJECT_NAME} src/main.cpp
        ${UIS}
        ${HEADERS}
        ${SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOUIC_SEARCH_PATHS ui)

target_include_directories(${PROJECT_NAME} PRIVATE
        include)
target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core Qt5::Widgets cereal spdlog::spdlog nlohmann_json::nlohmann_json Boost::headers)

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(test)
endif ()
