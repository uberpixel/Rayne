cmake_minimum_required(VERSION 2.8.4)
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

set(DEFINES RN_BUILD_OPENAL)

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


ExternalProject_Add(openal
        GIT_REPOSITORY https://github.com/kcat/openal-soft.git
        GIT_TAG refs/tags/openal-soft-1.20.1
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CMAKE_CACHE_ARGS
                #"-DCMAKE_PLATFORM_NO_VERSIONED_SONAME:BOOL=True"
                "-DALSOFT_UTILS:BOOL=False"
                "-DALSOFT_EXAMPLES:BOOL=False"
                "-DALSOFT_TESTS:BOOL=False"
                "-DALSOFT_CONFIG:BOOL=False"
                "-DALSOFT_HRTF_DEFS:BOOL=False"
                "-DALSOFT_AMBDEC_PRESETS:BOOL=False"
                "-DALSOFT_INSTALL:BOOL=False"
                "-DALSOFT_EMBED_HRTF_DATA:BOOL=True"
                "-DALSOFT_REQUIRE_OPENSL:BOOL=${ANDROID}"
                "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
                "-DANDROID_NATIVE_API_LEVEL:STRING=${ANDROID_NATIVE_API_LEVEL}"
                "-DANDROID_ABI:STRING=${ANDROID_ABI}"
                "-DANDROID_NDK:STRING=${ANDROID_NDK}"
                "-DANDROID_STL:STRING=${ANDROID_STL}"
        BUILD_BYPRODUCTS
                ${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Debug/libopenal.1.20.1.dylib
                ${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Release/libopenal.1.20.1.dylib
                ${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/libopenal.so
        INSTALL_COMMAND "")

add_dependencies(RayneOpenAL openal)

if(APPLE)
        target_link_libraries(RayneOpenAL 
                debug "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Debug/libopenal.1.20.1.dylib"
                optimized "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Release/libopenal.1.20.1.dylib")
elseif(ANDROID)
        list(GET ANDROID_ASSETS_DIRECTORIES 0 android-assets-dir)
        string(LENGTH ${android-assets-dir} length)
        MATH(EXPR length "${length}-2")
        string(SUBSTRING ${android-assets-dir} 1 ${length} android-assets-dir)

        target_link_libraries(RayneOpenAL 
                ${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/libopenal.so)
        add_custom_command(TARGET RayneOpenAL PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/libopenal.so ${android-assets-dir}/../libs/arm64-v8a/libopenal.so)
endif()

#Work around some xcode specific soname issues...
if(${CMAKE_GENERATOR} STREQUAL "Xcode")
        add_custom_command(TARGET RayneOpenAL PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/openal-prefix/src/openal-build/Debug/libopenal.1.20.1.dylib" "$<TARGET_FILE_DIR:RayneOpenAL>/libopenal.1.dylib")
else()
#        add_custom_command(TARGET RayneOpenAL PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:OpenAL>" "$<TARGET_FILE_DIR:RayneOpenAL>/$<TARGET_FILE_NAME:OpenAL>")
endif()

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOpenAL/. DESTINATION lib/Rayne/RayneOpenAL)
