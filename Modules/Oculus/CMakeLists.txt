cmake_minimum_required(VERSION 2.8.4)
project(RayneOculus)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNOculusInit.cpp
        RNOculusSwapChain.cpp
        RNOculusWindow.cpp
        RNOculusCamera.cpp
        RNOculusTrackingState.cpp)

set(HEADERS
	RNOculus.h
        RNOculusSwapChain.h
        RNOculusWindow.h
        RNOculusCamera.h
        RNOculusTrackingState.h)

set(DEFINES RN_BUILD_OCULUS)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(RAYNE_LIBRARIES Rayne ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusSDK/LibOVR/Lib/Windows/x64/Debug/VS2015/LibOVR.lib RayneD3D12)
else()
        set(RAYNE_LIBRARIES Rayne ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusSDK/LibOVR/Lib/Windows/x64/Release/VS2015/LibOVR.lib RayneD3D12)
endif()
set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusSDK/LibOVR/Include"
        "${CMAKE_CURRENT_BINARY_DIR}/../D3D12/include/Sources")

rayne_add_library(RayneOculus "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOculus)

rayne_use_modules(RayneOculus RayneD3D12)

target_include_directories(RayneOculus SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
#target_link_libraries(RayneOculus "${LINK_LIBRARIES}")
target_compile_definitions(RayneOculus PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOculus/. DESTINATION lib/Rayne/RayneOculus)
