cmake_minimum_required(VERSION 3.10)
project(RaynePhysX)

include(ExternalProject)
include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)


set(PHYSX_CMAKE_ARGS
        -DPX_BUILDSNIPPETS=False
        -DPX_BUILDPUBLICSAMPLES=False
        -DPX_CMAKE_SUPPRESS_REGENERATION=True
        -DPX_GENERATE_STATIC_LIBRARIES=True
        -DPHYSX_ROOT_DIR=${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx/physx
        -DPX_OUTPUT_LIB_DIR=${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx
        -DPX_OUTPUT_BIN_DIR=${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx
        -DPXSHARED_PATH=${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx/pxshared
        -DCMAKEMODULES_PATH=${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx/externals/cmakemodules
        -DCMAKEMODULES_NAME=CMakeModules
        -DCMAKEMODULES_VERSION=1.27)

set(PHYSX_PATCH_COMMAND "")

if(APPLE)
    set(PHYSX_X64_CMAKE_ARGS ${PHYSX_CMAKE_ARGS}
        -DTARGET_BUILD_PLATFORM=mac
        -DPX_OUTPUT_ARCH=x64
        -DCMAKE_OSX_ARCHITECTURES=x86_64
        -DCMAKE_CXX_FLAGS=-Wno-atomic-implicit-seq-cst
        -DPX_OUTPUT_LIB_DIR=${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/x64
        -DPX_OUTPUT_BIN_DIR=${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/x64)

    set(PHYSX_ARM64_CMAKE_ARGS ${PHYSX_CMAKE_ARGS}
        -DTARGET_BUILD_PLATFORM=mac
        -DPX_OUTPUT_ARCH=x64
        -DCMAKE_OSX_ARCHITECTURES=arm64
        -DCMAKE_CXX_FLAGS=-Wno-atomic-implicit-seq-cst
        -DPX_OUTPUT_LIB_DIR=${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/arm64
        -DPX_OUTPUT_BIN_DIR=${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/arm64)

    set(PHYSX_CMAKE_ARGS ${PHYSX_CMAKE_ARGS}
        -DTARGET_BUILD_PLATFORM=mac
        -DPX_OUTPUT_ARCH=x64
        -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=YES
        -DCMAKE_CXX_FLAGS=-Wno-atomic-implicit-seq-cst
        -DPX_OUTPUT_LIB_DIR=${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build
        -DPX_OUTPUT_BIN_DIR=${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build)

    set (RN_PHYSX_BUILD_BYPRODUCTS
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/debug/libPhysXFoundation_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/debug/libPhysXCommon_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/debug/libPhysX_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/debug/libPhysXCooking_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/debug/libPhysXCharacterKinematic_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/debug/libPhysXVehicle_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/debug/libPhysXExtensions_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/debug/libPhysXPvdSDK_static_64.a"

        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/release/libPhysXFoundation_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/release/libPhysXCommon_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/release/libPhysX_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/release/libPhysXCooking_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/release/libPhysXCharacterKinematic_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/release/libPhysXVehicle_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/release/libPhysXExtensions_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/release/libPhysXPvdSDK_static_64.a"

        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/debug/libPhysXFoundation_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/debug/libPhysXCommon_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/debug/libPhysX_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/debug/libPhysXCooking_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/debug/libPhysXCharacterKinematic_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/debug/libPhysXVehicle_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/debug/libPhysXExtensions_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/debug/libPhysXPvdSDK_static_64.a"

        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/release/libPhysXFoundation_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/release/libPhysXCommon_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/release/libPhysX_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/release/libPhysXCooking_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/release/libPhysXCharacterKinematic_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/release/libPhysXVehicle_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/release/libPhysXExtensions_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64/release/libPhysXPvdSDK_static_64.a")

    set(RN_PHYSX_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/bin/mac.x86_64)
    set(PHYSX_PATCH_COMMAND sed -i.bak /CMAKE_OSX_ARCHITECTURES/d <SOURCE_DIR>/physx/source/compiler/cmake/mac/CMakeLists.txt
        COMMAND sed -i.bak /arch/d <SOURCE_DIR>/physx/source/compiler/cmake/mac/CMakeLists.txt
        COMMAND sed -i.bak s/-Werror//g <SOURCE_DIR>/physx/source/compiler/cmake/mac/CMakeLists.txt)
elseif(ANDROID)
    set(PHYSX_PATCH_COMMAND sed -i.bak s/-Werror//g <SOURCE_DIR>/physx/source/compiler/cmake/android/CMakeLists.txt)

    set(PHYSX_CXX_FLAGS "-Wno-unknown-warning-option -Wno-invalid-noreturn -Wno-unused-private-field -Wno-unused-local-typedef -D__ANDROID__")
    set(PHYSX_CMAKE_ARGS ${PHYSX_CMAKE_ARGS}
        -DTARGET_BUILD_PLATFORM=android
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DANDROID_NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL}
        -DPX_OUTPUT_ARCH=arm
        -DANDROID_ABI=arm64-v8a
        -DANDROID_NDK=${ANDROID_NDK}
        -DANDROID_STL=${ANDROID_STL}
        -DCMAKE_CXX_FLAGS=${PHYSX_CXX_FLAGS}
        -DCMAKE_BUILD_TYPE=release)

    set(RN_PHYSX_BUILD_BYPRODUCTS
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXCharacterKinematic_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXExtensions_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysX_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXPvdSDK_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXVehicle_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXCooking_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXCommon_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXFoundation_static_64.a")
elseif(UNIX)
    set(PHYSX_CMAKE_ARGS ${PHYSX_CMAKE_ARGS}
        -DTARGET_BUILD_PLATFORM=linux
        -DPX_OUTPUT_ARCH=x86
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_BUILD_TYPE=release)

    set(RN_PHYSX_BUILD_BYPRODUCTS
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXFoundation_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXCommon_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysX_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXCooking_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXCharacterKinematic_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXVehicle_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXExtensions_static_64.a"
        "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXPvdSDK_static_64.a")
else()
    set(PHYSX_CMAKE_ARGS ${PHYSX_CMAKE_ARGS}
        -DTARGET_BUILD_PLATFORM=windows
        -DPX_OUTPUT_ARCH=x86
        -DPHYSX_CXX_FLAGS_DEBUG=/MDd)
    set(RN_PHYSX_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/win.x86_64.vc${MSVC_TOOLSET_VERSION}.md)

    set(RN_PHYSX_BUILD_BYPRODUCTS
        "${RN_PHYSX_BUILD_DIR}/debug/PhysXFoundation_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/debug/PhysXCommon_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/debug/PhysX_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/debug/PhysXCooking_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/debug/PhysXCharacterKinematic_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/debug/PhysXVehicle_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/debug/PhysXExtensions_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/debug/PhysXPvdSDK_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/release/PhysXFoundation_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/release/PhysXCommon_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/release/PhysX_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/release/PhysXCooking_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/release/PhysXCharacterKinematic_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/release/PhysXVehicle_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/release/PhysXExtensions_static_64.lib"
        "${RN_PHYSX_BUILD_DIR}/release/PhysXPvdSDK_static_64.lib")
endif()

if(APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    ExternalProject_Add(physx
        GIT_REPOSITORY https://github.com/NVIDIAGameWorks/PhysX.git
        GIT_TAG origin/4.1
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        PATCH_COMMAND ${PHYSX_PATCH_COMMAND}
        SOURCE_SUBDIR physx/compiler/public
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release "${PHYSX_X64_CMAKE_ARGS}" -B<BINARY_DIR>/x64 -S<SOURCE_DIR>/<SOURCE_SUBDIR>
        COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release "${PHYSX_ARM64_CMAKE_ARGS}" -B<BINARY_DIR>/arm64 -S<SOURCE_DIR>/<SOURCE_SUBDIR>
        BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR>/x64
        COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR>/arm64
        COMMAND mkdir release
        COMMAND lipo -create -output release/libPhysXFoundation_static_64.a x64/bin/mac.x86_64/release/libPhysXFoundation_static_64.a arm64/bin/mac.x86_64/release/libPhysXFoundation_static_64.a
        COMMAND lipo -create -output release/libPhysXCommon_static_64.a x64/bin/mac.x86_64/release/libPhysXCommon_static_64.a arm64/bin/mac.x86_64/release/libPhysXCommon_static_64.a
        COMMAND lipo -create -output release/libPhysX_static_64.a x64/bin/mac.x86_64/release/libPhysX_static_64.a arm64/bin/mac.x86_64/release/libPhysX_static_64.a
        COMMAND lipo -create -output release/libPhysXCooking_static_64.a x64/bin/mac.x86_64/release/libPhysXCooking_static_64.a arm64/bin/mac.x86_64/release/libPhysXCooking_static_64.a
        COMMAND lipo -create -output release/libPhysXCharacterKinematic_static_64.a x64/bin/mac.x86_64/release/libPhysXCharacterKinematic_static_64.a arm64/bin/mac.x86_64/release/libPhysXCharacterKinematic_static_64.a
        COMMAND lipo -create -output release/libPhysXVehicle_static_64.a x64/bin/mac.x86_64/release/libPhysXVehicle_static_64.a arm64/bin/mac.x86_64/release/libPhysXVehicle_static_64.a
        COMMAND lipo -create -output release/libPhysXExtensions_static_64.a x64/bin/mac.x86_64/release/libPhysXExtensions_static_64.a arm64/bin/mac.x86_64/release/libPhysXExtensions_static_64.a
        COMMAND lipo -create -output release/libPhysXPvdSDK_static_64.a x64/bin/mac.x86_64/release/libPhysXPvdSDK_static_64.a arm64/bin/mac.x86_64/release/libPhysXPvdSDK_static_64.a

        UPDATE_COMMAND ""
        BUILD_BYPRODUCTS ${RN_PHYSX_BUILD_BYPRODUCTS}
        INSTALL_COMMAND "")

    set(RN_PHYSX_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build)
else()
    ExternalProject_Add(physx
        GIT_REPOSITORY https://github.com/NVIDIAGameWorks/PhysX.git
        GIT_TAG origin/4.1
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        PATCH_COMMAND ${PHYSX_PATCH_COMMAND}
        SOURCE_SUBDIR physx/compiler/public
        CMAKE_ARGS "${PHYSX_CMAKE_ARGS}"
        UPDATE_COMMAND ""
        BUILD_BYPRODUCTS ${RN_PHYSX_BUILD_BYPRODUCTS}
        INSTALL_COMMAND "")
endif()

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

set(DEFINES RN_BUILD_PHYSX PX_PHYSX_STATIC_LIB)

if(APPLE)
    set(RAYNE_LIBRARIES Rayne)
elseif(ANDROID)
    set(RAYNE_LIBRARIES Rayne ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXCharacterKinematic_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXExtensions_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysX_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXPvdSDK_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXVehicle_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXCooking_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXCommon_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/android.arm64-v8a.fp-soft/release/libPhysXFoundation_static_64.a)
elseif(UNIX)
    set(RAYNE_LIBRARIES Rayne ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXFoundation_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXCommon_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysX_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXCooking_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXCharacterKinematic_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXVehicle_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXExtensions_static_64.a ${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx-build/physx/bin/linux.clang/release/libPhysXPvdSDK_static_64.a)
else()
    set(RAYNE_LIBRARIES Rayne)
endif()


set(INCLUDE_DIRECTORIES
    "${Rayne_BINARY_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx/pxshared/include"
    "${CMAKE_CURRENT_BINARY_DIR}/physx-prefix/src/physx/physx/include")

rayne_add_library(RaynePhysX "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")

if(APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    target_link_libraries(RaynePhysX ${RN_PHYSX_BUILD_DIR}/release/libPhysXFoundation_static_64.a ${RN_PHYSX_BUILD_DIR}/release/libPhysXCommon_static_64.a ${RN_PHYSX_BUILD_DIR}/release/libPhysX_static_64.a ${RN_PHYSX_BUILD_DIR}/release/libPhysXCooking_static_64.a ${RN_PHYSX_BUILD_DIR}/release/libPhysXCharacterKinematic_static_64.a ${RN_PHYSX_BUILD_DIR}/release/libPhysXVehicle_static_64.a ${RN_PHYSX_BUILD_DIR}/release/libPhysXExtensions_static_64.a ${RN_PHYSX_BUILD_DIR}/release/libPhysXPvdSDK_static_64.a)
elseif(APPLE)
    target_link_libraries(RaynePhysX debug ${RN_PHYSX_BUILD_DIR}/debug/libPhysXFoundation_static_64.a debug ${RN_PHYSX_BUILD_DIR}/debug/libPhysXCommon_static_64.a debug ${RN_PHYSX_BUILD_DIR}/debug/libPhysX_static_64.a debug ${RN_PHYSX_BUILD_DIR}/debug/libPhysXCooking_static_64.a debug ${RN_PHYSX_BUILD_DIR}/debug/libPhysXCharacterKinematic_static_64.a debug ${RN_PHYSX_BUILD_DIR}/debug/libPhysXVehicle_static_64.a debug ${RN_PHYSX_BUILD_DIR}/debug/libPhysXExtensions_static_64.a debug ${RN_PHYSX_BUILD_DIR}/debug/libPhysXPvdSDK_static_64.a

    optimized ${RN_PHYSX_BUILD_DIR}/release/libPhysXFoundation_static_64.a optimized ${RN_PHYSX_BUILD_DIR}/release/libPhysXCommon_static_64.a optimized ${RN_PHYSX_BUILD_DIR}/release/libPhysX_static_64.a optimized ${RN_PHYSX_BUILD_DIR}/release/libPhysXCooking_static_64.a optimized ${RN_PHYSX_BUILD_DIR}/release/libPhysXCharacterKinematic_static_64.a optimized ${RN_PHYSX_BUILD_DIR}/release/libPhysXVehicle_static_64.a optimized ${RN_PHYSX_BUILD_DIR}/release/libPhysXExtensions_static_64.a optimized ${RN_PHYSX_BUILD_DIR}/release/libPhysXPvdSDK_static_64.a)
elseif(WIN32)
    target_link_libraries(RaynePhysX debug ${RN_PHYSX_BUILD_DIR}/debug/PhysXFoundation_static_64.lib debug ${RN_PHYSX_BUILD_DIR}/debug/PhysXCommon_static_64.lib debug ${RN_PHYSX_BUILD_DIR}/debug/PhysX_static_64.lib debug ${RN_PHYSX_BUILD_DIR}/debug/PhysXCooking_static_64.lib debug ${RN_PHYSX_BUILD_DIR}/debug/PhysXCharacterKinematic_static_64.lib debug ${RN_PHYSX_BUILD_DIR}/debug/PhysXVehicle_static_64.lib debug ${RN_PHYSX_BUILD_DIR}/debug/PhysXExtensions_static_64.lib debug ${RN_PHYSX_BUILD_DIR}/debug/PhysXPvdSDK_static_64.lib

    optimized ${RN_PHYSX_BUILD_DIR}/release/PhysXFoundation_static_64.lib optimized ${RN_PHYSX_BUILD_DIR}/release/PhysXCommon_static_64.lib optimized ${RN_PHYSX_BUILD_DIR}/release/PhysX_static_64.lib optimized ${RN_PHYSX_BUILD_DIR}/release/PhysXCooking_static_64.lib optimized ${RN_PHYSX_BUILD_DIR}/release/PhysXCharacterKinematic_static_64.lib optimized ${RN_PHYSX_BUILD_DIR}/release/PhysXVehicle_static_64.lib optimized ${RN_PHYSX_BUILD_DIR}/release/PhysXExtensions_static_64.lib optimized ${RN_PHYSX_BUILD_DIR}/release/PhysXPvdSDK_static_64.lib)
endif()
add_dependencies(RaynePhysX physx)

rayne_set_module_output_directory(RaynePhysX)

target_include_directories(RaynePhysX SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RaynePhysX PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RaynePhysX/. DESTINATION lib/Rayne/RaynePhysX)
