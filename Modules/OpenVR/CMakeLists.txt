cmake_minimum_required(VERSION 2.8.4)
project(RayneOpenVR)

#option(BUILD_SHARED "Builds the library as shared library" ON)
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
        RNOpenVRWindow.cpp)

set(HEADERS
		RNOpenVR.h
        RNOpenVRWindow.h)

set(DEFINES RN_BUILD_OPENVR)

set(RAYNE_LIBRARIES Rayne RayneVR)
set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OpenVR/headers"
        "${CMAKE_CURRENT_BINARY_DIR}/../VRWrapper/include")


if(MSVC AND ${RN_BUILD_D3D12_MODULE})
    set(SOURCES
        ${SOURCES}
        RNOpenVRD3D12SwapChain.cpp)

    set(HEADERS
        ${HEADERS}
        RNOpenVRD3D12SwapChain.h)

    set(RAYNE_LIBRARIES ${RAYNE_LIBRARIES} openvr_api64 RayneD3D12)

    set(INCLUDE_DIRECTORIES
        ${INCLUDE_DIRECTORIES}
        "${CMAKE_CURRENT_BINARY_DIR}/../D3D12/include/Sources")
elseif(APPLE AND ${RN_BUILD_METAL_MODULE})
    set(SOURCES
        ${SOURCES}
        RNOpenVRMetalSwapChain.cpp)

    set(HEADERS
        ${HEADERS}
        RNOpenVRMetalSwapChain.h)

    set(RAYNE_LIBRARIES ${RAYNE_LIBRARIES} openvr_api RayneMetal)

    set(INCLUDE_DIRECTORIES
        ${INCLUDE_DIRECTORIES}
        "${CMAKE_CURRENT_BINARY_DIR}/../Metal/include/Sources")
endif()

rayne_add_library(RayneOpenVR "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOpenVR)

if(MSVC AND ${RN_BUILD_D3D12_MODULE})
    rayne_use_modules(RayneOpenVR RayneD3D12)
elseif(APPLE AND ${RN_BUILD_METAL_MODULE})
    rayne_use_modules(RayneOpenVR RayneMetal)
    target_compile_options(RayneOpenVR PRIVATE "-xobjective-c++")
    FIND_LIBRARY(IOSURFACE_LIBRARY IOSurface)
    target_link_libraries(RayneOpenVR ${IOSURFACE_LIBRARY})
endif()

target_include_directories(RayneOpenVR SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
#target_link_libraries(RayneOpenVR "${LINK_LIBRARIES}")
target_compile_definitions(RayneOpenVR PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOpenVR/. DESTINATION lib/Rayne/RayneOpenVR)
