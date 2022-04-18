cmake_minimum_required(VERSION 3.10.1)
project(RayneBHaptics)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNBHapticsInit.cpp
        RNBHapticsTypes.cpp
        RNBHapticsManager.cpp
        RNBHapticsAndroidWrapper.cpp)

set(HEADERS
	RNBHaptics.h
        RNBHapticsTypes.h
        RNBHapticsManager.h
        RNBHapticsAndroidWrapper.h)

set(DEFINES RN_BUILD_BHAPTICS)

set(COMPILE_OPTIONS "")
set(RAYNE_LIBRARIES Rayne)

set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics/include/shared")

rayne_add_library(RayneBHaptics "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneBHaptics)

target_include_directories(RayneBHaptics SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneBHaptics PRIVATE "${DEFINES}")
target_compile_options(RayneBHaptics PRIVATE ${COMPILE_OPTIONS})


ExternalProject_Add(bhaptics
        GIT_REPOSITORY https://github.com/bhaptics/haptic-library.git
        GIT_TAG refs/tags/tac-1.6.5
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists_haptics.txt ${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics/CMakeLists.txt
        CMAKE_CACHE_ARGS
                "-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH:BOOL=False"
                "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
                "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
                "-DANDROID_NATIVE_API_LEVEL:STRING=${ANDROID_NATIVE_API_LEVEL}"
                "-DANDROID_ABI:STRING=${ANDROID_ABI}"
                "-DANDROID_NDK:STRING=${ANDROID_NDK}"
                "-DANDROID_STL:STRING=${ANDROID_STL}"
        BUILD_BYPRODUCTS
                ${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics-build/Debug/libhaptic_library.a
                ${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics-build/Release/libhaptic_library.a
                ${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics-build/libhaptic_library.a
                ${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics-build/Debug/libhaptic_library.lib
                ${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics-build/Release/libhaptic_library.lib
        INSTALL_COMMAND "")

add_library(BHapticsLib STATIC IMPORTED)
if(WIN32)
        SET_PROPERTY(TARGET BHapticsLib PROPERTY IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo")

        SET_PROPERTY(TARGET BHapticsLib PROPERTY IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics-build/Debug/libhaptic_library.lib")
        SET_PROPERTY(TARGET BHapticsLib PROPERTY IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics-build/Release/libhaptic_library.lib")
        SET_PROPERTY(TARGET BHapticsLib PROPERTY IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics-build/RelWithDebInfo/libhaptic_library.lib")

elseif(ANDROID)
        SET_PROPERTY(TARGET BHapticsLib PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics-build/libhaptic_library.a)
else()
        SET_PROPERTY(TARGET BHapticsLib PROPERTY IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo")

        SET_PROPERTY(TARGET BHapticsLib PROPERTY IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics-build/Debug/libhaptic_library.a")
        SET_PROPERTY(TARGET BHapticsLib PROPERTY IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics-build/Release/libhaptic_library.a")
        SET_PROPERTY(TARGET BHapticsLib PROPERTY IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics-build/RelWithDebInfo/libhaptic_library.a")
endif()

add_dependencies(BHapticsLib bhaptics)
target_link_libraries(RayneBHaptics PUBLIC BHapticsLib)

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneBHaptics/. DESTINATION lib/Rayne/RayneBHaptics)
