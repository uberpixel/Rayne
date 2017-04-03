cmake_minimum_required(VERSION 2.8.4)
project(RayneOgg)

include(../../CMake/LibraryTarget.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNOggInit.cpp
        RNOggAssetLoader.cpp)

set(HEADERS
        RNOggAssetLoader.h
        RNOgg.h)

set(DEFINES
        RN_BUILD_OGG)

set(RAYNE_LIBRARIES Rayne)
set(INCLUDE_DIRECTORIES
        ${Rayne_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/Vendor)

rayne_add_library(RayneOgg "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOgg)

target_include_directories(RayneOgg SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneOgg PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOgg/. DESTINATION lib/Rayne/RayneOgg)
