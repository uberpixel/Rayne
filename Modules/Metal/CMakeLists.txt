cmake_minimum_required(VERSION 2.8.4)
project(Metal)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/CopyResources.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        Sources/RNMetalGPUBuffer.cpp
        Sources/RNMetalInternals.cpp
        Sources/RNMetalRenderer.cpp
        Sources/RNMetalRendererDescriptor.cpp
        Sources/RNMetalShader.cpp
        Sources/RNMetalShaderLibrary.cpp
        Sources/RNMetalStateCoordinator.cpp
        Sources/RNMetalTexture.cpp
        Sources/RNMetalUniformBuffer.cpp
        Sources/RNMetalWindow.cpp
        Sources/init.cpp)

set(RESOURCES
        Resources)

set(HEADERS "")

find_library(COCOA_LIBRARY Cocoa)
find_library(METAL_LIBRARY Metal)
find_library(QUARTZCORE_LIBRARY QuartzCore)

set(RAYNE_LIBRARIES Rayne)
set(LINK_LIBRARIES ${COCOA_LIBRARY} ${QUARTZCORE_LIBRARY} ${METAL_LIBRARY})
set(INCLUDE_DIRECTORIES ${Rayne_BINARY_DIR}/include)

rayne_add_library(RayneMetal "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_target_add_options(RayneMetal "-xobjective-c++")
rayne_target_include_directories(RayneMetal "${INCLUDE_DIRECTORIES}")
rayne_target_link_libraries(RayneMetal "${LINK_LIBRARIES}")
rayne_copy_resources(RayneMetal ${RESOURCES})

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Build/. DESTINATION lib/Rayne/RayneMetal)
