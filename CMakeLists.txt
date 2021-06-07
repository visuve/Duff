cmake_minimum_required(VERSION 3.16)

project(Duff LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

if (Qt6_FOUND)
	find_package(Qt6 COMPONENTS StateMachine REQUIRED)
endif()

file(GLOB DUFF_SOURCES "*.cpp" "*.hpp" ".ui" "*.qrc")

if (MSVC)
	set(APP_ICON_RESOURCE_WINDOWS "Duff.rc")
	set(DUFF_EXECUTABLE "Duff")
	add_executable(${DUFF_EXECUTABLE} ${DUFF_SOURCES} ${APP_ICON_RESOURCE_WINDOWS})
else()
	set(DUFF_EXECUTABLE "duff")
	add_executable(${DUFF_EXECUTABLE} ${DUFF_SOURCES})
endif()

target_link_libraries(${DUFF_EXECUTABLE} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if (Qt6_FOUND)
	target_link_libraries(${DUFF_EXECUTABLE} PRIVATE Qt6::StateMachine)
endif()