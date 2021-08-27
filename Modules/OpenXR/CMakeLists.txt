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
		"${CMAKE_CURRENT_BINARY_DIR}/OpenXRSDK-prefix/src/OpenXRSDK-build/src/loader/Debug/openxr_loaderd.lib"
		"${CMAKE_CURRENT_BINARY_DIR}/OpenXRSDK-prefix/src/OpenXRSDK-build/src/loader/Release/openxr_loader.lib"
	INSTALL_COMMAND "")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
	RNOpenXRInit.cpp
	RNOpenXRInternals.cpp
	RNOpenXRSwapChain.cpp
	RNOpenXRWindow.cpp)

set(HEADERS
	RNOpenXR.h
	RNOpenXRInternals.h
	RNOpenXRSwapChain.h
	RNOpenXRWindow.h)

set(DEFINES RN_BUILD_OPENXR)

if(ANDROID)
	set(DEFINES ${DEFINES} XR_USE_PLATFORM_ANDROID)
elseif(WIN32)
	set(DEFINES ${DEFINES} XR_USE_PLATFORM_WIN32)
elseif(UNIX AND NOT APPLE)
	set(DEFINES ${DEFINES} XR_USE_PLATFORM_XLIB)
endif()

if(MSVC AND ${RN_BUILD_D3D12_MODULE})
	set(SOURCES ${SOURCES} RNOpenXRD3D12SwapChain.cpp)
	set(HEADERS ${HEADERS} RNOpenXRD3D12SwapChain.h)
endif()

if(NOT APPLE AND ${RN_BUILD_VULKAN_MODULE})
	set(SOURCES ${SOURCES} RNOpenXRVulkanSwapChain.cpp)
	set(HEADERS ${HEADERS} RNOpenXRVulkanSwapChain.h)
endif()

ExternalProject_Get_property(OpenXRSDK SOURCE_DIR)
set(OpenXRSDK_SOURCE_DIR ${SOURCE_DIR})

set(RAYNE_LIBRARIES Rayne RayneVR)
set(INCLUDE_DIRECTORIES
	"${Rayne_BINARY_DIR}/include"
	"${CMAKE_CURRENT_BINARY_DIR}/../VRWrapper/include")

rayne_add_library(RayneOpenXR "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOpenXR)
add_dependencies(RayneOpenXR OpenXRSDK)

if(ANDROID)
	target_link_libraries(RayneOpenXR debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR/Libs/Android/arm64-v8a/Debug/libopenxr_loader.so optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR/Libs/Android/arm64-v8a/Release/libopenxr_loader.so)
	set(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR/Include)
elseif(NOT APPLE)
	target_link_libraries(RayneOpenXR debug ${CMAKE_CURRENT_BINARY_DIR}/OpenXRSDK-prefix/src/OpenXRSDK-build/src/loader/Debug/openxr_loaderd.lib optimized ${CMAKE_CURRENT_BINARY_DIR}/OpenXRSDK-prefix/src/OpenXRSDK-build/src/loader/Release/openxr_loader.lib)
endif()

if(MSVC AND ${RN_BUILD_D3D12_MODULE})
	target_link_libraries(RayneOpenXR RayneD3D12)
	set(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${RayneD3D12_BINARY_DIR}/include "${CMAKE_CURRENT_BINARY_DIR}/../D3D12/include/Sources")
	set(DEFINES ${DEFINES} XR_USE_GRAPHICS_API_D3D12)
endif()

if(NOT APPLE AND ${RN_BUILD_VULKAN_MODULE})
	ExternalProject_Get_property(vulkan_headers SOURCE_DIR)
	set(VulkanHeaders_SOURCE_DIR ${SOURCE_DIR})

	target_link_libraries(RayneOpenXR RayneVulkan)
	set(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES}
		${RayneVulkan_BINARY_DIR}/include
		"${VulkanHeaders_SOURCE_DIR}/include/vulkan"
		"${CMAKE_CURRENT_BINARY_DIR}/../Vulkan/include/Sources")
	set(DEFINES ${DEFINES} XR_USE_GRAPHICS_API_VULKAN)
endif()

target_include_directories(RayneOpenXR SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}" ${OpenXRSDK_SOURCE_DIR}/include)
target_compile_definitions(RayneOpenXR PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOpenXR/. DESTINATION lib/Rayne/RayneOpenXR)
