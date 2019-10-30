cmake_minimum_required(VERSION 2.8.4)
project(RaynePhysX)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNPhysXInit.cpp
        RNPhysXWorld.cpp
        RNPhysXDynamicBody.cpp
        RNPhysXStaticBody.cpp
        RNPhysXInternals.cpp
        RNPhysXCollisionObject.cpp
        RNPhysXMaterial.cpp
        RNPhysXShape.cpp
        RNPhysXConstraint.cpp
        RNPhysXKinematicController.cpp)

set(HEADERS
	RNPhysX.h
        RNPhysXWorld.h
        RNPhysXDynamicBody.h
        RNPhysXStaticBody.h
        RNPhysXInternals.h
        RNPhysXCollisionObject.h
        RNPhysXMaterial.h
        RNPhysXShape.h
        RNPhysXConstraint.h
        RNPhysXKinematicController.h)

set(DEFINES RN_BUILD_PHYSX)

if(APPLE)
        set(RAYNE_LIBRARIES Rayne ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PxShared/lib/osx64/libPxFoundation.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PxShared/lib/osx64/libPxTask.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PxShared/lib/osx64/libPxPvdSDK.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/osx64/libSceneQuery.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/osx64/libSimulationController.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/osx64/libPhysX3Common.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/osx64/libLowLevel.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/osx64/libLowLevelAABB.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/osx64/libLowLevelDynamics.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/osx64/libLowLevelCloth.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/osx64/libLowLevelParticles.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/osx64/libPhysX3.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/osx64/libPhysX3CharacterKinematic.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/osx64/libPhysX3Cooking.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/osx64/libPhysX3Extensions.a)
elseif(ANDROID)
        set(RAYNE_LIBRARIES Rayne ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/Android/obj/local/arm64-v8a/libPhysX.a)
elseif(UNIX)
        set(RAYNE_LIBRARIES Rayne ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX/physx/bin/linux.clang/release/libPhysXFoundation_static_64.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX/physx/bin/linux.clang/release/libPhysXCommon_static_64.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX/physx/bin/linux.clang/release/libPhysX_static_64.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX/physx/bin/linux.clang/release/libPhysXCooking_static_64.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX/physx/bin/linux.clang/release/libPhysXCharacterKinematic_static_64.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX/physx/bin/linux.clang/release/libPhysXVehicle_static_64.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX/physx/bin/linux.clang/release/libPhysXExtensions_static_64.a ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX/physx/bin/linux.clang/release/libPhysXPvdSDK_static_64.a)
else()
        set(RAYNE_LIBRARIES Rayne)
endif()

if(UNIX AND NOT APPLE)
    set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX/pxshared/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX/physx/include")
else()
    set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PxShared/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Include")
endif()

rayne_add_library(RaynePhysX "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")

if(WIN32)
    target_link_libraries(RaynePhysX debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PxShared/lib/vc14win64/PxFoundationDEBUG_x64.lib debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PxShared/lib/vc14win64/PxPvdSDKDEBUG_x64.lib debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3DEBUG_x64.lib debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3CommonDEBUG_x64.lib debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3CharacterKinematicDEBUG_x64.lib debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3CookingDEBUG_x64.lib debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3ExtensionsDEBUG.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PxShared/lib/vc14win64/PxFoundation_x64.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PxShared/lib/vc14win64/PxPvdSDK_x64.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3_x64.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3Common_x64.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3CharacterKinematic_x64.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3Cooking_x64.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3Extensions.lib)
endif()

rayne_set_module_output_directory(RaynePhysX)

target_include_directories(RaynePhysX SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RaynePhysX PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RaynePhysX/. DESTINATION lib/Rayne/RaynePhysX)
