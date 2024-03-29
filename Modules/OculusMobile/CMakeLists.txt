cmake_minimum_required(VERSION 3.10.1)
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

ExternalProject_Get_property(vulkan_headers SOURCE_DIR)
set(VulkanHeaders_SOURCE_DIR ${SOURCE_DIR})

set(RAYNE_LIBRARIES Rayne RayneVulkan RayneVR egl GLESv3)
set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_sdk_mobile_1/VrApi/Include"
        "${VulkanHeaders_SOURCE_DIR}/include/vulkan"
        "${CMAKE_CURRENT_BINARY_DIR}/../Vulkan/include/Sources"
        "${CMAKE_CURRENT_BINARY_DIR}/../VRWrapper/include")

rayne_add_library(RayneOculusMobile "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOculusMobile)
target_link_libraries(RayneOculusMobile PUBLIC debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_sdk_mobile_1/VrApi/Libs/Android/arm64-v8a/Debug/libvrapi.so optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_sdk_mobile_1/VrApi/Libs/Android/arm64-v8a/Release/libvrapi.so)

target_link_libraries(RayneOculusMobile PUBLIC RayneVulkan)
target_include_directories(RayneOculusMobile SYSTEM PRIVATE ${RayneVulkan_BINARY_DIR}/include)

target_include_directories(RayneOculusMobile SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneOculusMobile PRIVATE "${DEFINES}")

add_custom_command(TARGET RayneOculusMobile PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_sdk_mobile_1/VrApi/Libs/Android/arm64-v8a/Release/libvrapi.so "$<TARGET_FILE_DIR:RayneOculusMobile>")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOculusMobile/. DESTINATION lib/Rayne/RayneOculusMobile)
