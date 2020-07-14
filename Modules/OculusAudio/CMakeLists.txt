cmake_minimum_required(VERSION 2.8.4)
project(RayneOculusAudio)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

if(ANDROID)
        ExternalProject_Add(oboe
        GIT_REPOSITORY https://github.com/google/oboe.git
        GIT_TAG refs/tags/1.4.2
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CMAKE_CACHE_ARGS
                "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
                "-DANDROID_NATIVE_API_LEVEL:STRING=${ANDROID_NATIVE_API_LEVEL}"
                "-DANDROID_ABI:STRING=${ANDROID_ABI}"
                "-DANDROID_NDK:STRING=${ANDROID_NDK}"
                "-DANDROID_STL:STRING=${ANDROID_STL}"
        BUILD_BYPRODUCTS
                "${CMAKE_CURRENT_BINARY_DIR}/oboe-prefix/src/oboe-build/liboboe.a"
        INSTALL_COMMAND "")
else()
        ExternalProject_Add(rtaudio
        GIT_REPOSITORY https://github.com/thestk/rtaudio.git
        GIT_TAG refs/tags/5.1.0
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CMAKE_CACHE_ARGS
                "-DBUILD_SHARED_LIBS:BOOL=False"
        BUILD_BYPRODUCTS
                "${CMAKE_CURRENT_BINARY_DIR}/rtaudio-prefix/src/rtaudio-build/Debug/librtaudio.a"
                "${CMAKE_CURRENT_BINARY_DIR}/rtaudio-prefix/src/rtaudio-build/Release/librtaudio.a"
        INSTALL_COMMAND "")
endif()

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
        find_library(COREAUDIO_LIB CoreAudio)
        find_library(COREFOUNDATION_LIB CoreFoundation)
        set(RAYNE_LIBRARIES Rayne ${COREAUDIO_LIB} ${COREFOUNDATION_LIB} ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/Lib/macub/OVRAudio.framework/Versions/A/OVRAudio)
        set(INCLUDE_DIRECTORIES "${Rayne_BINARY_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/rtaudio-prefix/src/rtaudio" "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/Include")
elseif(WIN32)
        set(RAYNE_LIBRARIES Rayne winmm ole32 ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/Lib/x64/ovraudio64.lib)
        set(INCLUDE_DIRECTORIES "${Rayne_BINARY_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/rtaudio-prefix/src/rtaudio" "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/Include")
elseif(ANDROID)
        set(RAYNE_LIBRARIES Rayne ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/Lib/Android/arm64-v8a/libovraudiostatic64.a)
        set(INCLUDE_DIRECTORIES "${Rayne_BINARY_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/oboe-prefix/src/oboe/include" "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/Include")
endif()

rayne_add_library(RayneOculusAudio "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOculusAudio)

target_include_directories(RayneOculusAudio SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneOculusAudio PRIVATE "${DEFINES}")

if(ANDROID)
        add_dependencies(RayneOculusAudio oboe)
        target_link_libraries(RayneOculusAudio 
                "${CMAKE_CURRENT_BINARY_DIR}/oboe-prefix/src/oboe-build/liboboe.a")
else()
        add_dependencies(RayneOculusAudio rtaudio)
        target_link_libraries(RayneOculusAudio 
                debug "${CMAKE_CURRENT_BINARY_DIR}/rtaudio-prefix/src/rtaudio-build/Debug/librtaudio.a"
                optimized "${CMAKE_CURRENT_BINARY_DIR}/rtaudio-prefix/src/rtaudio-build/Release/librtaudio.a")
endif()

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOculusAudio/. DESTINATION lib/Rayne/RayneOculusAudio)

if(APPLE)
        add_custom_command(TARGET RayneOculusAudio PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/Lib/macub/OVRAudio.framework" "$<TARGET_FILE_DIR:RayneOculusAudio>/OVRAudio.framework")
endif()
if(ANDROID)
        list(GET ANDROID_ASSETS_DIRECTORIES 0 android-assets-dir)
        string(LENGTH ${android-assets-dir} length)
        MATH(EXPR length "${length}-2")
        string(SUBSTRING ${android-assets-dir} 1 ${length} android-assets-dir)

        add_custom_command(TARGET RayneOpenAL PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/OculusAudio/Lib/Android/arm64-v8a/libovraudio64.so ${android-assets-dir}/../libs/arm64-v8a/libovraudio64.so)
endif()
