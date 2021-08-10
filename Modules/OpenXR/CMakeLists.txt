cmake_minimum_required(VERSION 3.10.1)
project(RayneOpenXR)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

ExternalProject_Add(OpenXRSDK
        GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git
        GIT_TAG refs/tags/release-1.0.18
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CMAKE_CACHE_ARGS
                "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
                "-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH:BOOL=False"
                "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
                "-DANDROID_NATIVE_API_LEVEL:STRING=${ANDROID_NATIVE_API_LEVEL}"
                "-DANDROID_ABI:STRING=${ANDROID_ABI}"
                "-DANDROID_NDK:STRING=${ANDROID_NDK}"
                "-DANDROID_STL:STRING=${ANDROID_STL}"
        BUILD_BYPRODUCTS
                "${CMAKE_CURRENT_BINARY_DIR}/openxr-prefix/src/openxr/bin/${PLATFORM_NAME}/Debug/libopenvr_api.a"
                "${CMAKE_CURRENT_BINARY_DIR}/openxr-prefix/src/openxr/bin/${PLATFORM_NAME}/Release/libopenvr_api.a"
                "${CMAKE_CURRENT_BINARY_DIR}/openxr-prefix/src/openxr/bin/${PLATFORM_NAME}/libopenvr_api.a"
                "${CMAKE_CURRENT_BINARY_DIR}/openxr-prefix/src/openxr/bin/${PLATFORM_NAME}/Debug/openvr_api64.lib"
                "${CMAKE_CURRENT_BINARY_DIR}/openxr-prefix/src/openxr/bin/${PLATFORM_NAME}/Release/openvr_api64.lib"
        INSTALL_COMMAND "")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNOpenXRInit.cpp
        RNOpenXRInternals.cpp
        RNOpenXRVulkanSwapChain.cpp
        RNOpenXRWindow.cpp)

set(HEADERS
	RNOpenXR.h
        RNOpenXRInternals.h
        RNOpenXRVulkanSwapChain.h
        RNOpenXRWindow.h)

set(DEFINES RN_BUILD_OPENXR)

if(ANDROID)
        set(DEFINES ${DEFINES}
                XR_USE_PLATFORM_ANDROID
                XR_USE_GRAPHICS_API_VULKAN)
elseif(WIN32)
        set(DEFINES ${DEFINES}
                XR_USE_PLATFORM_WIN32
                XR_USE_GRAPHICS_API_VULKAN
                XR_USE_GRAPHICS_API_D3D12)
elseif(UNIX AND NOT APPLE)
        set(DEFINES ${DEFINES}
                XR_USE_PLATFORM_XLIB
                XR_USE_GRAPHICS_API_VULKAN)
endif()

ExternalProject_Get_property(vulkan_headers SOURCE_DIR)
set(VulkanHeaders_SOURCE_DIR ${SOURCE_DIR})

ExternalProject_Get_property(OpenXRSDK SOURCE_DIR)
set(OpenXRSDK_SOURCE_DIR ${SOURCE_DIR})

set(RAYNE_LIBRARIES Rayne RayneVulkan RayneVR egl GLESv3)
set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR/Include"
        "${VulkanHeaders_SOURCE_DIR}/include/vulkan"
        "${CMAKE_CURRENT_BINARY_DIR}/../Vulkan/include/Sources"
        "${CMAKE_CURRENT_BINARY_DIR}/../VRWrapper/include")

rayne_add_library(RayneOpenXR "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOpenXR)
target_link_libraries(RayneOpenXR debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR/Libs/Android/arm64-v8a/Debug/libopenxr_loader.so optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR/Libs/Android/arm64-v8a/Release/libopenxr_loader.so)
add_dependencies(RayneOpenXR OpenXRSDK)

target_link_libraries(RayneOpenXR RayneVulkan)
target_include_directories(RayneOpenXR SYSTEM PRIVATE ${RayneVulkan_BINARY_DIR}/include ${OpenXRSDK_SOURCE_DIR}/include)

target_include_directories(RayneOpenXR SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneOpenXR PRIVATE "${DEFINES}")

add_custom_command(TARGET RayneOpenXR PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR/Libs/Android/arm64-v8a/Release/libopenxr_loader.so "$<TARGET_FILE_DIR:RayneOpenXR>")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOpenXR/. DESTINATION lib/Rayne/RayneOpenXR)
