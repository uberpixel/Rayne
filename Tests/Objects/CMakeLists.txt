cmake_minimum_required(VERSION 2.8.4)
project(Objects-Tests)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${Rayne_BINARY_DIR}/include)

add_executable(objectTests
        ObjectTests.cpp
        StringTests.cpp)

set(RESOURCES
        manifest.json)

copy_resources(${RESOURCES} objectTests)

target_link_libraries(objectTests gtest gtest_main)
target_link_libraries(objectTests Rayne-x64)