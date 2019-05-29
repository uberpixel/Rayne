cmake_minimum_required(VERSION 2.8.4)
project(RayneENet)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

add_subdirectory("Vendor/enet")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNENetInit.cpp
        RNENetWorld.cpp
        RNENetHost.cpp
        RNENetClient.cpp
        RNENetServer.cpp)

set(HEADERS
	RNENet.h
        RNENetWorld.h
        RNENetHost.h
        RNENetClient.h
        RNENetServer.h)

set(DEFINES RN_BUILD_ENET)

if(WIN32)
        set(RAYNE_LIBRARIES Rayne enet ws2_32.lib winmm.lib)
else()
        set(RAYNE_LIBRARIES Rayne enet)
endif()

set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/enet/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/steamaudio/include")

rayne_add_library(RayneENet "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneENet)

target_include_directories(RayneENet SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneENet PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneENet/. DESTINATION lib/Rayne/RayneENet)
