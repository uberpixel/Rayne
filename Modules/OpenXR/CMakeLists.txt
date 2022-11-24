cmake_minimum_required(VERSION 3.10.1)
project(RayneOpenXR)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

option(RN_OPENXR_SUPPORT_METAQUEST "Include the loader library for Meta Quest" ON)
option(RN_OPENXR_SUPPORT_PICO "Include the loader library for Pico android devices" ON)

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

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Pico_OpenXR_Mobile_SDK_2/OpenXR)
	set(RN_OPENXR_SUPPORT_PICO OFF)
endif()


if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR)
	set(RN_OPENXR_SUPPORT_METAQUEST OFF)
endif()

if(ANDROID)
	set(DEFINES ${DEFINES} XR_USE_PLATFORM_ANDROID XR_NO_PROTOTYPES) #Use indirect linking on android to allow different loaders
	set(SOURCES ${SOURCES} RNOpenXRDispatchTable.c)
	set(HEADERS ${HEADERS} RNOpenXRDispatchTable.h)

	if(RN_OPENXR_SUPPORT_PICO)
		set(DEFINES ${DEFINES} RN_OPENXR_SUPPORTS_PICO_LOADER)

		execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Pico_OpenXR_Mobile_SDK_2/OpenXR/Libs/Android/arm64-v8a/libopenxr_loader.so ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Pico_OpenXR_Mobile_SDK_2/OpenXR/Libs/Android/arm64-v8a/libopenxr_loader_pico.so)
		execute_process(COMMAND patchelf --set-soname libopenxr_loader_pico.so ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Pico_OpenXR_Mobile_SDK_2/OpenXR/Libs/Android/arm64-v8a/libopenxr_loader_pico.so)

		add_library(OpenXRPico SHARED IMPORTED)
		set_target_properties(OpenXRPico PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Pico_OpenXR_Mobile_SDK_2/OpenXR/Libs/Android/arm64-v8a/libopenxr_loader_pico.so INTERFACE_LINK_LIBRARIES "" IMPORTED_IMPLIB "")
	endif()

	if(RN_OPENXR_SUPPORT_METAQUEST)
		set(DEFINES ${DEFINES} RN_OPENXR_SUPPORTS_METAQUEST_LOADER)
		add_library(OpenXRMeta SHARED IMPORTED)

		if(${RN_BUILD_CONFIGURATION} STREQUAL "oculus")
			#Do not rename when building for oculus, as the build will fail processing on the store if the library name is not libopenxr_loader.so
			#Hopfully the quest specifics can be removed soon, if they start supporting the official android OpenXR loader.
			set_target_properties(OpenXRMeta PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR/Libs/Android/arm64-v8a/Release/libopenxr_loader.so INTERFACE_LINK_LIBRARIES "" IMPORTED_IMPLIB "")
		else()
			execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR/Libs/Android/arm64-v8a/Release/libopenxr_loader.so ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR/Libs/Android/arm64-v8a/Release/libopenxr_loader_meta.so)
			execute_process(COMMAND patchelf --set-soname libopenxr_loader_meta.so ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR/Libs/Android/arm64-v8a/Release/libopenxr_loader_meta.so)
			set_target_properties(OpenXRMeta PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR/Libs/Android/arm64-v8a/Release/libopenxr_loader_meta.so INTERFACE_LINK_LIBRARIES "" IMPORTED_IMPLIB "")
		endif()
	endif()

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
	if(RN_OPENXR_SUPPORT_PICO)
		set(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Pico_OpenXR_Mobile_SDK_2/OpenXR/Include/openxr) #include directory for pico specific openxr header
		target_link_libraries(RayneOpenXR PRIVATE OpenXRPico)
	endif()
	if(RN_OPENXR_SUPPORT_METAQUEST)
		target_link_libraries(RayneOpenXR PRIVATE OpenXRMeta)
	endif()
elseif(NOT APPLE)
	target_link_libraries(RayneOpenXR PUBLIC debug ${CMAKE_CURRENT_BINARY_DIR}/OpenXRSDK-prefix/src/OpenXRSDK-build/src/loader/Debug/openxr_loaderd.lib optimized ${CMAKE_CURRENT_BINARY_DIR}/OpenXRSDK-prefix/src/OpenXRSDK-build/src/loader/Release/openxr_loader.lib)
endif()

#Include files from the meta openxr sdk for features that aren't part of the openxr specification yet
if(RN_OPENXR_SUPPORT_METAQUEST)
	set(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ovr_openxr_mobile_sdk_1/OpenXR/Include) #include directory for meta specific openxr headers
endif()

if(MSVC AND ${RN_BUILD_D3D12_MODULE})
	target_link_libraries(RayneOpenXR PUBLIC RayneD3D12)
	set(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${RayneD3D12_BINARY_DIR}/include "${CMAKE_CURRENT_BINARY_DIR}/../D3D12/include/Sources")
	set(DEFINES ${DEFINES} XR_USE_GRAPHICS_API_D3D12)
endif()

if(NOT APPLE AND ${RN_BUILD_VULKAN_MODULE})
	ExternalProject_Get_property(vulkan_headers SOURCE_DIR)
	set(VulkanHeaders_SOURCE_DIR ${SOURCE_DIR})

	target_link_libraries(RayneOpenXR PUBLIC RayneVulkan)
	set(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES}
		${RayneVulkan_BINARY_DIR}/include
		"${VulkanHeaders_SOURCE_DIR}/include/vulkan"
		"${CMAKE_CURRENT_BINARY_DIR}/../Vulkan/include/Sources")
	set(DEFINES ${DEFINES} XR_USE_GRAPHICS_API_VULKAN)
endif()

target_include_directories(RayneOpenXR SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}" ${OpenXRSDK_SOURCE_DIR}/include ${OpenXRSDK_SOURCE_DIR}/src/common)
target_compile_definitions(RayneOpenXR PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOpenXR/. DESTINATION lib/Rayne/RayneOpenXR)
