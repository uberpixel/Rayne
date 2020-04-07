cmake_minimum_required(VERSION 2.8.4)
project(RayneENet)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

ExternalProject_Add(enet
        GIT_REPOSITORY https://github.com/lsalzman/enet.git
        GIT_TAG origin/master
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        BUILD_BYPRODUCTS
                "${CMAKE_CURRENT_BINARY_DIR}/enet-prefix/src/enet-build/Debug/libenet.a"
                "${CMAKE_CURRENT_BINARY_DIR}/enet-prefix/src/enet-build/Release/libenet.a"
        INSTALL_COMMAND "")

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
        RNENetServer.cpp
        RNENetInternals.cpp)

set(HEADERS
	RNENet.h
        RNENetWorld.h
        RNENetHost.h
        RNENetClient.h
        RNENetServer.h
        RNENetInternals.h)

set(DEFINES RN_BUILD_ENET)

if(WIN32)
        set(RAYNE_LIBRARIES Rayne ws2_32.lib winmm.lib)
else()
        set(RAYNE_LIBRARIES Rayne)
endif()

set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_BINARY_DIR}/enet-prefix/src/enet/include")

rayne_add_library(RayneENet "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneENet)

target_include_directories(RayneENet SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneENet PRIVATE "${DEFINES}")

add_dependencies(RayneENet enet)
target_link_libraries(RayneENet 
        debug "${CMAKE_CURRENT_BINARY_DIR}/enet-prefix/src/enet-build/Debug/libenet.a"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/enet-prefix/src/enet-build/Release/libenet.a")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneENet/. DESTINATION lib/Rayne/RayneENet)
