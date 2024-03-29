cmake_minimum_required(VERSION 3.10.1)
project(RayneOpenAL)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNOpenALInit.cpp
        RNOpenALWorld.cpp
        RNOpenALListener.cpp
        RNOpenALResourceAttachment.cpp
        RNOpenALSource.cpp)

set(HEADERS
	RNOpenAL.h
        RNOpenALWorld.h
        RNOpenALListener.h
        RNOpenALResourceAttachment.h
        RNOpenALSource.h)

set(DEFINES RN_BUILD_OPENAL AL_LIBTYPE_STATIC)

set(COMPILE_OPTIONS "")
set(RAYNE_LIBRARIES Rayne)

if(APPLE)
        set(COMPILE_OPTIONS -xobjective-c++)
        find_library(AVFOUNDATION_LIBRARY AVFoundation)
        set(RAYNE_LIBRARIES ${RAYNE_LIBRARIES} ${AVFOUNDATION_LIBRARY})
endif()

set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal/include")

rayne_add_library(RayneOpenAL "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOpenAL)

target_include_directories(RayneOpenAL SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneOpenAL PRIVATE "${DEFINES}")
target_compile_options(RayneOpenAL PRIVATE ${COMPILE_OPTIONS})

if(IOS OR VISIONOS)
        #Build static for iOS and visionOS. Since this module itself is dynamically linked and open source it should be fine with the license...
        ExternalProject_Add(openal
        GIT_REPOSITORY https://github.com/kcat/openal-soft.git
        GIT_TAG refs/tags/1.23.1
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CMAKE_CACHE_ARGS
                #"-DCMAKE_PLATFORM_NO_VERSIONED_SONAME:BOOL=True"
                "-DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}"
                "-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH:BOOL=False"
                "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
                "-DCMAKE_SYSTEM_NAME:STRING=${CMAKE_SYSTEM_NAME}"
                "-DCMAKE_OSX_SYSROOT:STRING=${CMAKE_OSX_SYSROOT}"
                "-DLIBTYPE:STRING=STATIC"
                "-DALSOFT_UTILS:BOOL=False"
                "-DALSOFT_EXAMPLES:BOOL=False"
                "-DALSOFT_TESTS:BOOL=False"
                "-DALSOFT_CONFIG:BOOL=False"
                "-DALSOFT_HRTF_DEFS:BOOL=False"
                "-DALSOFT_AMBDEC_PRESETS:BOOL=False"
                "-DALSOFT_INSTALL:BOOL=False"
                "-DALSOFT_EMBED_HRTF_DATA:BOOL=True"
                "-DALSOFT_REQUIRE_OPENSL:BOOL=${ANDROID}"
                "-DALSOFT_CPUEXT_NEON:BOOL=False"
                "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
                "-DANDROID_NATIVE_API_LEVEL:STRING=${ANDROID_NATIVE_API_LEVEL}"
                "-DANDROID_ABI:STRING=${ANDROID_ABI}"
                "-DANDROID_NDK:STRING=${ANDROID_NDK}"
                "-DANDROID_STL:STRING=${ANDROID_STL}"
                "-DCMAKE_SYSTEM_VERSION:STRING=${CMAKE_SYSTEM_VERSION}"
        BUILD_BYPRODUCTS
                ${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Debug-${APPLE_SDK_NAME}/libopenal.a
                ${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Release-${APPLE_SDK_NAME}/libopenal.a
        INSTALL_COMMAND "")
else()
        ExternalProject_Add(openal
                GIT_REPOSITORY https://github.com/kcat/openal-soft.git
                GIT_TAG refs/tags/1.23.1
                GIT_SHALLOW ON
                GIT_PROGRESS ON
                UPDATE_COMMAND ""
                CMAKE_CACHE_ARGS
                        #"-DCMAKE_PLATFORM_NO_VERSIONED_SONAME:BOOL=True"
                        "-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH:BOOL=False"
                        "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
                        "-DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}"
                        "-DCMAKE_SYSTEM_NAME:STRING=${CMAKE_SYSTEM_NAME}"
                        "-DCMAKE_OSX_SYSROOT:STRING=${CMAKE_OSX_SYSROOT}"
                        "-DALSOFT_UTILS:BOOL=False"
                        "-DALSOFT_EXAMPLES:BOOL=False"
                        "-DALSOFT_TESTS:BOOL=False"
                        "-DALSOFT_CONFIG:BOOL=False"
                        "-DALSOFT_HRTF_DEFS:BOOL=False"
                        "-DALSOFT_AMBDEC_PRESETS:BOOL=False"
                        "-DALSOFT_INSTALL:BOOL=False"
                        "-DALSOFT_EMBED_HRTF_DATA:BOOL=True"
                        "-DALSOFT_REQUIRE_OPENSL:BOOL=${ANDROID}"
                        "-DALSOFT_CPUEXT_NEON:BOOL=False"
                        "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
                        "-DANDROID_NATIVE_API_LEVEL:STRING=${ANDROID_NATIVE_API_LEVEL}"
                        "-DANDROID_ABI:STRING=${ANDROID_ABI}"
                        "-DANDROID_NDK:STRING=${ANDROID_NDK}"
                        "-DANDROID_STL:STRING=${ANDROID_STL}"
                        "-DCMAKE_SYSTEM_VERSION:STRING=${CMAKE_SYSTEM_VERSION}"
                BUILD_BYPRODUCTS
                        ${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Debug/libopenal.1.23.1.dylib
                        ${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Release/libopenal.1.23.1.dylib
                        ${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/libopenal.so
                        ${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Debug/OpenAL32.lib
                        ${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Release/OpenAL32.lib
                INSTALL_COMMAND "")
endif()

add_library(OpenALLib SHARED IMPORTED)
if(WIN32)
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo")

        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Debug/OpenAL32.dll")
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_IMPLIB_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Debug/OpenAL32.lib")
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Release/OpenAL32.dll")
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_IMPLIB_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Release/OpenAL32.lib")
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/RelWithDebInfo/OpenAL32.dll")
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_IMPLIB_RELWITHDEBINFO "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/RelWithDebInfo/OpenAL32.lib")
elseif(ANDROID)
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/libopenal.so)
elseif(IOS OR VISIONOS)
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo")
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Debug-${APPLE_SDK_NAME}/libopenal.a")
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Release-${APPLE_SDK_NAME}/libopenal.a")
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/RelWithDebInfo-${APPLE_SDK_NAME}/libopenal.a")
else()
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo")

        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Debug/libopenal.1.23.1.dylib")
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Release/libopenal.1.23.1.dylib")
        SET_PROPERTY(TARGET OpenALLib PROPERTY IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/RelWithDebInfo/libopenal.1.23.1.dylib")
endif()

add_dependencies(OpenALLib openal)
target_link_libraries(RayneOpenAL PUBLIC OpenALLib)

if(IOS OR VISIONOS)
        find_library(AUDIOTOOLBOX_LIB AudioToolbox)
        target_link_libraries(RayneOpenAL PUBLIC ${AUDIOTOOLBOX_LIB})
endif()


#Work around some xcode specific soname issues...
if(${CMAKE_GENERATOR} STREQUAL "Xcode" AND NOT IOS AND NOT VISIONOS)
        add_custom_command(TARGET RayneOpenAL PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/$<CONFIG>/libopenal.1.23.1.dylib" "$<TARGET_FILE_DIR:RayneOpenAL>/libopenal.1.dylib")
elseif(WIN32)
        add_custom_command(TARGET RayneOpenAL PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/$<CONFIG>/OpenAL32.dll" "$<TARGET_FILE_DIR:RayneOpenAL>/OpenAL32.dll")
endif()

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOpenAL/. DESTINATION lib/Rayne/RayneOpenAL)
