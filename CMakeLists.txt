cmake_minimum_required(VERSION 2.8.4)
project(RayneAll)

# General settings, independent of target
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_definitions(-DDEBUG=1)

add_subdirectory("Source")
add_subdirectory("Tests")