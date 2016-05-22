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
        Sources/RNMetalDevice.cpp
        Sources/RNMetalFramebuffer.cpp
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
        Sources/RNMetalInit.cpp)

set(RESOURCES
        Resources)

set(HEADERS
        Sources/RNMetal.h
        Sources/RNMetalDevice.h
        Sources/RNMetalFramebuffer.h
        Sources/RNMetalGPUBuffer.h
        Sources/RNMetalInternals.h
        Sources/RNMetalRenderer.h
        Sources/RNMetalRendererDescriptor.h
        Sources/RNMetalShader.h
        Sources/RNMetalShaderLibrary.h
        Sources/RNMetalStateCoordinator.h
        Sources/RNMetalTexture.h
        Sources/RNMetalUniformBuffer.h
        Sources/RNMetalWindow.h)

set(DEFINES
        "RN_BUILD_METAL")

find_library(COCOA_LIBRARY Cocoa)
find_library(METAL_LIBRARY Metal)
find_library(QUARTZCORE_LIBRARY QuartzCore)

set(RAYNE_LIBRARIES Rayne)
set(LINK_LIBRARIES ${COCOA_LIBRARY} ${QUARTZCORE_LIBRARY} ${METAL_LIBRARY})
set(INCLUDE_DIRECTORIES ${Rayne_BINARY_DIR}/include)

rayne_add_library(RayneMetal "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneMetal)
rayne_copy_resources(RayneMetal ${RESOURCES})

target_include_directories(RayneMetal SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_link_libraries(RayneMetal "${LINK_LIBRARIES}")
target_compile_options(RayneMetal PRIVATE "-xobjective-c++")
target_compile_definitions(RayneMetal PRIVATE "${DEFINES}")


rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneMetal/. DESTINATION lib/Rayne/RayneMetal)
