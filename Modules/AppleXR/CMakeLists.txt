cmake_minimum_required(VERSION 3.10.1)
project(RayneAppleXR)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNAppleXRInit.cpp
        RNAppleXRWindow.cpp
        RNAppleXRSwapChain.cpp)

set(HEADERS
		RNAppleXR.h
        RNAppleXRWindow.h
        RNAppleXRSwapChain.h)

set(DEFINES RN_BUILD_APPLEXR APPLEXR_BUILD_STATIC)

find_library(COMPOSITORSERVICES_LIBRARY CompositorServices)
find_library(ARKIT_LIBRARY ARKit)
set(RAYNE_LIBRARIES Rayne RayneVR ${COMPOSITORSERVICES_LIBRARY} ${ARKIT_LIBRARY})
set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_BINARY_DIR}/../VRWrapper/include")

if(APPLE AND ${RN_BUILD_METAL_MODULE})
    set(SOURCES
        ${SOURCES}
        RNAppleXRMetalSwapChain.cpp)

    set(HEADERS
        ${HEADERS}
        RNAppleXRMetalSwapChain.h)

    set(RAYNE_LIBRARIES ${RAYNE_LIBRARIES} RayneMetal)

    set(INCLUDE_DIRECTORIES
        ${INCLUDE_DIRECTORIES}
        "${CMAKE_CURRENT_BINARY_DIR}/../Metal/include/Sources")
endif()

rayne_add_library(RayneAppleXR "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneAppleXR)

if(APPLE AND ${RN_BUILD_METAL_MODULE})
    target_compile_definitions(RayneAppleXR PRIVATE -DRN_APPLEXR_SUPPORTS_METAL)
    target_link_libraries(RayneAppleXR PUBLIC RayneMetal)
    target_include_directories(RayneAppleXR SYSTEM PRIVATE ${RayneMetal_BINARY_DIR}/include)
    target_compile_options(RayneAppleXR PRIVATE "-xobjective-c++")
    #FIND_LIBRARY(IOSURFACE_LIBRARY IOSurface)
    #target_link_libraries(RayneAppleXR PUBLIC ${IOSURFACE_LIBRARY})
endif()

target_include_directories(RayneAppleXR SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneAppleXR PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneAppleXR/. DESTINATION lib/Rayne/RayneAppleXR)
