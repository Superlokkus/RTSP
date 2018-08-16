cmake_minimum_required(VERSION 3.0)
project(rtsp_gui VERSION 1.0)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#Only on my OSX MBP13
set(QTDIR "/opt/local/libexec/qt4" CACHE PATH "FOO" FORCE)
set(QT_QMAKE_EXECUTABLE "/opt/local/libexec/qt4/bin/qmake" CACHE FILEPATH "BAR" FORCE)

find_package(Qt4 REQUIRED)

include_directories(${Qt_INCLUDE_DIR})

set(gui_source_files
        src/rtsp_gui.cpp)

add_executable(${PROJECT_NAME} ${gui_source_files})
target_link_libraries(${PROJECT_NAME} Qt4::QtCore Qt4::QtDeclarative Qt4::QtGui)

