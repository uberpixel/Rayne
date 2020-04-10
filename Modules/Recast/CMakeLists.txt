cmake_minimum_required(VERSION 2.8.4)
project(RayneRecast)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

ExternalProject_Add(recast
        GIT_REPOSITORY https://github.com/recastnavigation/recastnavigation.git
        GIT_TAG origin/master
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        BUILD_BYPRODUCTS
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/Recast.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastAlloc.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastArea.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastAssert.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastContour.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastFilter.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastLayers.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastMesh.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastMeshDetail.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastRasterization.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastRegion.cpp

                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourAlloc.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourAssert.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourCommon.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourNavMesh.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourNavMeshBuilder.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourNavMeshQuery.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourNode.cpp

                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Source/DetourCrowd.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Source/DetourLocalBoundary.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Source/DetourObstacleAvoidance.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Source/DetourPathCorridor.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Source/DetourPathQueue.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Source/DetourProximityGrid.cpp

                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Include/Recast.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Include/RecastAlloc.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Include/RecastAssert.h

                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourAlloc.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourAssert.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourCommon.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourMath.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourNavMesh.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourNavMeshBuilder.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourNavMeshQuery.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourNode.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourStatus.h

                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Include/DetourCrowd.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Include/DetourLocalBoundary.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Include/DetourObstacleAvoidance.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Include/DetourPathCorridor.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Include/DetourPathQueue.h
                ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Include/DetourProximityGrid.h
        INSTALL_COMMAND "")


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

        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/Recast.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastAlloc.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastArea.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastAssert.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastContour.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastFilter.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastLayers.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastMesh.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastMeshDetail.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastRasterization.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Source/RecastRegion.cpp

        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourAlloc.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourAssert.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourCommon.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourNavMesh.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourNavMeshBuilder.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourNavMeshQuery.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Source/DetourNode.cpp

        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Source/DetourCrowd.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Source/DetourLocalBoundary.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Source/DetourObstacleAvoidance.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Source/DetourPathCorridor.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Source/DetourPathQueue.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Source/DetourProximityGrid.cpp)

set(HEADERS
	RNRecast.h
        RNRecastWorld.h
        RNRecastMesh.h
        RNRecastAgent.h

#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Include/Recast.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Include/RecastAlloc.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Include/RecastAssert.h

#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourAlloc.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourAssert.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourCommon.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourMath.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourNavMesh.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourNavMeshBuilder.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourNavMeshQuery.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourNode.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include/DetourStatus.h

#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Include/DetourCrowd.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Include/DetourLocalBoundary.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Include/DetourObstacleAvoidance.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Include/DetourPathCorridor.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Include/DetourPathQueue.h
#        ${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Include/DetourProximityGrid.h
)

set(DEFINES RN_BUILD_RECAST)

set(RAYNE_LIBRARIES Rayne)

set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Recast/Include"
        "${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/Detour/Include"
        "${CMAKE_CURRENT_BINARY_DIR}/recast-prefix/src/recast/DetourCrowd/Include")

rayne_add_library(RayneRecast "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneRecast)

target_include_directories(RayneRecast SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneRecast PRIVATE "${DEFINES}")

add_dependencies(RayneRecast_copyHeaderTarget recast)

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneRecast/. DESTINATION lib/Rayne/RayneRecast)
