cmake_minimum_required(VERSION 2.8.4)
project(Metal)

include(../../CMake/TargetDirectories.cmake)
include(../../CMake/LibraryTarget.cmake)

target_directories(METAL)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNMetalGPUBuffer.cpp
        RNMetalInternals.cpp
        RNMetalRenderer.cpp
        RNMetalRendererDescriptor.cpp
        RNMetalShader.cpp
        RNMetalShaderLibrary.cpp
        RNMetalStateCoordinator.cpp
        RNMetalTexture.cpp
        RNMetalUniformBuffer.cpp
        RNMetalWindow.cpp
        init.cpp)

set(HEADERS "")

find_library(COCOA_LIBRARY Cocoa)
find_library(METAL_LIBRARY Metal)
find_library(QUARTZCORE_LIBRARY QuartzCore)

set(RAYNE_LIBRARIES Rayne)
set(LINK_LIBRARIES ${COCOA_LIBRARY} ${QUARTZCORE_LIBRARY} ${METAL_LIBRARY})
set(INCLUDE_DIRECTORIES ${Rayne_BINARY_DIR}/include)

rayne_add_library(Metal "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_target_add_options(Metal "-xobjective-c++")
rayne_target_include_directories(Metal "${INCLUDE_DIRECTORIES}")
rayne_target_link_libraries(Metal "${LINK_LIBRARIES}")