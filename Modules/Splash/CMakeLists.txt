cmake_minimum_required(VERSION 2.8.4)
project(RayneSplash)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNSplashInit.cpp
        RNSplashWorld.cpp
        RNSplashBody.cpp
        RNSplashShape.cpp)

set(HEADERS
	RNSplash.h
        RNSplashWorld.h
        RNSplashBody.h
        RNSplashShape.h)

set(DEFINES RN_BUILD_SPLASH)

set(RAYNE_LIBRARIES Rayne)
set(INCLUDE_DIRECTORIES "${Rayne_BINARY_DIR}/include")

rayne_add_library(RayneSplash "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneSplash)

target_include_directories(RayneSplash SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneSplash PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneSplash/. DESTINATION lib/Rayne/RayneSplash)
