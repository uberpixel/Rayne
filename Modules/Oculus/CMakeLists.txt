cmake_minimum_required(VERSION 3.10.1)
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

        ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusSDK/LibOVR/Shim/OVR_StereoProjection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusSDK/LibOVR/Shim/OVR_CAPI_Util.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusSDK/LibOVR/Shim/OVR_CAPIShim.c)

set(HEADERS
	RNOculus.h
        RNOculusSwapChain.h
        RNOculusWindow.h)

set(DEFINES RN_BUILD_OCULUS)

set(RAYNE_LIBRARIES Rayne  RayneD3D12 RayneVR)
set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusSDK/LibOVR/Include"
        "${CMAKE_CURRENT_BINARY_DIR}/../D3D12/include/Sources"
        "${CMAKE_CURRENT_BINARY_DIR}/../VRWrapper/include")

rayne_add_library(RayneOculus "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOculus)

target_link_libraries(RayneOculus RayneD3D12)
target_include_directories(RayneOculus SYSTEM PRIVATE ${RayneD3D12_BINARY_DIR}/include)

target_include_directories(RayneOculus SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
#target_link_libraries(RayneOculus "${LINK_LIBRARIES}")
target_compile_definitions(RayneOculus PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOculus/. DESTINATION lib/Rayne/RayneOculus)
