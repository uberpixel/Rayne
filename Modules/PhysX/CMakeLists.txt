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
        RNPhysXKinematicController.h)

set(DEFINES RN_BUILD_PHYSX)

if(NOT APPLE)
        set(RAYNE_LIBRARIES Rayne ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PxShared/lib/vc14win64/PxFoundationDEBUG_x64.lib ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PxShared/lib/vc14win64/PxPvdSDKDEBUG_x64.lib ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3DEBUG_x64.lib ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3CommonDEBUG_x64.lib ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3CharacterKinematicDEBUG_x64.lib ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3CookingDEBUG_x64.lib ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3ExtensionsDEBUG.lib)

 #       set(RAYNE_LIBRARIES Rayne "DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3DEBUG_x64.lib ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3CommonDEBUG_x64.lib ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3CharacterKinematicDEBUG_x64.lib ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PxShared/lib/vc14win64/PxFoundationDEBUG_x64.lib OPTIMIZED ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3_x64.lib ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3Common_x64.lib ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Lib/vc14win64/PhysX3CharacterKinematic_x64.lib ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PxShared/lib/vc14win64/PxFoundation_x64.lib")
else()
        set(RAYNE_LIBRARIES Rayne debug ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/opus/win32/VS2015/x64/Debug/opus.lib optimized ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/opus/win32/VS2015/x64/Release/opus.lib)
endif()

set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PxShared/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/PhysX-3.4/PhysX_3.4/Include")

rayne_add_library(RaynePhysX "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RaynePhysX)

target_include_directories(RaynePhysX SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RaynePhysX PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RaynePhysX/. DESTINATION lib/Rayne/RaynePhysX)
