cmake_minimum_required(VERSION 2.8.4)
project(RayneOpenAL)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

set(CMAKE_PLATFORM_NO_VERSIONED_SONAME 1)

option(ALSOFT_UTILS "Build and install utility programs" OFF)
option(ALSOFT_EXAMPLES "Build and install example programs" OFF)
option(ALSOFT_TESTS "Build and install test programs" OFF)
option(ALSOFT_CONFIG "Install alsoft.conf sample configuration file" OFF)
option(ALSOFT_HRTF_DEFS "Install HRTF definition files" OFF)
option(ALSOFT_AMBDEC_PRESETS "Install AmbDec preset files" OFF)
option(ALSOFT_INSTALL "Install headers and libraries" OFF)
option(ALSOFT_EMBED_HRTF_DATA "Embed the HRTF data files (increases library footprint)" ON)
add_subdirectory("Vendor/openal-soft")

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

set(RAYNE_LIBRARIES Rayne OpenAL)
set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/openal-soft/include")

rayne_add_library(RayneOpenAL "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOpenAL)

target_include_directories(RayneOpenAL SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneOpenAL PRIVATE "${DEFINES}")

#Work around some xcode specific soname issues...
if(${CMAKE_GENERATOR} STREQUAL "Xcode")
        SET_TARGET_PROPERTIES(OpenAL PROPERTIES VERSION "version")
        add_custom_command(TARGET RayneOpenAL PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:OpenAL>/libopenal.version.dylib" "$<TARGET_FILE_DIR:RayneOpenAL>/$<TARGET_FILE_NAME:OpenAL>")
        add_custom_command(TARGET RayneOpenAL PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:OpenAL>/libopenal.version.dylib" "$<TARGET_FILE_DIR:OpenAL>/$<TARGET_FILE_NAME:OpenAL>")
else()
        add_custom_command(TARGET RayneOpenAL PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:OpenAL>" "$<TARGET_FILE_DIR:RayneOpenAL>/$<TARGET_FILE_NAME:OpenAL>")
endif()

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOpenAL/. DESTINATION lib/Rayne/RayneOpenAL)
