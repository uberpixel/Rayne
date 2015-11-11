cmake_minimum_required(VERSION 2.8.4)
project(Rayne)

include(../CMake/CompilerCheck.cmake)
include(../CMake/BoostInclude.cmake)
include(../CMake/LibraryTarget.cmake)

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
    Rendering/RNRendererCoordinator.cpp
    Rendering/RNRendererDescriptor.cpp
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
    Rendering/RNRendererCoordinator.h
    Rendering/RNRendererDescriptor.h
    Rendering/RNRendererTypes.h
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

find_boost_library(assert)
find_boost_library(static_assert)
find_boost_library(config)
find_boost_library(type_traits)
find_boost_library(mpl)
find_boost_library(preprocessor)
find_boost_library(array)
find_boost_library(atomic)
find_boost_library(core)
find_boost_library(throw_exception)
find_boost_library(functional)
find_boost_library(parameter)
find_boost_library(detail)
find_boost_library(utility)
find_boost_library(lockfree)

set(INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_BINARY_DIR}/include
        "${jansson_BINARY_DIR}/include"
        "${libpng_SOURCE_DIR}"
        "${libpng_BINARY_DIR}"
        "${zlib_SOURCE_DIR}"
        "${BOOST_assert_INCLUDE}"
        "${BOOST_static_assert_INCLUDE}"
        "${BOOST_config_INCLUDE}"
        "${BOOST_type_traits_INCLUDE}"
        "${BOOST_mpl_INCLUDE}"
        "${BOOST_preprocessor_INCLUDE}"
        "${BOOST_array_INCLUDE}"
        "${BOOST_atomic_INCLUDE}"
        "${BOOST_core_INCLUDE}"
        "${BOOST_throw_exception_INCLUDE}"
        "${BOOST_functional_INCLUDE}"
        "${BOOST_parameter_INCLUDE}"
        "${BOOST_detail_INCLUDE}"
        "${BOOST_utility_INCLUDE}"
        "${BOOST_lockfree_INCLUDE}")

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
