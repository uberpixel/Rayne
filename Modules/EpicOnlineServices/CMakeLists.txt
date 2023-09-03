cmake_minimum_required(VERSION 3.10.1)
project(RayneEOS)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

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
        RNEOSServer.cpp)

set(HEADERS
	RNEOS.h
        RNEOSWorld.h
        RNEOSLobbyManager.h
        RNEOSHost.h
        RNEOSClient.h
        RNEOSServer.h)

set(DEFINES RN_BUILD_EOS)

set(INCLUDE_DIRECTORIES "${Rayne_BINARY_DIR}/include")

add_library(EOSSDK SHARED IMPORTED)
if(ANDROID)
        set_property(TARGET EOSSDK PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK-Android/SDK/Bin/Android/static-stdc++/libs/${ANDROID_ABI}/libEOSSDK.so)
        set_property(TARGET EOSSDK PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK-Android/SDK/Include/)
        set(EOSSDK_DYNAMIC_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK-Android/SDK/Bin/Android/static-stdc++/libs/${ANDROID_ABI}/libEOSSDK.so)
elseif(WIN32)
        set_property(TARGET EOSSDK PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Bin/EOSSDK-Win64-Shipping.dll)
        set_property(TARGET EOSSDK PROPERTY IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Lib/EOSSDK-Win64-Shipping.lib)
        set_property(TARGET EOSSDK PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Include/)
        set(EOSSDK_DYNAMIC_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Bin/EOSSDK-Win64-Shipping.dll)
else()
        set_property(TARGET EOSSDK PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Bin/libEOSSDK-Mac-Shipping.dylib)
        set_property(TARGET EOSSDK PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Include/)
        set(EOSSDK_DYNAMIC_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Bin/libEOSSDK-Mac-Shipping.dylib)
endif()

if(WIN32)
        set(RAYNE_LIBRARIES Rayne EOSSDK)
else()
        set(RAYNE_LIBRARIES Rayne EOSSDK)
endif()


rayne_add_library(RayneEOS "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneEOS)

target_include_directories(RayneEOS SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneEOS PRIVATE "${DEFINES}")

if(ANDROID)
        #Nothing to do!?
else()
        add_custom_command(TARGET RayneEOS POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${EOSSDK_DYNAMIC_LIBRARY}" "$<TARGET_FILE_DIR:RayneEOS>")

        if(WIN32)
                add_custom_command(TARGET RayneEOS POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK/SDK/Bin/x64/xaudio2_9redist.dll" "$<TARGET_FILE_DIR:RayneEOS>")
        endif()
endif()

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneEOS/. DESTINATION lib/Rayne/RayneEOS)
