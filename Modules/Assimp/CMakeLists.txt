cmake_minimum_required(VERSION 2.8.4)
project(RayneAssimp)

include(../../CMake/LibraryTarget.cmake)
include(ExternalProject)

ExternalProject_Add(assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG refs/tags/v5.0.1
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    PATCH_COMMAND grep -qxF "target_compile_definitions(assimp PRIVATE -DNOUNCRYPT)" <SOURCE_DIR>/CMakeLists.txt || echo "target_compile_definitions(assimp PRIVATE -DNOUNCRYPT)" >> <SOURCE_DIR>/CMakeLists.txt
    CMAKE_CACHE_ARGS
        "-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH:BOOL=False"
        "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
        "-DBUILD_SHARED_LIBS:BOOL=False"
        "-DASSIMP_BUILD_TESTS:BOOL=False"
        "-DASSIMP_NO_EXPORT:BOOL=True"
        "-DASSIMP_BUILD_ASSIMP_TOOLS:BOOL=False"
        "-DANDROID_NATIVE_API_LEVEL:STRING=${ANDROID_NATIVE_API_LEVEL}"
        "-DANDROID_ABI:STRING=${ANDROID_ABI}"
        "-DANDROID_NDK:STRING=${ANDROID_NDK}"
        "-DANDROID_STL:STRING=${ANDROID_STL}"
    BUILD_BYPRODUCTS
        "${CMAKE_CURRENT_BINARY_DIR}/assimp-prefix/src/assimp-build/code/Debug/libassimpd.a"
        "${CMAKE_CURRENT_BINARY_DIR}/assimp-prefix/src/assimp-build/contrib/irrXML/Debug/libIrrXMLd.a"
        "${CMAKE_CURRENT_BINARY_DIR}/assimp-prefix/src/assimp-build/code/Release/libassimp.a"
        "${CMAKE_CURRENT_BINARY_DIR}/assimp-prefix/src/assimp-build/contrib/irrXML/Release/libIrrXML.a"
    INSTALL_COMMAND "")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNAssimpInit.cpp
        RNAssimpAssetLoader.cpp)

set(HEADERS
        RNAssimpAssetLoader.h
        RNAssimp.h)

set(DEFINES
        RN_BUILD_ASSIMP)

set(RAYNE_LIBRARIES Rayne)
set(INCLUDE_DIRECTORIES
        ${Rayne_BINARY_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/assimp-prefix/src/assimp/include
        ${CMAKE_CURRENT_BINARY_DIR}/assimp-prefix/src/assimp-build/include)

rayne_add_library(RayneAssimp "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneAssimp)

target_include_directories(RayneAssimp SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneAssimp PRIVATE "${DEFINES}")

add_dependencies(RayneAssimp assimp)

target_link_libraries(RayneAssimp "${LINK_LIBRARIES}"
        debug "${CMAKE_CURRENT_BINARY_DIR}/assimp-prefix/src/assimp-build/code/Debug/libassimpd.a"
        debug "${CMAKE_CURRENT_BINARY_DIR}/assimp-prefix/src/assimp-build/contrib/irrXML/Debug/libIrrXMLd.a"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/assimp-prefix/src/assimp-build/code/Release/libassimp.a"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/assimp-prefix/src/assimp-build/contrib/irrXML/Release/libIrrXML.a")


rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneAssimp/. DESTINATION lib/Rayne/RayneAssimp)
