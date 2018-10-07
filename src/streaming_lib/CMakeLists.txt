cmake_minimum_required(VERSION 3.0)
project(streaming_lib VERSION 1.0)

set(${PROJECT_NAME}_implementation_files
        ${CMAKE_CURRENT_SOURCE_DIR}/src/streaming_lib.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/streaming_lib.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rtsp_message.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtsp_message.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtsp_server_.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rtsp_server_.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtsp_session.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtsp_server_internals.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rtsp_methods.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtsp_methods.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtsp_definitions.hpp)


add_library(${PROJECT_NAME} ${${PROJECT_NAME}_implementation_files})
set_property(TARGET ${PROJECT_NAME} PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )

if (${TEST_ENABLED})
    enable_testing()
    set(streaming_lib_test_src
            ${CMAKE_CURRENT_SOURCE_DIR}/test/rtsp_grammar_test.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/test/rtsp_server_internals_test.cpp
            )
    foreach (testSrc ${streaming_lib_test_src})
        get_filename_component(testName ${testSrc} NAME_WE)
        add_executable(${PROJECT_NAME}_${testName} ${testSrc})
        set_property(TARGET ${PROJECT_NAME}_${testName} PROPERTY CXX_STANDARD 14)
        target_link_libraries(${PROJECT_NAME}_${testName} ${Boost_LIBRARIES} ${PROJECT_NAME})
        add_test(NAME ${PROJECT_NAME}_${testName} COMMAND ${PROJECT_NAME}_${testName} -m HRF -l warning --build_info)
    endforeach (testSrc)
endif ()
