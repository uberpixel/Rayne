cmake_minimum_required(VERSION 3.10.1)
project(Objects-Tests)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${Rayne_BINARY_DIR}/include)

add_executable(objectTests
        ObjectTests.cpp
        StringTests.cpp
        NumberTests.cpp
        KVOTests.cpp)

set(RESOURCES
        manifest.json)

rayne_copy_resources(${RESOURCES} objectTests)

target_link_libraries(objectTests ${gtest_LIBRARIES})
target_link_libraries(objectTests Rayne)