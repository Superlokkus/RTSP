cmake_minimum_required(VERSION 3.0)
project(streaming_lib VERSION 1.0)

set(${PROJECT_NAME}_implementation_files
        ${CMAKE_CURRENT_SOURCE_DIR}/src/streaming_lib.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/streaming_lib.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rtsp_message.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtsp_message.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtsp_server_.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rtsp_server_.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtsp_session.hpp)


add_library(${PROJECT_NAME} ${${PROJECT_NAME}_implementation_files})
set_property(TARGET ${PROJECT_NAME} PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )

if (${TEST_ENABLED})
    enable_testing()
    add_executable(${PROJECT_NAME}_tests ${CMAKE_CURRENT_SOURCE_DIR}/test/${PROJECT_NAME}_tests.cpp)
    set_property(TARGET ${PROJECT_NAME}_tests PROPERTY CXX_STANDARD 14)
    target_link_libraries(${PROJECT_NAME}_tests ${Boost_LIBRARIES} ${PROJECT_NAME})
    add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests -m HRF -l warning --build_info)
endif ()
