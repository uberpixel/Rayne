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
        RNRecastMesh.cpp
        RNRecastAgent.cpp

        Vendor/recast/Recast/Source/Recast.cpp
        Vendor/recast/Recast/Source/RecastAlloc.cpp
        Vendor/recast/Recast/Source/RecastArea.cpp
        Vendor/recast/Recast/Source/RecastContour.cpp
        Vendor/recast/Recast/Source/RecastFilter.cpp
        Vendor/recast/Recast/Source/RecastLayers.cpp
        Vendor/recast/Recast/Source/RecastMesh.cpp
        Vendor/recast/Recast/Source/RecastMeshDetail.cpp
        Vendor/recast/Recast/Source/RecastRasterization.cpp
        Vendor/recast/Recast/Source/RecastRegion.cpp

        Vendor/recast/Detour/Source/DetourAlloc.cpp
        Vendor/recast/Detour/Source/DetourCommon.cpp
        Vendor/recast/Detour/Source/DetourNavMesh.cpp
        Vendor/recast/Detour/Source/DetourNavMeshBuilder.cpp
        Vendor/recast/Detour/Source/DetourNavMeshQuery.cpp
        Vendor/recast/Detour/Source/DetourNode.cpp

        Vendor/recast/DetourCrowd/Source/DetourCrowd.cpp
        Vendor/recast/DetourCrowd/Source/DetourLocalBoundary.cpp
        Vendor/recast/DetourCrowd/Source/DetourObstacleAvoidance.cpp
        Vendor/recast/DetourCrowd/Source/DetourPathCorridor.cpp
        Vendor/recast/DetourCrowd/Source/DetourPathQueue.cpp
        Vendor/recast/DetourCrowd/Source/DetourProximityGrid.cpp)

set(HEADERS
	RNRecast.h
        RNRecastWorld.h
        RNRecastMesh.h
        RNRecastAgent.h

        Vendor/recast/Recast/Include/Recast.h
        Vendor/recast/Recast/Include/RecastAlloc.h
        Vendor/recast/Recast/Include/RecastAssert.h

        Vendor/recast/Detour/Include/DetourAlloc.h
        Vendor/recast/Detour/Include/DetourAssert.h
        Vendor/recast/Detour/Include/DetourCommon.h
        Vendor/recast/Detour/Include/DetourMath.h
        Vendor/recast/Detour/Include/DetourNavMesh.h
        Vendor/recast/Detour/Include/DetourNavMeshBuilder.h
        Vendor/recast/Detour/Include/DetourNavMeshQuery.h
        Vendor/recast/Detour/Include/DetourNode.h
        Vendor/recast/Detour/Include/DetourStatus.h

        Vendor/recast/DetourCrowd/Include/DetourCrowd.h
        Vendor/recast/DetourCrowd/Include/DetourLocalBoundary.h
        Vendor/recast/DetourCrowd/Include/DetourObstacleAvoidance.h
        Vendor/recast/DetourCrowd/Include/DetourPathCorridor.h
        Vendor/recast/DetourCrowd/Include/DetourPathQueue.h
        Vendor/recast/DetourCrowd/Include/DetourProximityGrid.h)

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
