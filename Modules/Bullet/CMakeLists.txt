cmake_minimum_required(VERSION 3.10.1)
project(RayneBullet)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

set(BULLET_PATCH_COMMAND "")
if(WIN32)
        set(BULLET_PATCH_COMMAND powershell -Command "(Get-Content ${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet/src/LinearMath/btScalar.h).replace('#define BT_DEBUG', '') | Set-Content ${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet/src/LinearMath/btScalar.h")
endif()

ExternalProject_Add(bullet
        GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
        GIT_TAG refs/tags/3.06
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        PATCH_COMMAND ${BULLET_PATCH_COMMAND}
        CMAKE_CACHE_ARGS
                "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
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
                "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
                "-DANDROID_NATIVE_API_LEVEL:STRING=${ANDROID_NATIVE_API_LEVEL}"
                "-DANDROID_ABI:STRING=${ANDROID_ABI}"
                "-DANDROID_NDK:STRING=${ANDROID_NDK}"
                "-DANDROID_STL:STRING=${ANDROID_STL}"
                "-DCMAKE_SYSTEM_VERSION:STRING=${CMAKE_SYSTEM_VERSION}"
        BUILD_BYPRODUCTS
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletDynamics/Debug/libBulletDynamics.a"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletCollision/Debug/libBulletCollision.a"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/LinearMath/Debug/libLinearMath.a"

                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletDynamics/Release/libBulletDynamics.a"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletCollision/Release/libBulletCollision.a"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/LinearMath/Release/libLinearMath.a"

                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletDynamics/libBulletDynamics.a"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletCollision/libBulletCollision.a"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/LinearMath/libLinearMath.a"

                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/lib/Debug/BulletDynamics_Debug.lib"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/lib/Debug/BulletCollision_Debug.lib"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/lib/Debug/LinearMath_Debug.lib"

                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/lib/Release/BulletDynamics.lib"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/lib/Release/BulletCollision.lib"
                "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/lib/Release/LinearMath.lib"
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

if(ANDROID)
        target_link_libraries(RayneBullet 
        "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletDynamics/libBulletDynamics.a"
        "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletCollision/libBulletCollision.a"
        "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/LinearMath/libLinearMath.a")
elseif(WIN32)
        target_link_libraries(RayneBullet 
        debug "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/lib/Debug/BulletDynamics_Debug.lib"
        debug "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/lib/Debug/BulletCollision_Debug.lib"
        debug "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/lib/Debug/LinearMath_Debug.lib"

        optimized "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/lib/Release/BulletDynamics.lib"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/lib/Release/BulletCollision.lib"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/lib/Release/LinearMath.lib")
else()
        target_link_libraries(RayneBullet 
        debug "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletDynamics/Debug/libBulletDynamics.a"
        debug "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletCollision/Debug/libBulletCollision.a"
        debug "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/LinearMath/Debug/libLinearMath.a"

        optimized "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletDynamics/Release/libBulletDynamics.a"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/BulletCollision/Release/libBulletCollision.a"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/bullet-prefix/src/bullet-build/src/LinearMath/Release/libLinearMath.a")
endif()

target_include_directories(RayneBullet SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneBullet PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneBullet/. DESTINATION lib/Rayne/RayneBullet)
