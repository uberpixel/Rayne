cmake_minimum_required(VERSION 2.8.4)
project(RayneENet)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

add_subdirectory("Vendor/enet")

set(RN_ENET_USE_BOTAN_DTLS_ENCRYPTION 0)

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

if(ENET_USE_BOTAN_DTLS_ENCRYPTION)
        ExternalProject_Add(botan
                URL "https://github.com/randombit/botan/archive/2.12.1.tar.gz"
                CONFIGURE_COMMAND python ${CMAKE_CURRENT_BINARY_DIR}/botan-prefix/src/botan/configure.py --cc=clang --minimized-build --enable-modules=uuid,tls,auto_rng,system_rng
                BUILD_COMMAND make ./libbotan-2.a
                INSTALL_COMMAND "")
        add_dependencies(RayneENet botan)
        target_link_libraries(RayneENet ${CMAKE_CURRENT_BINARY_DIR}/botan-prefix/src/botan-build/libbotan-2.a)
        target_include_directories(RayneENet PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/botan-prefix/src/botan-build/build/include)

        target_compile_definitions(RayneENet PRIVATE "RN_ENET_USE_BOTAN_DTLS_ENCRYPTION")
endif()

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneENet/. DESTINATION lib/Rayne/RayneENet)
