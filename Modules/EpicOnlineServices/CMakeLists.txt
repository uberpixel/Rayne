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
        set_property(TARGET EOSSDK PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK-Android/SDK/Bin/Android/libs/${ANDROID_ABI}/libEOSSDK.so)
        set_property(TARGET EOSSDK PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK-Android/SDK/Bin/Android/include/)
        set(EOSSDK_DYNAMIC_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/EOS-SDK-Android/SDK/Bin/Android/libs/${ANDROID_ABI}/libEOSSDK.so)
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
        list(GET ANDROID_ASSETS_DIRECTORIES 0 android-assets-dir)
        string(LENGTH ${android-assets-dir} length)
        MATH(EXPR length "${length}-2")
        string(SUBSTRING ${android-assets-dir} 1 ${length} android-assets-dir)

        #add_custom_command(TARGET RayneEOS POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${EOSSDK_DYNAMIC_LIBRARY}" ${android-assets-dir}/../libs/arm64-v8a/libEOSSDK.so)
        add_custom_command(TARGET RayneEOS POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libs/arm64-v8a/libc++_shared.so" ${android-assets-dir}/../libs/arm64-v8a/libc++_shared.so)
else()
        add_custom_command(TARGET RayneEOS POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${EOSSDK_DYNAMIC_LIBRARY}" "$<TARGET_FILE_DIR:RayneEOS>")
endif()

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneEOS/. DESTINATION lib/Rayne/RayneEOS)
