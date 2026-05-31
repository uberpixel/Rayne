cmake_minimum_required(VERSION 3.28)
include(ExternalProject)

project(__TMP_APPLICATION_TARGET__)
set(LIBRARY_NAME ${PROJECT_NAME}Lib)
set(BUNDLE_DOMAIN __TMP_BUNDLE_DOMAIN__)
set(BUNDLE_ID __TMP_BUNDLE_ID__)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../Rayne/CMake/Rayne.cmake)
set(CMAKE_XCODE_GENERATE_TOP_LEVEL_PROJECT_ONLY ON)

option(RN_BUILD_BULLET_MODULE "Build the Bullet physics module" OFF)
option(RN_BUILD_PHYSX_MODULE "Build the PhysX physics module" OFF)
option(RN_BUILD_JOLT_MODULE "Build the Jolt physics module" ON)
option(RN_BUILD_ODE_MODULE "Build the ODE physics module" OFF)
option(RN_BUILD_SPLASH_MODULE "Build the Splash physics module" OFF)
option(RN_BUILD_ASSIMP_MODULE "Build the Assimp module" OFF)
option(RN_BUILD_STEAMAUDIO_MODULE "Build the Steam Audio module" OFF)
option(RN_BUILD_OCULUSAUDIO_MODULE "Build the Oculus Audio module" OFF)
option(RN_BUILD_RESONANCEAUDIO_MODULE "Build the Resonance Audio module" OFF)
option(RN_BUILD_ENET_MODULE "Build the ENet module" OFF)
option(RN_BUILD_SAMPLES "Enable building the sample projects" OFF)
option(RN_BUILD_RECAST_MODULE "Build the Recast pathfinding module" OFF)
option(RN_BUILD_OGG_MODULE "Build the ogg audio file module" OFF)
option(RN_BUILD_OPENAL_MODULE "Build the OpenAL module" OFF)
option(RN_BUILD_UI_MODULE "Build the UI module" ON)
option(RN_BUILD_OPENXR_MODULE "Build the OpenXR module" ON)
option(RN_BUILD_APPLEXR_MODULE "Build the AppleXR module" ON)
option(RN_BUILD_VRSTUB_MODULE "Build the VRStub module" ON)

set(RN_BUILD_CONFIGURATION "independent" CACHE STRING "The build configuration.")
set_property(CACHE RN_BUILD_CONFIGURATION PROPERTY STRINGS independent oculus steam pico)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)

if(IOS OR VISIONOS)
	enable_language(OBJCXX)
	enable_language(Swift)
endif()

if(IOS)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 15.0)
elseif(VISIONOS)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 1.0)
else()
	set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Rayne" "${CMAKE_CURRENT_BINARY_DIR}/Rayne")

if(WIN32)
	set(CMAKE_SYSTEM_VERSION "10.0.10240.0")
endif()

set(SOURCES
	Sources/main.cpp
	Sources/__TMP__Application.cpp
	Sources/__TMP__CameraManager.cpp
	Sources/__TMP__World.cpp)

set(HEADERS
	Sources/__TMP__Application.h
	Sources/__TMP__CameraManager.h
	Sources/__TMP__World.h
	Sources/__TMP_APPLICATION_TARGET__Lib.h)

set(RESOURCES
	manifest.json
	Resources)

if(IOS OR VISIONOS)
	set(SOURCES_SWIFT Sources/main-apple.swift)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

if(ANDROID)
	add_library(${PROJECT_NAME} SHARED "${SOURCES}" "${HEADERS}")
elseif(IOS OR VISIONOS)
	add_library(${LIBRARY_NAME} STATIC "${SOURCES}" "${HEADERS}")
	set_property(TARGET ${LIBRARY_NAME} PROPERTY PUBLIC_HEADER ${HEADERS})
	target_compile_options(${LIBRARY_NAME} PRIVATE -xobjective-c++)
	set_target_properties(${LIBRARY_NAME} PROPERTIES
		FRAMEWORK TRUE
		MACOSX_FRAMEWORK_IDENTIFIER ${BUNDLE_DOMAIN}.${LIBRARY_NAME}
		XCODE_ATTRIBUTE_DEFINES_MODULE YES
	)

	add_executable(${PROJECT_NAME} "${SOURCES_SWIFT}")
	set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_SWIFT_OBJC_INTEROP_MODE objcxx)
	target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBRARY_NAME})
else()
	add_executable(${PROJECT_NAME} "${SOURCES}" "${HEADERS}")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Sources)

set(ADDITIONAL_RESOURCE_COPY_PARAMS "--resourcespec=${CMAKE_CURRENT_SOURCE_DIR}/resources.json")
rayne_copy_resources(${PROJECT_NAME} "${RESOURCES}" "${ADDITIONAL_RESOURCE_COPY_PARAMS}")

rayne_link_with(${PROJECT_NAME})
rayne_use_modules(${PROJECT_NAME} RayneJolt)
rayne_use_modules(${PROJECT_NAME} RayneVR)

if(NOT APPLE)
	rayne_use_modules(${PROJECT_NAME} RayneOpenXR)
endif()

if(VISIONOS)
	rayne_use_modules(${PROJECT_NAME} RayneAppleXR)
endif()

if(NOT APPLE AND ${RN_BUILD_VULKAN_MODULE})
	rayne_use_modules(${PROJECT_NAME} RayneVulkan)
endif()

if(APPLE AND ${RN_BUILD_METAL_MODULE})
	rayne_use_modules(${PROJECT_NAME} RayneMetal)
endif()

if(${RN_BUILD_CONFIGURATION} STREQUAL "oculus")
	target_compile_definitions(${PROJECT_NAME} PRIVATE -DBUILD_FOR_OCULUS)
elseif(${RN_BUILD_CONFIGURATION} STREQUAL "steam")
	target_compile_definitions(${PROJECT_NAME} PRIVATE -DBUILD_FOR_STEAM)
elseif(${RN_BUILD_CONFIGURATION} STREQUAL "pico")
	target_compile_definitions(${PROJECT_NAME} PRIVATE -DBUILD_FOR_PICO)
elseif(WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Other/RunPancake-windows.bat $<TARGET_FILE_DIR:${PROJECT_NAME}>/RunPancake-windows.bat)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${PROJECT_NAME}> $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
		COMMAND_EXPAND_LISTS)
endif()

if(APPLE)
	if(IOS OR VISIONOS)
		set_target_properties(${PROJECT_NAME} PROPERTIES
			MACOSX_BUNDLE_SHORT_VERSION_STRING 0.0.1
			MACOSX_BUNDLE_BUNDLE_VERSION 1
		)
	endif()

	set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Other/Info.plist.in
		MACOSX_BUNDLE_GUI_IDENTIFIER ${BUNDLE_ID}

		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
		XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "YOUR_TEAM_HERE"
	)
endif()

if(MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()
