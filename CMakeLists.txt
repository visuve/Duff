cmake_minimum_required(VERSION 3.8)

project(TunkioGUI LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

file(GLOB DUFF_SOURCES "*.cpp" "*.hpp" ".ui" "*.qrc")

set(APP_ICON_RESOURCE_WINDOWS "Duff.rc")

if (MSVC)
	set(DUFF_EXECUTABLE "Duff")
	qt_add_executable(${DUFF_EXECUTABLE} ${DUFF_SOURCES} ${APP_ICON_RESOURCE_WINDOWS})
else()
	set(DUFF_EXECUTABLE "duff")
	qt_add_executable(${DUFF_EXECUTABLE} ${DUFF_SOURCES})
endif()

target_link_libraries(${DUFF_EXECUTABLE} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)