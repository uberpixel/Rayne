cmake_minimum_required(VERSION 2.8.4)
project(RayneSteamAudio)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

option(BUILD_STATIC_LIBS "Build static libraries" ON)
option(BUILD_DYNAMIC_LIBS "Build dynamic libraries" OFF)
option(BUILD_EXAMPLE_PROGRAMS "Build example programs" OFF)
option(BUILD_TESTS "Build tests" OFF)
option(ENABLE_JACK "Enable JACK backend" ON)
option(ENABLE_PULSEAUDIO "Enable PulseAudio backend" ON)
option(ENABLE_ALSA "Enable ALSA backend" ON)
option(ENABLE_COREAUDIO "Enable CoreAudio backend" ON)
option(ENABLE_WASAPI "Enable WASAPI backend" ON)
add_subdirectory("Vendor/libsoundio")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNSteamAudioInit.cpp
        RNSteamAudioWorld.cpp
        RNSteamAudioSampler.cpp
        RNSteamAudioSource.cpp
        RNSteamAudioPlayer.cpp
        RNSteamAudioInternals.cpp)

set(HEADERS
	RNSteamAudio.h
        RNSteamAudioWorld.h
        RNSteamAudioSampler.h
        RNSteamAudioSource.h
        RNSteamAudioPlayer.h
        RNSteamAudioInternals.h)

set(DEFINES RN_BUILD_STEAMAUDIO SOUNDIO_STATIC_LIBRARY)
if(APPLE)
        find_package(CoreAudio)
        find_library(AUDIOUNIT_LIBRARY NAMES AudioUnit)
        set(RAYNE_LIBRARIES Rayne ${COREAUDIO_LIBRARY} ${AUDIOUNIT_LIBRARY} libsoundio_static ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/steamaudio/lib/OSX/libphonon.dylib)
else()
        set(RAYNE_LIBRARIES Rayne libsoundio_static ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/steamaudio/lib/Windows/x64/phonon.lib)
endif()

set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/libsoundio"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/steamaudio/include")

rayne_add_library(RayneSteamAudio "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneSteamAudio)

target_include_directories(RayneSteamAudio SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneSteamAudio PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneSteamAudio/. DESTINATION lib/Rayne/RayneSteamAudio)

if(APPLE)
        add_custom_command(TARGET RayneSteamAudio PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/steamaudio/lib/OSX/libphonon.dylib" "$<TARGET_FILE_DIR:RayneSteamAudio>/libphonon.dylib")
endif()
