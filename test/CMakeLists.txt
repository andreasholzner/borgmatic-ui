find_package(Catch2 REQUIRED)
find_package(Qt5Test REQUIRED)

find_package(trompeloeil QUIET)
if (NOT trompeloeil_FOUND)
    include(FetchContent)
    FetchContent_Declare(trompeloeil
            GIT_REPOSITORY https://github.com/rollbear/trompeloeil.git
            GIT_TAG v39
            )
    FetchContent_MakeAvailable(trompeloeil)
endif ()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

list(TRANSFORM SRC PREPEND "../" OUTPUT_VARIABLE MAIN_SRC)
list(TRANSFORM HEADERS PREPEND "../" OUTPUT_VARIABLE MAIN_HEADERS)
list(TRANSFORM UIS PREPEND "../" OUTPUT_VARIABLE MAIN_UIS)
set(TESTS
        testMainWindow.cpp
        testConfigTab.cpp
        testBorgmaticManager.cpp
        testBackupConfig.cpp
        testBackupListModel.cpp)
add_executable(borgmatic_test
        test_main.cpp mocks.h ${TESTS}
        ${MAIN_SRC} ${MAIN_HEADERS} ${MAIN_UIS})
set_target_properties(borgmatic_test PROPERTIES AUTOUIC_SEARCH_PATHS ../ui)
target_include_directories(borgmatic_test PRIVATE ../include)
target_link_libraries(borgmatic_test PRIVATE Catch2::Catch2 Qt5::Test trompeloeil
        Qt5::Core Qt5::Widgets cereal spdlog::spdlog nlohmann_json::nlohmann_json Boost::headers)
