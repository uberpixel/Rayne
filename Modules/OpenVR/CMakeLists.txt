cmake_minimum_required(VERSION 2.8.4)
project(RayneOpenVR)

add_subdirectory("Vendor/OpenVR")

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNOpenVRInit.cpp
        RNOpenVRSwapChain.cpp
        RNOpenVRWindow.cpp)

set(HEADERS
		RNOpenVR.h
        RNOpenVRSwapChain.h
        RNOpenVRWindow.h)

set(DEFINES RN_BUILD_OPENVR)

set(RAYNE_LIBRARIES Rayne openvr_api64 RayneD3D12 RayneVR)
set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OpenVR/headers"
        "${CMAKE_CURRENT_BINARY_DIR}/../D3D12/include/Sources"
        "${CMAKE_CURRENT_BINARY_DIR}/../VRWrapper/include")

rayne_add_library(RayneOpenVR "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOpenVR)

rayne_use_modules(RayneOpenVR RayneD3D12)

target_include_directories(RayneOpenVR SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
#target_link_libraries(RayneOpenVR "${LINK_LIBRARIES}")
target_compile_definitions(RayneOpenVR PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOpenVR/. DESTINATION lib/Rayne/RayneOpenVR)
