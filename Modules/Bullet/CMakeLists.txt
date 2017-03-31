cmake_minimum_required(VERSION 2.8.4)
project(RayneBullet)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

OPTION(USE_GRAPHICAL_BENCHMARK "Use Graphical Benchmark" OFF)
OPTION(USE_MSVC_RUNTIME_LIBRARY_DLL "Use MSVC Runtime Library DLL (/MD or /MDd)" ON)
OPTION(BUILD_CPU_DEMOS "Build original Bullet CPU examples" OFF)
OPTION(USE_GLUT "Use Glut" OFF)
OPTION(BUILD_ENET "Set when you want to build apps with enet UDP networking support" OFF)
OPTION(BUILD_CLSOCKET "Set when you want to build apps with enet TCP networking support" OFF)
OPTION(BUILD_OPENGL3_DEMOS "Set when you want to build the OpenGL3+ demos" OFF)
OPTION(BUILD_BULLET2_DEMOS "Set when you want to build the Bullet 2 demos" OFF)
OPTION(BUILD_EXTRAS "Set when you want to build the extras" OFF)
OPTION(BUILD_UNIT_TESTS "Build Unit Tests" OFF)
OPTION(BUILD_BULLET3 "Set when you want to build Bullet 3" OFF)
add_subdirectory("Vendor/bullet")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNBulletInit.cpp
        RNBulletWorld.cpp
        RNBulletCollisionObject.cpp
        RNBulletKinematicController.cpp
        RNBulletMaterial.cpp
        RNBulletRigidBody.cpp
        RNBulletShape.cpp
        RNBulletConstraint.cpp
        RNBulletInternals.cpp)

set(HEADERS
	RNBullet.h
        RNBulletWorld.h
        RNBulletCollisionObject.h
        RNBulletKinematicController.h
        RNBulletMaterial.h
        RNBulletRigidBody.h
        RNBulletShape.h
        RNBulletConstraint.h
        RNBulletInternals.h)

set(DEFINES RN_BUILD_BULLET)

set(RAYNE_LIBRARIES Rayne BulletCollision BulletDynamics LinearMath)
set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/bullet/src")

rayne_add_library(RayneBullet "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneBullet)

rayne_use_modules(RayneBullet RayneD3D12)

target_include_directories(RayneBullet SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneBullet PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneBullet/. DESTINATION lib/Rayne/RayneBullet)
