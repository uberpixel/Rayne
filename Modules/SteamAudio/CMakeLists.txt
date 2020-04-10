cmake_minimum_required(VERSION 2.8.4)
project(RayneSteamAudio)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

ExternalProject_Add(libsoundio
        GIT_REPOSITORY https://github.com/andrewrk/libsoundio.git
        GIT_TAG refs/tags/2.0.0
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CMAKE_CACHE_ARGS
                "-DBUILD_STATIC_LIBS:BOOL=True"
                "-DBUILD_DYNAMIC_LIBS:BOOL=False"
                "-DBUILD_EXAMPLE_PROGRAMS:BOOL=False"
                "-DBUILD_TESTS:BOOL=False"
                "-DENABLE_JACK:BOOL=True"
                "-DENABLE_PULSEAUDIO:BOOL=True"
                "-DENABLE_ALSA:BOOL=True"
                "-DENABLE_COREAUDIO:BOOL=True"
                "-DENABLE_WASAPI:BOOL=True"
        BUILD_BYPRODUCTS
                "${CMAKE_CURRENT_BINARY_DIR}/libsoundio-prefix/src/libsoundio-build/Debug/libsoundio.a"
                "${CMAKE_CURRENT_BINARY_DIR}/libsoundio-prefix/src/libsoundio-build/Release/libsoundio.a"
        INSTALL_COMMAND "")


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
        set(RAYNE_LIBRARIES Rayne ${COREAUDIO_LIB} ${AUDIOUNIT_LIBRARY} ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/steamaudio/lib/OSX/libphonon.dylib)
elseif(WIN32)
        set(RAYNE_LIBRARIES Rayne ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/steamaudio/lib/Windows/x64/phonon.lib)
elseif(ANDROID)
        set(RAYNE_LIBRARIES Rayne ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/steamaudio/lib/Android/libphonon.so)
endif()

set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_BINARY_DIR}/libsoundio-prefix/src/libsoundio"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/steamaudio/include")

rayne_add_library(RayneSteamAudio "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneSteamAudio)

target_include_directories(RayneSteamAudio SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneSteamAudio PRIVATE "${DEFINES}")

add_dependencies(RayneSteamAudio libsoundio)
target_link_libraries(RayneSteamAudio 
        debug "${CMAKE_CURRENT_BINARY_DIR}/libsoundio-prefix/src/libsoundio-build/Debug/libsoundio.a"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/libsoundio-prefix/src/libsoundio-build/Release/libsoundio.a")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneSteamAudio/. DESTINATION lib/Rayne/RayneSteamAudio)

if(APPLE)
        add_custom_command(TARGET RayneSteamAudio PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/steamaudio/lib/OSX/libphonon.dylib" "$<TARGET_FILE_DIR:RayneSteamAudio>/libphonon.dylib")
elseif(WIN32)
        add_custom_command(TARGET RayneSteamAudio PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/steamaudio/bin/Windows/x64/phonon.dll" "$<TARGET_FILE_DIR:RayneSteamAudio>/phonon.dll")
elseif(ANDROID)
        add_custom_command(TARGET RayneSteamAudio PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/steamaudio/lib/Android/libphonon.so" "$<TARGET_FILE_DIR:RayneSteamAudio>/libphonon.so")
endif()
