cmake_minimum_required(VERSION 2.8.4)
project(RayneBullet)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

ExternalProject_Add(bullet
        GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
        GIT_TAG refs/tags/2.89
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        CMAKE_CACHE_ARGS
                "-DUSE_GRAPHICAL_BENCHMARK:BOOL=False"
                "-DUSE_MSVC_RUNTIME_LIBRARY_DLL:BOOL=True"
                "-DBUILD_CPU_DEMOS:BOOL=False"
                "-DUSE_GLUT:BOOL=False"
                "-DBUILD_ENET:BOOL=False"
                "-DBUILD_CLSOCKET:BOOL=False"
                "-DBUILD_OPENGL3_DEMOS:BOOL=False"
                "-DBUILD_BULLET2_DEMOS:BOOL=False"
                "-DBUILD_EXTRAS:BOOL=False"
                "-DBUILD_UNIT_TESTS:BOOL=False"
                "-DBUILD_BULLET3:BOOL=False"
        BUILD_BYPRODUCTS
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletDynamics/Debug/libBulletDynamics.a"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletCollision/Debug/libBulletCollision.a"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/LinearMath/Debug/libLinearMath.a"

                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletDynamics/Release/libBulletDynamics.a"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletCollision/Release/libBulletCollision.a"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/LinearMath/Release/libLinearMath.a"
        INSTALL_COMMAND "")

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

set(RAYNE_LIBRARIES Rayne)
set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet/src")

rayne_add_library(RayneBullet "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneBullet)

add_dependencies(RayneBullet bullet)

target_link_libraries(RayneBullet 
        debug "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletDynamics/Debug/libBulletDynamics.a"
        debug "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletCollision/Debug/libBulletCollision.a"
        debug "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/LinearMath/Debug/libLinearMath.a"

        optimized "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletDynamics/Release/libBulletDynamics.a"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletCollision/Release/libBulletCollision.a"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/LinearMath/Release/libLinearMath.a")

target_include_directories(RayneBullet SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneBullet PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneBullet/. DESTINATION lib/Rayne/RayneBullet)
