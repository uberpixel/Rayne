cmake_minimum_required(VERSION 2.8.4)
project(Rayne)

include(../CMake/CompilerCheck.cmake)
include(../CMake/BoostInclude.cmake)
include(../CMake/TargetDirectories.cmake)
include(../CMake/LibraryTarget.cmake)

target_directories(RAYNE)

set(VERSION_MAJOR 2)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(VERSION_ABI 10)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

# Source files
set(RAYNE_SOURCES
    Base/RNMemory.cpp # RNMemory should come first
    Base/RNApplication.cpp
    Base/RNArgumentParser.cpp
    Base/RNBase.cpp
    Base/RNException.cpp
    Base/RNKernel.cpp
    Base/RNMemoryPool.cpp
    Base/RNSettings.cpp
    Base/RNUnicode.cpp
    Assets/RNAsset.cpp
    Assets/RNAssetCoordinator.cpp
    Assets/RNAssetLoader.cpp
    Assets/RNPNGAssetLoader.cpp
    Debug/RNLogger.cpp
    Debug/RNLoggingEngine.cpp
    Math/RNMath.cpp
    Math/RNRandom.cpp
    Modules/RNModule.cpp
    Modules/RNModuleCoordinator.cpp
    Objects/RNCatalogue.cpp
    Objects/RNObject.cpp
    Objects/RNObjectInternals.cpp
    Objects/RNKVO.cpp
    Objects/RNAutoreleasePool.cpp
    Objects/RNSerialization.cpp
    Objects/RNData.cpp
    Objects/RNNull.cpp
    Objects/RNNumber.cpp
    Objects/RNCharacterSet.cpp
    Objects/RNString.cpp
    Objects/RNStringInternal.cpp
    Objects/RNValue.cpp
    Objects/RNArray.cpp
    Objects/RNDictionary.cpp
    Objects/RNJSONSerialization.cpp
    Objects/RNSet.cpp
    Objects/RNCountedSet.cpp
    Objects/RNWeakStorage.cpp
    Rendering/RNFramebuffer.cpp
    Rendering/RNGPUBuffer.cpp
    Rendering/RNGPUResource.cpp
    Rendering/RNMaterial.cpp
    Rendering/RNMesh.cpp
    Rendering/RNRenderer.cpp
    Rendering/RNRenderingDescriptor.cpp
    Rendering/RNShader.cpp
    Rendering/RNShaderLibrary.cpp
    Rendering/RNTexture.cpp
    Rendering/RNWindow.cpp
    Scene/RNCamera.cpp
    Scene/RNScene.cpp
    Scene/RNSceneCoordinator.cpp
    Scene/RNSceneNode.cpp
    System/RNFile.cpp
    System/RNFileCoordinator.cpp
    System/RNScreen.cpp
    Threads/RNThread.cpp
    Threads/RNRunLoop.cpp
    Threads/RNSpinlock.cpp
    Threads/RNWorkGroup.cpp
    Threads/RNWorkQueue.cpp
    Threads/RNWorkSource.cpp)

set(RAYNE_HEADERS
    Rayne.h
    Assets/RNAsset.h
    Assets/RNAssetCoordinator.h
    Assets/RNAssetLoader.h
    Base/RNApplication.h
    Base/RNArgumentParser.h
    Base/RNBase.h
    Base/RNOptions.h
    Base/RNException.h
    Base/RNExpected.h
    Base/RNFunction.h
    Base/RNKernel.h
    Base/RNLockGuard.h
    Base/RNMemory.h
    Base/RNMemoryPool.h
    Base/RNScopeGuard.h
    Base/RNSettings.h
    Base/RNSignal.h
    Base/RNSTL.h
    Base/RNTypeTranslator.h
    Base/RNUnicode.h
    Data/RNAny.h
    Data/RNIntrusiveList.h
    Data/RNRingBuffer.h
    Data/RNRRef.h
    Debug/RNLogger.h
    Debug/RNLoggingEngine.h
    Math/RNAlgorithm.h
    Math/RNColor.h
    Math/RNConstants.h
    Math/RNMath.h
    Math/RNMatrix.h
    Math/RNMatrixQuaternion.h
    Math/RNPlane.h
    Math/RNRandom.h
    Math/RNQuaternion.h
    Math/RNRect.h
    Math/RNSIMD.h
    Math/RNVector.h
    Modules/RNModule.h
    Modules/RNModuleCoordinator.h
    Objects/RNArray.h
    Objects/RNAutoreleasePool.h
    Objects/RNCatalogue.h
    Objects/RNCountedSet.h
    Objects/RNData.h
    Objects/RNDictionary.h
    Objects/RNJSONSerialization.h
    Objects/RNKVO.h
    Objects/RNKVOImplementation.h
    Objects/RNNull.h
    Objects/RNNumber.h
    Objects/RNObject.h
    Objects/RNSerialization.h
    Objects/RNSet.h
    Objects/RNCharacterSet.h
    Objects/RNString.h
    Objects/RNValue.h
    Objects/RNWeakStorage.h
    Rendering/RNFramebuffer.h
    Rendering/RNGPUBuffer.h
    Rendering/RNGPUResource.h
    Rendering/RNMaterial.h
    Rendering/RNMesh.h
    Rendering/RNRenderer.h
    Rendering/RNRendererTypes.h
    Rendering/RNRenderingDescriptor.h
    Rendering/RNShader.h
    Rendering/RNShaderLibrary.h
    Rendering/RNTexture.h
    Rendering/RNWindow.h
    Scene/RNCamera.h
    Scene/RNScene.h
    Scene/RNSceneCoordinator.h
    Scene/RNSceneNode.h
    System/RNFile.h
    System/RNFileCoordinator.h
    System/RNScreen.h
    Threads/RNRunLoop.h
    Threads/RNSemaphore.h
    Threads/RNSpinLock.h
    Threads/RNThread.h
    Threads/RNThreadLocalStorage.h
    Threads/RNWorkGroup.h
    Threads/RNWorkQueue.h
    Threads/RNWorkSource.h)

set(RAYNE_HEADERS_PRIVATE
    Assets/RNPNGAssetLoader.h
    Base/RNBaseInternal.h
    Objects/RNHashTableInternal.h
    Objects/RNObjectInternals.h
    Objects/RNStringInternal.h)


set(TARGET_FILES ${RAYNE_SOURCES} ${RAYNE_HEADERS} ${RAYNE_HEADERS_PRIVATE})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/RayneConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/RayneConfig.h)

set(INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_BINARY_DIR}/include
        "${jansson_BINARY_DIR}/include"
        "${libpng_SOURCE_DIR}"
        "${libpng_BINARY_DIR}"
        "${zlib_SOURCE_DIR}")

boost_include(assert INCLUDE_DIRECTORIES)
boost_include(static_assert INCLUDE_DIRECTORIES)
boost_include(config INCLUDE_DIRECTORIES)
boost_include(type_traits INCLUDE_DIRECTORIES)
boost_include(mpl INCLUDE_DIRECTORIES)
boost_include(preprocessor INCLUDE_DIRECTORIES)
boost_include(array INCLUDE_DIRECTORIES)
boost_include(atomic INCLUDE_DIRECTORIES)
boost_include(core INCLUDE_DIRECTORIES)
boost_include(throw_exception INCLUDE_DIRECTORIES)
boost_include(functional INCLUDE_DIRECTORIES)
boost_include(parameter INCLUDE_DIRECTORIES)
boost_include(detail INCLUDE_DIRECTORIES)
boost_include(utility INCLUDE_DIRECTORIES)
boost_include(lockfree INCLUDE_DIRECTORIES)

set(COMPILE_DEFINITIONS "")
set(COMPILE_OPTIONS "")
set(LINK_LIBRARIES "")
set(RAYNE_LIBRARIES "")

if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(IOKIT_LIBRARY IOKit)

    set(COMPILE_DEFINITIONS "-DRN_BUILD_LIBRARY=1")
    set(COMPILE_OPTIONS "-xobjective-c++")

    set(LINK_LIBRARIES ${COCOA_LIBRARY} ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
endif()

set(LINK_LIBRARIES ${LINK_LIBRARIES} jansson png16_static zlibstatic)

rayne_add_library(Rayne "${RAYNE_SOURCES}" "${RAYNE_HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_target_add_definitions(Rayne "${COMPILE_DEFINITIONS}")
rayne_target_add_options(Rayne "${COMPILE_OPTIONS}")
rayne_target_include_directories(Rayne "${INCLUDE_DIRECTORIES}")
rayne_target_link_libraries(Rayne "${LINK_LIBRARIES}")
