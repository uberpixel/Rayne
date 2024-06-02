cmake_minimum_required(VERSION 3.10.1)
project(RayneUIComponents)

include(../../CMake/LibraryTarget.cmake)
include(ExternalProject)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNUICWindow.cpp
        RNUICComponent.cpp
        RNUICEventManager.cpp)

set(RESOURCES
        Resources)

set(HEADERS
        RNUIC.h
        RNUICWindow.h
        RNUICComponent.h
        RNUICEventManager.h)

set(DEFINES
        "RN_BUILD_UICOMPONENTS") 

set(RAYNE_LIBRARIES Rayne)

set(INCLUDE_DIRECTORIES ${Rayne_BINARY_DIR}/include)

rayne_add_library(RayneUIComponents "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneUIComponents)

target_include_directories(RayneUIComponents SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneUIComponents PRIVATE "${DEFINES}")


rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneUIComponents/. DESTINATION lib/Rayne/RayneUIComponents)
