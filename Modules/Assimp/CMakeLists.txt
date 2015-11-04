cmake_minimum_required(VERSION 2.8.4)
project(RayneAssimp)

include(../../CMake/TargetDirectories.cmake)
include(../../CMake/LibraryTarget.cmake)

set(ASSIMP_BUILD_TESTS off)
set(BUILD_SHARED_LIBS off)
set(ASSIMP_NO_EXPORT on)
set(ASSIMP_BUILD_ASSIMP_TOOLS off)

add_subdirectory(Vendor/Assimp)

target_directories(ASSIMP)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        init.cpp
        RNAssimpAssetLoader.cpp)

set(HEADERS "")

set(RAYNE_LIBRARIES Rayne)
set(LINK_LIBRARIES assimp)
set(INCLUDE_DIRECTORIES
        ${Rayne_BINARY_DIR}/include
        ${Assimp_SOURCE_DIR}/include)

rayne_add_library(RayneAssimp "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_target_include_directories(RayneAssimp "${INCLUDE_DIRECTORIES}")
rayne_target_link_libraries(RayneAssimp "${LINK_LIBRARIES}")