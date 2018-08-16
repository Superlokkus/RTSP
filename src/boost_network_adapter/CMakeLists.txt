cmake_minimum_required(VERSION 3.0)
project(boost_network_adapter VERSION 1.0)

find_package(Boost REQUIRED COMPONENTS system)
include_directories(${Boost_INCLUDE_DIR})

set(${PROJECT_NAME}_implementation_files
        ${CMAKE_CURRENT_SOURCE_DIR}/src/boost_network_adapter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/boost_network_adapter.hpp
        )


add_library(${PROJECT_NAME} ${${PROJECT_NAME}_implementation_files})
set_property(TARGET ${PROJECT_NAME} PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )
