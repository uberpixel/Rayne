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
    Assets/RNAssetManager.cpp
    Assets/RNAssetLoader.cpp
    Assets/RNPNGAssetLoader.cpp
    Assets/RNSGMAssetLoader.cpp
    Debug/RNLogger.cpp
    Debug/RNLoggingEngine.cpp
    Input/RNInputControl.cpp
    Input/RNInputDevice.cpp
    Input/RNInputManager.cpp
    Math/RNMath.cpp
    Math/RNRandom.cpp
    Modules/RNExtensionPoint.cpp
    Modules/RNModule.cpp
    Modules/RNModuleManager.cpp
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
    Rendering/RNDynamicGPUBuffer.cpp
    Rendering/RNFramebuffer.cpp
    Rendering/RNGPUBuffer.cpp
    Rendering/RNGPUResource.cpp
    Rendering/RNInstancingData.cpp
    Rendering/RNMaterial.cpp
    Rendering/RNMesh.cpp
    Rendering/RNModel.cpp
    Rendering/RNRenderer.cpp
    Rendering/RNRendererDescriptor.cpp
    Rendering/RNRenderingDevice.cpp
    Rendering/RNShader.cpp
    Rendering/RNShaderLibrary.cpp
    Rendering/RNTexture.cpp
    Rendering/RNWindow.cpp
    Scene/RNCamera.cpp
    Scene/RNEntity.cpp
    Scene/RNInstancingNode.cpp
    Scene/RNScene.cpp
    Scene/RNSceneManager.cpp
    Scene/RNSceneNode.cpp
    System/RNFile.cpp
    System/RNFileCoordinator.cpp
    System/RNScreen.cpp
    Threads/RNThread.cpp
    Threads/RNRunLoop.cpp
    Threads/RNSpinLock.cpp
    Threads/RNWorkGroup.cpp
    Threads/RNWorkQueue.cpp
    Threads/RNWorkSource.cpp)

set(RAYNE_HEADERS
    Rayne.h
    Assets/RNAsset.h
    Assets/RNAssetManager.h
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
    Base/RNUnistd.h
    Data/RNAny.h
    Data/RNIntrusiveList.h
    Data/RNRingBuffer.h
    Data/RNSpatialMap.h
    Debug/RNLogger.h
    Debug/RNLoggingEngine.h
    Input/RNInputControl.h
    Input/RNInputDevice.h
    Input/RNInputManager.h
    Math/RNAABB.h
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
    Math/RNSphere.h
    Math/RNVector.h
    Modules/RNExtensionPoint.h
    Modules/RNModule.h
    Modules/RNModuleManager.h
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
    Rendering/RNDynamicGPUBuffer.h
    Rendering/RNFramebuffer.h
    Rendering/RNGPUBuffer.h
    Rendering/RNGPUResource.h
    Rendering/RNInstancingData.h
    Rendering/RNMaterial.h
    Rendering/RNMesh.h
    Rendering/RNModel.h
    Rendering/RNRenderer.h
    Rendering/RNRendererDescriptor.h
    Rendering/RNRendererTypes.h
    Rendering/RNRenderingDevice.h
    Rendering/RNShader.h
    Rendering/RNShaderLibrary.h
    Rendering/RNTexture.h
    Rendering/RNWindow.h
    Scene/RNCamera.h
    Scene/RNEntity.h
    Scene/RNInstancingNode.h
    Scene/RNScene.h
    Scene/RNSceneManager.h
    Scene/RNSceneNode.h
    System/RNFile.h
    System/RNFileCoordinator.h
    System/RNScreen.h
    Threads/RNAdaptiveLock.h
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
    Assets/RNSGMAssetLoader.h
    Base/RNBaseInternal.h
    Objects/RNHashTableInternal.h
    Objects/RNObjectInternals.h
    Objects/RNStringInternal.h)

set(RAYNE_PLATFORM_SOURCES "")
set(RAYNE_PLATFORM_HEADERS_PRIVATE "")

if(APPLE)
    set(RAYNE_PLATFORM_SOURCES
            Input/RNInputOSX.cpp)

    set(RAYNE_PLATFORM_HEADERS_PRIVATE
            Input/RNInputOSX.h)
elseif(WIN32)
    set(RAYNE_PLATFORM_SOURCES
            Input/RNInputWindows.cpp)

    set(RAYNE_PLATFORM_HEADERS_PRIVATE
            Input/RNInputWindows.h)
elseif(UNIX)
    set(RAYNE_PLATFORM_SOURCES
            Input/RNInputLinux.cpp)

    set(RAYNE_PLATFORM_HEADERS_PRIVATE
            Input/RNInputLinux.h)
endif()

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
        ${CMAKE_CURRENT_SOURCE_DIR}/../Vendor/utf8_v2_3_4/source
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

set(COMPILE_OPTIONS "")
set(LINK_LIBRARIES "")
set(RAYNE_LIBRARIES "")

set(COMPILE_DEFINITIONS "-DRN_BUILD_LIBRARY=1")

if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(IOKIT_LIBRARY IOKit)

    set(COMPILE_OPTIONS "-xobjective-c++")

    set(LINK_LIBRARIES ${COCOA_LIBRARY} ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
endif()

if(WIN32)
    if(MINGW)
        find_library(DBGHELP_LIBRARY Dbghelp)
        find_library(PSAPI_LIBRARY Psapi)

        set(LINK_LIBRARIES ${DBGHELP_LIBRARY} ${PSAPI_LIBRARY})
        set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -latomic")
    endif()
endif()

if(UNIX AND (NOT APPLE))
    find_package(XCB REQUIRED)
    find_library(PTHREAD_LIBRARY pthread)
    find_library(DL_LIBRARY dl)

    set(LINK_LIBRARIES ${PTHREAD_LIBRARY} ${DL_LIBRARY} ${XCB_LIBRARIES})
    set(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${XCB_INCLUDE_DIRS})
endif()

set(LINK_LIBRARIES ${LINK_LIBRARIES} jansson png16_static zlibstatic)

set(TARGET_SOURCES ${RAYNE_SOURCES} ${RAYNE_HEADERS} ${RAYNE_HEADERS_PRIVATE} ${RAYNE_PLATFORM_SOURCES} ${RAYNE_PLATFORM_HEADERS_PRIVATE})
set(TARGET_HEADERS ${RAYNE_HEADERS})

rayne_add_library(Rayne "${TARGET_SOURCES}" "${TARGET_HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")

target_include_directories(Rayne SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_link_libraries(Rayne "${LINK_LIBRARIES}")
target_compile_definitions(Rayne PRIVATE "${COMPILE_DEFINITIONS}")
target_compile_definitions(Rayne PRIVATE "${COMPILE_OPTIONS}")

#rayne_install(Rayne Rayne ${_TARGETS} LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/. DESTINATION include/Rayne)
