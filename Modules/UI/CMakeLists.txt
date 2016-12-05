cmake_minimum_required(VERSION 2.8.4)
project(RayneUI)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/CopyResources.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        Sources/RNUIContext.cpp
        Sources/RNUIPath.cpp
        Sources/RNUIInit.cpp
        Sources/RNUIInternals.h)

set(RESOURCES
        Resources)

set(HEADERS
        Sources/RNUIContext.h
        Sources/RNUIPath.h
        Sources/RNUIConfig.h
        Sources/RNUI.h)

set(DEFINES
        "RN_BUILD_UI")

set(RAYNE_LIBRARIES Rayne ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/libskia/libskia.a)
set(INCLUDE_DIRECTORIES ${Rayne_BINARY_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/libskia/ ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/libskia/include/core ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/libskia/include/config)

rayne_add_library(RayneUI "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneUI)
#rayne_copy_resources(RayneUI ${RESOURCES})

target_include_directories(RayneUI SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneUI PRIVATE "${DEFINES}")


rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneUI/. DESTINATION lib/Rayne/RayneUI)
