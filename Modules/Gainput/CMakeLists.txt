cmake_minimum_required(VERSION 2.8.4)
project(RayneGainput)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

option(GAINPUT_SAMPLES "Build Samples for Gainput" OFF)
option(GAINPUT_TESTS "Build Tests for Gainput" OFF)

add_subdirectory("Vendor/Gainput")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNGainputInit.cpp
        RNGainputManager.cpp
        RNGainputInternals.cpp)

set(HEADERS
	RNGainput.h
        RNGainputManager.h
        RNGainputInternals.h)

set(DEFINES RN_BUILD_GAINPUT)

set(RAYNE_LIBRARIES Rayne gainputstatic)
set(INCLUDE_DIRECTORIES "${Rayne_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Gainput/lib/include")

rayne_add_library(RayneGainput "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneGainput)

target_include_directories(RayneGainput SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneGainput PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneGainput/. DESTINATION lib/Rayne/RayneGainput)
