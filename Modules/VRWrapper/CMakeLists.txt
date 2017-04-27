cmake_minimum_required(VERSION 2.8.4)
project(RayneVR)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNVRWindow.cpp
        RNVRCamera.cpp)

set(HEADERS
	RNVR.h
        RNVRWindow.h
        RNVRCamera.h
        RNVRTrackingState.h)

set(DEFINES RN_BUILD_VR)

set(RAYNE_LIBRARIES Rayne RayneD3D12)
set(INCLUDE_DIRECTORIES "${Rayne_BINARY_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/../D3D12/include/Sources")

rayne_add_library(RayneVR "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneVR)

target_include_directories(RayneVR SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneVR PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneVR/. DESTINATION lib/Rayne/RayneVR)
