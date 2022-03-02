cmake_minimum_required(VERSION 3.10)
project(cbsp_test)
set(CMAKE_CXX_STANDARD 14)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_definitions(-DDEBUG)
include_directories(../)

add_executable(
    cbsp_test
    cbsp_buffer_test.cpp
)
target_link_libraries(
    cbsp_test
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread
)

include(GoogleTest)