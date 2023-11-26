cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0097 NEW)
project(RayneJolt)

include(ExternalProject)
include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

ExternalProject_Add(jolt
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG refs/tags/v4.0.1
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    SOURCE_SUBDIR "Build"
    CMAKE_CACHE_ARGS
        "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
        "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
        "-DANDROID_NATIVE_API_LEVEL:STRING=${ANDROID_NATIVE_API_LEVEL}"
        "-DANDROID_ABI:STRING=${ANDROID_ABI}"
        "-DANDROID_NDK:STRING=${ANDROID_NDK}"
        "-DANDROID_STL:STRING=${ANDROID_STL}"
        "-DCMAKE_SYSTEM_VERSION:STRING=${CMAKE_SYSTEM_VERSION}"
    BUILD_BYPRODUCTS
        "${CMAKE_CURRENT_BINARY_DIR}/jolt-prefix/src/jolt-build/Debug/libJolt.a"
        "${CMAKE_CURRENT_BINARY_DIR}/jolt-prefix/src/jolt-build/Release/libJolt.a"

        "${CMAKE_CURRENT_BINARY_DIR}/jolt-prefix/src/jolt-build/libJolt.a"

        "${CMAKE_CURRENT_BINARY_DIR}/jolt-prefix/src/jolt-build/Debug/Jolt_Debug.lib"
        "${CMAKE_CURRENT_BINARY_DIR}/jolt-prefix/src/jolt-build/Release/Jolt.lib"
    INSTALL_COMMAND "")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNJoltInit.cpp
        RNJoltWorld.cpp
        RNJoltDynamicBody.cpp
        RNJoltStaticBody.cpp
        RNJoltInternals.cpp
        RNJoltCollisionObject.cpp
        RNJoltMaterial.cpp
        RNJoltShape.cpp
        RNJoltConstraint.cpp
        RNJoltKinematicController.cpp)

set(HEADERS
	RNJolt.h
        RNJoltWorld.h
        RNJoltDynamicBody.h
        RNJoltStaticBody.h
        RNJoltInternals.h
        RNJoltCollisionObject.h
        RNJoltMaterial.h
        RNJoltShape.h
        RNJoltConstraint.h
        RNJoltKinematicController.h)

set(DEFINES RN_BUILD_JOLT)

set(RAYNE_LIBRARIES Rayne)

set(INCLUDE_DIRECTORIES
    "${Rayne_BINARY_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/Jolt-prefix/src/Jolt")

rayne_add_library(RayneJolt "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")

if(ANDROID)
    target_link_libraries(RayneJolt PUBLIC
        "${CMAKE_CURRENT_BINARY_DIR}/jolt-prefix/src/jolt-build/libJolt.a")
elseif(WIN32)
    target_link_libraries(RayneJolt PUBLIC
        debug "${CMAKE_CURRENT_BINARY_DIR}/jolt-prefix/src/jolt-build/Debug/Jolt_Debug.lib"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/jolt-prefix/src/jolt-build/Release/Jolt.lib")
else()
    target_link_libraries(RayneJolt PUBLIC
        debug "${CMAKE_CURRENT_BINARY_DIR}/jolt-prefix/src/jolt-build/Debug/libJolt.a"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/jolt-prefix/src/jolt-build/Release/libJolt.a")
endif()

target_compile_definitions(RayneJolt PUBLIC "$<$<CONFIG:Debug>:_DEBUG;JPH_PROFILE_ENABLED;JPH_DEBUG_RENDERER>")
target_compile_definitions(RayneJolt PUBLIC "$<$<CONFIG:Release>:NDEBUG;JPH_PROFILE_ENABLED;JPH_DEBUG_RENDERER>")
#target_compile_definitions(RayneJolt PUBLIC "$<$<CONFIG:Distribution>:NDEBUG>")
#target_compile_definitions(RayneJolt PUBLIC "$<$<CONFIG:ReleaseASAN>:NDEBUG;JPH_PROFILE_ENABLED;JPH_DISABLE_TEMP_ALLOCATOR;JPH_DISABLE_CUSTOM_ALLOCATOR;JPH_DEBUG_RENDERER>")
#target_compile_definitions(RayneJolt PUBLIC "$<$<CONFIG:ReleaseUBSAN>:NDEBUG;JPH_PROFILE_ENABLED;JPH_DEBUG_RENDERER>")
#target_compile_definitions(RayneJolt PUBLIC "$<$<CONFIG:ReleaseCoverage>:NDEBUG>")

add_dependencies(RayneJolt jolt)

rayne_set_module_output_directory(RayneJolt)

target_include_directories(RayneJolt SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneJolt PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneJolt/. DESTINATION lib/Rayne/RayneJolt)
