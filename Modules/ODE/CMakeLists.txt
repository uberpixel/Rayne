cmake_minimum_required(VERSION 2.8.4)
project(RayneODE)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

option(BUILD_SHARED_LIBS "Build shared libraries." OFF)
option(ODE_16BIT_INDICES "Use 16-bit indices for trimeshes (default is 32-bit)." OFF)
option(ODE_NO_BUILTIN_THREADING_IMPL "Disable built-in multithreaded threading implementation." OFF)
option(ODE_NO_THREADING_INTF "Disable threading interface support (external implementations cannot be assigned." OFF)
option(ODE_OLD_TRIMESH "Use old OPCODE trimesh-trimesh collider." OFF)
option(ODE_WITH_DEMOS "Builds the demo applications and DrawStuff library." OFF)
option(ODE_WITH_GIMPACT "Use GIMPACT for trimesh collisions (experimental)." OFF)
option(ODE_WITH_LIBCCD "Use libccd for handling some collision tests absent in ODE." ON)
option(ODE_WITH_OPCODE "Use old OPCODE trimesh-trimesh collider." ON)
option(ODE_WITH_OU "Use TLS for global caches (allows threaded collision checks for separated spaces)." OFF)
option(ODE_WITH_TESTS "Builds the unit test application." OFF)
add_subdirectory("Vendor/ode")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNODEInit.cpp
        RNODEWorld.cpp
        RNODECollisionObject.cpp
        RNODEBody.cpp
        RNODEShape.cpp
        RNODEInternals.cpp)

set(HEADERS
		RNODE.h
        RNODEWorld.h
        RNODECollisionObject.h
        RNODEBody.h
        RNODEShape.h
        RNODEInternals.h)

set(DEFINES RN_BUILD_ODE)

set(RAYNE_LIBRARIES Rayne ODE)
set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ode/include")

rayne_add_library(RayneODE "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneODE)

target_include_directories(RayneODE SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneODE PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneODE/. DESTINATION lib/Rayne/RayneODE)
