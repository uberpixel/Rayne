cmake_minimum_required(VERSION 3.10.1)
project(RayneGlTF)

include(../../CMake/LibraryTarget.cmake)
include(ExternalProject)

ExternalProject_Add(tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CMAKE_CACHE_ARGS
        "-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH:BOOL=False"
        "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
        "-DBUILD_SHARED_LIBS:BOOL=False"
        "-DANDROID_NATIVE_API_LEVEL:STRING=${ANDROID_NATIVE_API_LEVEL}"
        "-DANDROID_ABI:STRING=${ANDROID_ABI}"
        "-DANDROID_NDK:STRING=${ANDROID_NDK}"
        "-DANDROID_STL:STRING=${ANDROID_STL}"
    BUILD_BYPRODUCTS
		"${CMAKE_CURRENT_BINARY_DIR}/tinygltf-prefix/src/tinygltf-build/Debug/libtinygltf.a"
		"${CMAKE_CURRENT_BINARY_DIR}/tinygltf-prefix/src/tinygltf-build/Release/libtinygltf.a"
    INSTALL_COMMAND "")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNglTFInit.cpp
        RNglTFAssetLoader.cpp)

set(HEADERS
        RNglTFAssetLoader.h
        RNglTF.h)

set(DEFINES
        RN_BUILD_GLTF)

set(RAYNE_LIBRARIES Rayne)
set(INCLUDE_DIRECTORIES
        ${Rayne_BINARY_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/tinygltf-prefix/src/tinygltf)

rayne_add_library(RayneGlTF "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneGlTF)

target_include_directories(RayneGlTF SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneGlTF PRIVATE "${DEFINES}")

add_dependencies(RayneGlTF tinygltf)

target_link_libraries(RayneGlTF "${LINK_LIBRARIES}"
        debug "${CMAKE_CURRENT_BINARY_DIR}/tinygltf-prefix/src/tinygltf-build/Debug/libtinygltf.a"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/tinygltf-prefix/src/tinygltf-build/Release/libtinygltf.a")


rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneGlTF/. DESTINATION lib/Rayne/RayneGlTF)
