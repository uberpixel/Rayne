cmake_minimum_required(VERSION 2.8.4)
project(RayneAssimp)

include(../../CMake/LibraryTarget.cmake)

#Force some better default values...
option(BUILD_SHARED_LIBS "Build shared libraries." OFF)
option(ASSIMP_BUILD_TESTS  "If the test suite for Assimp is built in addition to the library." OFF)
option(ASSIMP_NO_EXPORT "Disable Assimp's export functionality." ON)
option(ASSIMP_BUILD_ASSIMP_TOOLS "If the supplementary tools for Assimp are built in addition to the library." OFF)

add_subdirectory(Vendor/Assimp)
target_compile_definitions(assimp PRIVATE -DNOUNCRYPT)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNAssimpInit.cpp
        RNAssimpAssetLoader.cpp)

set(HEADERS
        RNAssimpAssetLoader.h
        RNAssimp.h)

set(DEFINES
        RN_BUILD_ASSIMP)

set(RAYNE_LIBRARIES Rayne)
set(LINK_LIBRARIES assimp)
set(INCLUDE_DIRECTORIES
        ${Rayne_BINARY_DIR}/include
        ${Assimp_SOURCE_DIR}/include)

rayne_add_library(RayneAssimp "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneAssimp)

target_include_directories(RayneAssimp SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_link_libraries(RayneAssimp "${LINK_LIBRARIES}")
target_compile_definitions(RayneAssimp PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneAssimp/. DESTINATION lib/Rayne/RayneAssimp)