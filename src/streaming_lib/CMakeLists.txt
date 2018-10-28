cmake_minimum_required(VERSION 3.0)
project(streaming_lib VERSION 1.0)

add_subdirectory(rtsp_lib)
add_subdirectory(rtp_lib)

set(${PROJECT_NAME}_implementation_files
        ${CMAKE_CURRENT_SOURCE_DIR}/src/streaming_lib.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/streaming_lib.hpp
        )

add_library(${PROJECT_NAME} ${${PROJECT_NAME}_implementation_files})
set_property(TARGET ${PROJECT_NAME} PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} rtsp_lib rtp_lib)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )

