cmake_minimum_required(VERSION 3.9)

project(borgmatic-ui LANGUAGES CXX)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADERS
        include/mainwindow.h
        include/configtab.h)

set(SRC
        src/mainwindow.cpp
        src/configtab.cpp
        src/main.cpp)

set(UIS
        ui/mainwindow.ui
        ui/tabContent.ui)

add_executable(${PROJECT_NAME}
        ${UIS}
        ${HEADERS}
        ${SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOUIC_SEARCH_PATHS ui)

target_include_directories(${PROJECT_NAME} PRIVATE
        include)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Widgets)
