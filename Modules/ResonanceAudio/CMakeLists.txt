cmake_minimum_required(VERSION 3.10.1)
project(RayneResonanceAudio)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

#Resonance does the audio simulation
ExternalProject_Add(resonance
        GIT_REPOSITORY https://github.com/Slin/resonance-audio.git
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        PATCH_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/resonance-prefix/src/resonance/third_party/clone_core_deps.sh"
        CMAKE_ARGS
                "-DCMAKE_CXX_STANDARD=17"
                "-DCMAKE_CXX_STANDARD_REQUIRED=True"
        CMAKE_CACHE_ARGS
                "-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH:BOOL=False"
                "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
                "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
                "-DANDROID_NATIVE_API_LEVEL:STRING=${ANDROID_NATIVE_API_LEVEL}"
                "-DANDROID_ABI:STRING=${ANDROID_ABI}"
                "-DANDROID_NDK:STRING=${ANDROID_NDK}"
                "-DANDROID_STL:STRING=${ANDROID_STL}"
                "-DBUILD_RESONANCE_AUDIO_API:BOOL=True"
                "-DCMAKE_SYSTEM_VERSION:STRING=${CMAKE_SYSTEM_VERSION}"
        BUILD_BYPRODUCTS
                "${CMAKE_CURRENT_BINARY_DIR}/resonance-prefix/src/resonance-build/resonance_audio/libResonanceAudioStatic.a"
        INSTALL_COMMAND "")

#Miniaudio is a single header library that offers low level multi platform audio in and output
ExternalProject_Add(miniaudio
        GIT_REPOSITORY https://github.com/mackron/miniaudio.git
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/miniaudio-prefix/src/miniaudio/miniaudio.h"
        INSTALL_COMMAND "")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNResonanceAudioInit.cpp
        RNResonanceAudioWorld.cpp
        RNResonanceAudioSystem.cpp
        RNResonanceAudioSampler.cpp
        RNResonanceAudioSource.cpp
        RNResonanceAudioInternals.cpp)

set(HEADERS
	RNResonanceAudio.h
        RNResonanceAudioWorld.h
        RNResonanceAudioSystem.h
        RNResonanceAudioSampler.h
        RNResonanceAudioSource.h
        RNResonanceAudioInternals.h)

set(DEFINES RN_BUILD_OCULUS_AUDIO)
set(INCLUDE_DIRECTORIES "${Rayne_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ResonanceAudio/Include" "${CMAKE_CURRENT_BINARY_DIR}/miniaudio-prefix/src/miniaudio" "${CMAKE_CURRENT_BINARY_DIR}/resonance-prefix/src/resonance/resonance_audio" "${CMAKE_CURRENT_BINARY_DIR}/resonance-prefix/src/resonance" "${CMAKE_CURRENT_BINARY_DIR}/resonance-prefix/src/resonance/third_party/eigen")

if(APPLE)
        find_library(COREAUDIO_LIB CoreAudio)
        find_library(COREFOUNDATION_LIB CoreFoundation)
        set(RAYNE_LIBRARIES Rayne ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})
elseif(WIN32)
        #set(RAYNE_LIBRARIES Rayne winmm ole32)
endif()

rayne_add_library(RayneResonanceAudio "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneResonanceAudio)

target_include_directories(RayneResonanceAudio SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneResonanceAudio PRIVATE "${DEFINES}")

add_dependencies(RayneResonanceAudio resonance miniaudio)
if(UNIX)
        target_link_libraries(RayneResonanceAudio PRIVATE
                "${CMAKE_CURRENT_BINARY_DIR}/resonance-prefix/src/resonance-build/resonance_audio/libResonanceAudioStatic.a")
else(WIN32)
        #TODO: Link with correct libraries
endif()

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneResonanceAudio/. DESTINATION lib/Rayne/RayneResonanceAudio)

