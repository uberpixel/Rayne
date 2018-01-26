cmake_minimum_required(VERSION 2.8.4)
project(RayneRecast)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNRecastInit.cpp
        RNRecastWorld.cpp
        Vendor/recast/Recast/Source/Recast.cpp
        Vendor/recast/Recast/Source/RecastAlloc.cpp
        Vendor/recast/Recast/Source/RecastArea.cpp
        Vendor/recast/Recast/Source/RecastContour.cpp
        Vendor/recast/Recast/Source/RecastFilter.cpp
        Vendor/recast/Recast/Source/RecastLayers.cpp
        Vendor/recast/Recast/Source/RecastMesh.cpp
        Vendor/recast/Recast/Source/RecastMeshDetail.cpp
        Vendor/recast/Recast/Source/RecastRasterization.cpp
        Vendor/recast/Recast/Source/RecastRegion.cpp)

set(HEADERS
	RNRecast.h
        RNRecastWorld.h
        Vendor/recast/Recast/Include/Recast.h
        Vendor/recast/Recast/Include/RecastAlloc.h
        Vendor/recast/Recast/Include/RecastAssert.h)

set(DEFINES RN_BUILD_RECAST)

set(RAYNE_LIBRARIES Rayne)

set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/recast/Recast/Include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/recast/Detour/Include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/recast/DetourCrowd/Include")

rayne_add_library(RayneRecast "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneRecast)

target_include_directories(RayneRecast SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneRecast PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneRecast/. DESTINATION lib/Rayne/RayneRecast)
