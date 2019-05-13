cmake_minimum_required(VERSION 2.8.4)
project(RayneOculusAudio)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

add_subdirectory("Vendor/rtaudio")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNOculusAudioInit.cpp
        RNOculusAudioWorld.cpp
        RNOculusAudioSystem.cpp
        RNOculusAudioSampler.cpp
        RNOculusAudioSource.cpp
        RNOculusAudioPlayer.cpp
        RNOculusAudioInternals.cpp)

set(HEADERS
	RNOculusAudio.h
        RNOculusAudioWorld.h
        RNOculusAudioSystem.h
        RNOculusAudioSampler.h
        RNOculusAudioSource.h
        RNOculusAudioPlayer.h
        RNOculusAudioInternals.h)

set(DEFINES RN_BUILD_OCULUS_AUDIO)
if(APPLE)
        set(RAYNE_LIBRARIES Rayne rtaudio ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/Lib/macub/OVRAudio.framework/Versions/A/OVRAudio)
elseif(WIN32)
        set(RAYNE_LIBRARIES Rayne rtaudio ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/Lib/x64/ovraudio64.lib)
elseif(ANDROID)
        set(RAYNE_LIBRARIES Rayne rtaudio ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/Lib/Android/arm64-v8a/libovraudiostatic64.a)
endif()

set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/rtaudio"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/Include")

rayne_add_library(RayneOculusAudio "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOculusAudio)

target_include_directories(RayneOculusAudio SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneOculusAudio PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOculusAudio/. DESTINATION lib/Rayne/RayneOculusAudio)

if(APPLE)
        add_custom_command(TARGET RayneOculusAudio PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/Lib/macub/OVRAudio.framework" "$<TARGET_FILE_DIR:RayneOculusAudio>/OVRAudio.framework")
elseif(WIN32)
        #add_custom_command(TARGET RayneOculusAudio PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/bin/Windows/x64/phonon.dll" "$<TARGET_FILE_DIR:RayneOculusAudio>/phonon.dll")
elseif(ANDROID)
        #add_custom_command(TARGET RayneOculusAudio PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/lib/Android/libphonon.so" "$<TARGET_FILE_DIR:RayneOculusAudio>/libphonon.so")
endif()
