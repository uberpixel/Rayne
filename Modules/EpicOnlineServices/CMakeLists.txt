cmake_minimum_required(VERSION 3.28)
project(RayneEOS)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNEOSInit.cpp
        RNEOSWorld.cpp
        RNEOSLobbyManager.cpp
        RNEOSHost.cpp
        RNEOSClient.cpp
        RNEOSP2PClient.cpp
        RNEOSServer.cpp)

set(HEADERS
        RNEOS.h
        RNEOSWorld.h
        RNEOSLobbyManager.h
        RNEOSHost.h
        RNEOSClient.h
        RNEOSP2PClient.h
        RNEOSServer.h)

set(DEFINES RN_BUILD_EOS)

set(EOSSDK_INCLUDE_DIR)
add_library(EOSSDK SHARED IMPORTED)
if(ANDROID)
        set(EOSSDK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK-Android/SDK/Include/)
        set_property(TARGET EOSSDK PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK-Android/SDK/Lib/libEOSSDK.so)
elseif(IOS OR VISIONOS)
        set(EOSSDK_FRAMEWORK ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK-IOS/SDK/Bin/IOS/EOSSDK.framework)
        set(EOSSDK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK-IOS/SDK/Bin/IOS/EOSSDK.framework/Headers)
        set_property(TARGET EOSSDK PROPERTY IMPORTED_LOCATION ${EOSSDK_FRAMEWORK})
elseif(WIN32)
        set(EOSSDK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Include/)
        set_property(TARGET EOSSDK PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Bin/EOSSDK-Win64-Shipping.dll)
        set_property(TARGET EOSSDK PROPERTY IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Lib/EOSSDK-Win64-Shipping.lib)
else()
        set(EOSSDK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Include/)
        set_property(TARGET EOSSDK PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Bin/libEOSSDK-Mac-Shipping.dylib)
endif()

rayne_add_library(RayneEOS "${SOURCES}" "${HEADERS}" "${VERSION_STRING}" "${VERSION_ABI}" PUBLIC_LIBRARIES Rayne)
rayne_set_module_output_directory(RayneEOS)

target_include_directories(RayneEOS SYSTEM PRIVATE ${EOSSDK_INCLUDE_DIR})
target_compile_definitions(RayneEOS PRIVATE "${DEFINES}")
target_link_libraries(RayneEOS PRIVATE ZLIB::ZLIB)

if(ANDROID OR IOS OR VISIONOS)
        target_link_libraries(RayneEOS PRIVATE EOSSDK)
        if(IOS OR VISIONOS)
                set_property(TARGET RayneEOS APPEND PROPERTY RAYNE_XCODE_EMBED_FRAMEWORKS "${EOSSDK_FRAMEWORK}")
        endif()
else()
        rayne_add_runtime_dependency(RayneEOS EOSSDK)

        if(WIN32)
                rayne_add_runtime_dependency(RayneEOS "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Bin/x64/xaudio2_9redist.dll")
        endif()
endif()
