cmake_minimum_required(VERSION 3.0)
project(rtsp VERSION 1.0)

set(CMAKE_CXX_STANDARD 14)

set(TEST_ENABLED TRUE CACHE BOOL "Tests are enabled")
set(PROJECT_RESSOURCE_DIR ${CMAKE_SOURCE_DIR}/res)

ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)
find_package(Boost COMPONENTS log system thread REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_subdirectory(src/streaming_lib)

if (${TEST_ENABLED})
    enable_testing()
    add_subdirectory(test)
endif ()

add_executable(rtsp_server_standalone src/rtsp_server_standalone.cpp)

target_link_libraries(rtsp_server_standalone
        ${Boost_LIBRARIES}
        streaming_lib)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#Only on my OSX MBP13
set(QTDIR "/opt/local/libexec/qt4" CACHE PATH "FOO" FORCE)
set(QT_QMAKE_EXECUTABLE "/opt/local/libexec/qt4/bin/qmake" CACHE FILEPATH "BAR" FORCE)

find_package(Qt4 REQUIRED)

include_directories(${Qt_INCLUDE_DIR})

set(gui_source_files
        src/rtsp_gui.cpp
        src/rtsp_gui/jpeg_rtsp_player.cpp
        src/rtsp_gui/jpeg_rtsp_player.hpp)

add_executable(rtsp_gui ${gui_source_files})

target_link_libraries(rtsp_gui
        Qt4::QtCore Qt4::QtDeclarative Qt4::QtGui Qt4::QtNetwork
        ${Boost_LIBRARIES}
        streaming_lib)

