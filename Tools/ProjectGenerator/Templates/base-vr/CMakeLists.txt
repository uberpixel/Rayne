cmake_minimum_required(VERSION 3.10.1)
include(ExternalProject)

project(__TMP_APPLICATION_TARGET__)

set(CMAKE_XCODE_GENERATE_TOP_LEVEL_PROJECT_ONLY ON)

option(RN_BUILD_BULLET_MODULE "Build the Bullet physics module" OFF)
option(RN_BUILD_PHYSX_MODULE "Build the PhysX physics module" OFF)
option(RN_BUILD_JOLT_MODULE "Build the Jolt physics module" ON)
option(RN_BUILD_ODE_MODULE "Build the ODE physics module" OFF)
option(RN_BUILD_SPLASH_MODULE "Build the Splash physics module" OFF)
option(RN_BUILD_ASSIMP_MODULE "Build the Assimp module" OFF)
option(RN_BUILD_STEAMAUDIO_MODULE "Build the Steam Audio module" OFF)
option(RN_BUILD_OCULUSAUDIO_MODULE "Build the Oculus Audio module" OFF)
option(RN_BUILD_ENET_MODULE "Build the ENet module" OFF)
option(RN_BUILD_SAMPLES "Enable building the sample projects" OFF)
option(RN_BUILD_RECAST_MODULE "Build the Recast pathfinding module" OFF)
option(RN_BUILD_OGG_MODULE "Build the ogg audio file module" OFF)
option(RN_BUILD_OPENAL_MODULE "Build the OpenAL module" OFF)
option(RN_BUILD_UI_MODULE "Build the UI module" OFF)
option(RN_BUILD_OPENXR_MODULE "Build the OpenXR module" ON)
option(RN_BUILD_VRSTUB_MODULE "Build the VRStub module" ON)
if(WIN32)
	set(CMAKE_SYSTEM_VERSION "10.0.10240.0")
endif()

set(RN_BUILD_CONFIGURATION "independent" CACHE STRING "The build configuration.")
set_property(CACHE RN_BUILD_CONFIGURATION PROPERTY STRINGS independent oculus steam pico)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
if(APPLE)
	if(IOS OR VISIONOS)
		set(CMAKE_INSTALL_RPATH @executable_path;@executable_path/Frameworks)
	else()
		set(CMAKE_INSTALL_RPATH @executable_path;@executable_path/../Resources/Modules/RayneJolt;@executable_path/../Resources/Modules/RayneMetal;@executable_path/../Resources/Modules/RayneOpenXR;@executable_path/../Resources/Modules/RayneVR)
	endif()
elseif(UNIX)
	set(CMAKE_INSTALL_RPATH \$ORIGIN;\$ORIGIN/Resources/Modules/RayneJolt;\$ORIGIN/Resources/Modules/RayneVulkan;\$ORIGIN/Resources/Modules/RayneOpenXR;\$ORIGIN/Resources/Modules/RayneVR)
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Rayne" "${CMAKE_CURRENT_BINARY_DIR}/Rayne")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../Rayne/CMake/Rayne.cmake)

set(SOURCES
	Sources/main.cpp
	Sources/__TMP__Application.cpp
	Sources/__TMP__CameraManager.cpp
	Sources/__TMP__World.cpp)

set(HEADERS
	Sources/__TMP__Application.h
	Sources/__TMP__CameraManager.h
	Sources/__TMP__World.h)

set(RESOURCES
	manifest.json
	Resources)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

if(ANDROID)
    add_library(__TMP_APPLICATION_TARGET__ SHARED "${SOURCES}" "${HEADERS}")
else()
    add_executable(__TMP_APPLICATION_TARGET__ "${SOURCES}" "${HEADERS}")
endif()

target_include_directories(__TMP_APPLICATION_TARGET__ PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Sources)

set(ADDITIONAL_RESOURCE_COPY_PARAMS "--resourcespec=${CMAKE_CURRENT_SOURCE_DIR}/resources.json")
rayne_copy_resources(__TMP_APPLICATION_TARGET__ "${RESOURCES}" "${ADDITIONAL_RESOURCE_COPY_PARAMS}")

rayne_link_with(__TMP_APPLICATION_TARGET__)
rayne_use_modules(__TMP_APPLICATION_TARGET__ RayneJolt)
rayne_use_modules(__TMP_APPLICATION_TARGET__ RayneVR)

if(NOT APPLE)
	rayne_use_modules(__TMP_APPLICATION_TARGET__ RayneOpenXR)
endif()

if(NOT APPLE AND ${RN_BUILD_VULKAN_MODULE})
	rayne_use_modules(__TMP_APPLICATION_TARGET__ RayneVulkan)
endif()

if(MSVC AND ${RN_BUILD_D3D12_MODULE})
	rayne_use_modules(__TMP_APPLICATION_TARGET__ RayneD3D12)
endif()

if(APPLE AND ${RN_BUILD_METAL_MODULE})
	rayne_use_modules(__TMP_APPLICATION_TARGET__ RayneMetal)
endif()

if(${RN_BUILD_CONFIGURATION} STREQUAL "oculus")
	set(__TMP___COMPILE_DEFINITIONS ${__TMP___COMPILE_DEFINITIONS} -DBUILD_FOR_OCULUS)
elseif(${RN_BUILD_CONFIGURATION} STREQUAL "steam")
	set(__TMP___COMPILE_DEFINITIONS ${__TMP___COMPILE_DEFINITIONS} -DBUILD_FOR_STEAM)
elseif(${RN_BUILD_CONFIGURATION} STREQUAL "pico")
	set(__TMP___COMPILE_DEFINITIONS ${__TMP___COMPILE_DEFINITIONS} -DBUILD_FOR_PICO)
elseif(WIN32)
	add_custom_command(TARGET __TMP_APPLICATION_TARGET__ POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Other/RunPancake-windows.bat $<TARGET_FILE_DIR:__TMP_APPLICATION_TARGET__>/RunPancake-windows.bat)
endif()

target_compile_definitions(__TMP_APPLICATION_TARGET__ PRIVATE ${__TMP___COMPILE_DEFINITIONS})

if(APPLE)
	set_target_properties(__TMP_APPLICATION_TARGET__ PROPERTIES MACOSX_BUNDLE TRUE)
	set_target_properties(__TMP_APPLICATION_TARGET__ PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Other/Info.plist.in)
endif()

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT __TMP_APPLICATION_TARGET__)
endif()
