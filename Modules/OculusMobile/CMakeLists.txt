cmake_minimum_required(VERSION 2.8.4)
project(RayneOculusMobile)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNOculusMobileInit.cpp
        RNOculusMobileVulkanSwapChain.cpp
        RNOculusMobileWindow.cpp)

set(HEADERS
	RNOculusMobile.h
        RNOculusMobileVulkanSwapChain.h
        RNOculusMobileWindow.h)

set(DEFINES RN_BUILD_OCULUSMOBILE)

set(RAYNE_LIBRARIES Rayne RayneVulkan RayneVR)
set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_sdk_mobile_1/VrApi/Include"
        "${CMAKE_CURRENT_BINARY_DIR}/../Vulkan/include/Sources"
        "${CMAKE_CURRENT_BINARY_DIR}/../VRWrapper/include")

rayne_add_library(RayneOculusMobile "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOculusMobile)
target_link_libraries(RayneOculusMobile debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_sdk_mobile_1/VrApi/Libs/Android/arm64-v8a/Debug/libvrapi.so optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_sdk_mobile_1/VrApi/Libs/Android/arm64-v8a/Release/libvrapi.so)

rayne_use_modules(RayneOculusMobile RayneVulkan)

target_include_directories(RayneOculusMobile SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneOculusMobile PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOculusMobile/. DESTINATION lib/Rayne/RayneOculusMobile)
