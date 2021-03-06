cmake_minimum_required(VERSION 3.10.1)
project(RayneNewton)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNNewtonInit.cpp
        RNNewtonWorld.cpp
        RNNewtonRigidBody.cpp
        RNNewtonInternals.cpp
        RNNewtonCollisionObject.cpp
#        RNNewtonMaterial.cpp
        RNNewtonShape.cpp
        RNNewtonConstraint.cpp
        RNNewtonCharacterController.cpp)

set(HEADERS
	RNNewton.h
        RNNewtonWorld.h
        RNNewtonRigidBody.h
        RNNewtonInternals.h
        RNNewtonCollisionObject.h
 #       RNNewtonMaterial.h
        RNNewtonShape.h
        RNNewtonConstraint.h
        RNNewtonCharacterController.h
        Vendor/newton-extensions/KCJoint.h)

set(DEFINES RN_BUILD_NEWTON _NEWTON_STATIC_LIB _CUSTOM_JOINTS_STATIC_LIB)

if(NOT APPLE)
        set(RAYNE_LIBRARIES Rayne)
else()
        set(RAYNE_LIBRARIES Rayne ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/build/lib/libNewton.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/build/lib/libdMath.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/build/lib/libdContainers.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/build/lib/libdCustomJoints.a)
endif()

set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/dgNewton"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/dMath"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/dContainers"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/dCustomJoints"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-extensions")

rayne_add_library(RayneNewton "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")

if(NOT APPLE)
        target_link_libraries(RayneNewton debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/projects/visualStudio_2015_static_md/x64/dgCore/Debug/dgCore_d.lib debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/projects/visualStudio_2015_static_md/x64/dgPhysics/Debug/dgPhysics_d.lib debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/projects/visualStudio_2015_static_md/x64/newton/Debug/newton_d.lib debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/projects/visualStudio_2015_static_md/x64/dMath/Debug/dMath_d.lib debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/projects/visualStudio_2015_static_md/x64/dContainers/Debug/dContainers_d.lib debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/projects/visualStudio_2015_static_md/x64/dCustomJoints/Debug/dCustomJoints_d.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/projects/visualStudio_2015_static_md/x64/dgCore/Release/dgCore.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/projects/visualStudio_2015_static_md/x64/dgPhysics/Release/dgPhysics.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/projects/visualStudio_2015_static_md/x64/newton/Release/newton.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/projects/visualStudio_2015_static_md/x64/dMath/Release/dMath.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/projects/visualStudio_2015_static_md/x64/dContainers/Release/dContainers.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/newton-dynamics/sdk/projects/visualStudio_2015_static_md/x64/dCustomJoints/Release/dCustomJoints.lib)
endif()

rayne_set_module_output_directory(RayneNewton)

target_include_directories(RayneNewton SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneNewton PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneNewton/. DESTINATION lib/Rayne/RayneNewton)
