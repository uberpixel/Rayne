cmake_minimum_required(VERSION 2.8.4)
project(Rayne)

include(../CMake/CompilerCheck.cmake)
include(../CMake/LibraryTarget.cmake)

if(MSVC)
    include(../CMake/Modules/FindWindowsSDK.cmake)
    get_windowssdk_library_dirs(${WINDOWSSDK_PREFERRED_DIR} WINDOWS_SDK_LIBRARY_PATH)
endif()

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
    Base/RNBumpAllocator.cpp
    Base/RNException.cpp
    Base/RNKernel.cpp
    Base/RNMemoryPool.cpp
    Base/RNNotification.cpp
    Base/RNNotificationManager.cpp
    Base/RNScopeAllocator.cpp
    Base/RNSettings.cpp
    Base/RNUnicode.cpp
    Assets/RNAsset.cpp
    Assets/RNAssetManager.cpp
    Assets/RNAssetManagerInternals.cpp
    Assets/RNAssetLoader.cpp
    Assets/RNBitmap.cpp
    Assets/RNPNGAssetLoader.cpp
    Assets/RNASTCAssetLoader.cpp
    Assets/RNDDSAssetLoader.cpp
    Assets/RNSGMAssetLoader.cpp
    Assets/RNSGAAssetLoader.cpp
    Assets/RNAudioAsset.cpp
    Debug/RNLogFormatter.cpp
    Debug/RNLogger.cpp
    Debug/RNLoggingEngine.cpp
    Input/Devices/RNPS4Controller.cpp
    Input/RNHIDDevice.cpp
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
    Rendering/RNSkeleton.cpp
    Rendering/RNPostProcessing.cpp
    Rendering/RNRenderer.cpp
    Rendering/RNRendererDescriptor.cpp
    Rendering/RNRenderingDevice.cpp
    Rendering/RNRenderPass.cpp
    Rendering/RNShader.cpp
    Rendering/RNShaderLibrary.cpp
    Rendering/RNTexture.cpp
    Rendering/RNWindow.cpp
    Scene/RNCamera.cpp
    Scene/RNEntity.cpp
    Scene/RNInstancingNode.cpp
    Scene/RNScene.cpp
    Scene/RNSceneBasic.cpp
	Scene/RNSceneWithVisibilityLists.cpp
    Scene/RNSceneAttachment.cpp
    Scene/RNSceneManager.cpp
    Scene/RNSceneNode.cpp
    Scene/RNSceneNodeAttachment.cpp
    Scene/RNLight.cpp
    Scene/RNParticle.cpp
    Scene/RNParticleEmitter.cpp
    System/RNFile.cpp
    System/RNFileManager.cpp
    System/RNScreen.cpp
    Threads/RNLockable.cpp
    Threads/RNFutex.cpp
    Threads/RNRecursiveLockable.cpp
    Threads/RNThread.cpp
    Threads/RNRunLoop.cpp
    Threads/RNLockable.cpp
    Threads/RNWorkGroup.cpp
    Threads/RNWorkQueue.cpp
    Threads/RNWorkSource.cpp)

set(RAYNE_HEADERS
    Rayne.h
    Assets/RNAsset.h
    Assets/RNAssetManager.h
    Assets/RNAssetLoader.h
    Assets/RNBitmap.h
    Assets/RNAudioAsset.h
    Base/RNApplication.h
    Base/RNArgumentParser.h
    Base/RNBase.h
    Base/RNBumpAllocator.h
    Base/RNOptions.h
    Base/RNException.h
    Base/RNExpected.h
    Base/RNFunction.h
    Base/RNKernel.h
    Base/RNMemory.h
    Base/RNMemoryPool.h
    Base/RNNotification.h
    Base/RNNotificationManager.h
    Base/RNScopeAllocator.h
    Base/RNScopedFunction.h
    Base/RNScopeGuard.h
    Base/RNSettings.h
    Base/RNSignal.h
    Base/RNTypeTranslator.h
    Base/RNUnicode.h
    Base/RNUnistd.h
    Data/RNAny.h
    Data/RNIntrusiveList.h
    Data/RNRingBuffer.h
    Data/RNSpatialMap.h
    Debug/RNLogFormatter.h
    Debug/RNLogger.h
    Debug/RNLoggingEngine.h
    Input/Devices/RNPS4Controller.h
    Input/RNHID.h
    Input/RNHIDDevice.h
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
    Math/RNInterpolation.h
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
    Rendering/RNSkeleton.h
    Rendering/RNPostProcessing.h
    Rendering/RNRenderer.h
    Rendering/RNRendererDescriptor.h
    Rendering/RNRendererTypes.h
    Rendering/RNRenderingDevice.h
    Rendering/RNRenderPass.h
    Rendering/RNShader.h
    Rendering/RNShaderLibrary.h
    Rendering/RNTexture.h
    Rendering/RNWindow.h
    Scene/RNCamera.h
    Scene/RNEntity.h
    Scene/RNInstancingNode.h
    Scene/RNScene.h
    Scene/RNSceneBasic.h
	Scene/RNSceneWithVisibilityLists.h
    Scene/RNSceneAttachment.h
    Scene/RNSceneManager.h
    Scene/RNSceneNode.h
    Scene/RNSceneNodeAttachment.h
    Scene/RNLight.h
    Scene/RNParticle.h
    Scene/RNParticleEmitter.h
    System/RNFile.h
    System/RNFileManager.h
    System/RNScreen.h
    Threads/RNCondition.h
    Threads/RNLockable.h
    Threads/RNLockGuard.h
    Threads/RNLockTools.h
    Threads/RNLockWrapper.h
    Threads/RNFutex.h
    Threads/RNRecursiveLockable.h
    Threads/RNRunLoop.h
    Threads/RNSemaphore.h
    Threads/RNLockable.h
    Threads/RNThread.h
    Threads/RNThreadLocalStorage.h
    Threads/RNUniqueLock.h
    Threads/RNWorkGroup.h
    Threads/RNWorkQueue.h
    Threads/RNWorkSource.h)

set(RAYNE_HEADERS_PRIVATE
    Assets/RNPNGAssetLoader.h
    Assets/RNASTCAssetLoader.h
    Assets/RNDDSAssetLoader.h
    Assets/RNSGMAssetLoader.h
    Assets/RNSGAAssetLoader.h
    Assets/RNAssetManagerInternals.h
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
elseif(ANDROID)
    set(RAYNE_PLATFORM_SOURCES
            Input/RNInputAndroid.cpp)

    set(RAYNE_PLATFORM_HEADERS_PRIVATE
            Input/RNInputAndroid.h)
elseif(UNIX)
    set(RAYNE_PLATFORM_SOURCES
            Input/RNInputLinux.cpp)

    set(RAYNE_PLATFORM_HEADERS_PRIVATE
            Input/RNInputLinux.h)
endif()

set(INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../Vendor/utf8_v2_3_4/source
        ${CMAKE_CURRENT_SOURCE_DIR}/../Vendor/concurrentqueue
        ${jansson_BINARY_DIR}/include
        ${libpng_SOURCE_DIR}
        ${libpng_BINARY_DIR}
        ${zlib_SOURCE_DIR})

set(COMPILE_OPTIONS "")
set(LINK_LIBRARIES "")
set(RAYNE_LIBRARIES "")

set(COMPILE_DEFINITIONS "-DRN_BUILD_LIBRARY=1")

if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(IOKIT_LIBRARY IOKit)

    set(COMPILE_OPTIONS -xobjective-c++)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmost")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmost")

    set(LINK_LIBRARIES ${COCOA_LIBRARY} ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
endif()

if((UNIX OR MINGW) AND (NOT ANDROID))
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -march=core2 -mfpmath=sse -msse -msse2)

    if(NOT APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    endif()
endif()

set(RAYNE_HAS_VTUNE 0)

if(${RAYNE_VTUNE})
    set(RAYNE_ENABLE_VTUNE 1)
else()
    set(RAYNE_ENABLE_VTUNE 1)
endif()

if(WIN32)
    find_package(VTune)

    if(MINGW)
        find_library(DBGHELP_LIBRARY Dbghelp)
        find_library(PSAPI_LIBRARY Psapi)

        set(LINK_LIBRARIES
                ${DBGHELP_LIBRARY}
                ${PSAPI_LIBRARY})
        set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -latomic")
    endif()

    if(MSVC)
        find_library(SETUPAPI_LIBRARY SetupAPI
                PATHS ${WINDOWS_SDK_LIBRARY_PATH})
    else()
        find_library(SETUPAPI_LIBRARY SetupAPI)
    endif()

    if(NOT SETUPAPI_LIBRARY)
        message(FATAL_ERROR "Setup API library not found")
    endif()

    set(LINK_LIBRARIES
            ${LINK_LIBRARIES}
            ${SETUPAPI_LIBRARY})

    if(VTUNE_FOUND)
        set(RAYNE_HAS_VTUNE 1)

        set(LINK_LIBRARIES
                ${LINK_LIBRARIES}
                ${VTune_LIBRARY})

        set(INCLUDE_DIRECTORIES
                ${INCLUDE_DIRECTORIES}
                ${VTune_INCLUDE_DIRS})
    endif()

endif()

if(UNIX AND (NOT APPLE) AND (NOT ANDROID))
    find_package(XCB REQUIRED)
    find_library(PTHREAD_LIBRARY pthread)
    find_library(DL_LIBRARY dl)

    set(LINK_LIBRARIES ${PTHREAD_LIBRARY} ${DL_LIBRARY} ${XCB_LIBRARIES} -lX11 -lXi) #-ludev -levdev would be needed for hid stuff, but require root...
    set(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${XCB_INCLUDE_DIRS})
endif()

if(ANDROID)
    #set(RAYNE_PLATFORM_SOURCES ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
    #set(RAYNE_HEADERS ${RAYNE_HEADERS} ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.h)
    set(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${ANDROID_NDK}/sources/android/native_app_glue)

    set(LINK_LIBRARIES ${LINK_LIBRARIES} android log)
endif()

set(LINK_LIBRARIES ${LINK_LIBRARIES} jansson png_static zlibstatic)

set(TARGET_SOURCES ${RAYNE_SOURCES} ${RAYNE_HEADERS} ${RAYNE_HEADERS_PRIVATE} ${RAYNE_PLATFORM_SOURCES} ${RAYNE_PLATFORM_HEADERS_PRIVATE})
set(TARGET_HEADERS ${RAYNE_HEADERS})


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/RayneConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/RayneConfig.h)

rayne_add_library(Rayne "${TARGET_SOURCES}" "${TARGET_HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")

target_include_directories(Rayne SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_link_libraries(Rayne "${LINK_LIBRARIES}")
target_compile_definitions(Rayne PRIVATE "${COMPILE_DEFINITIONS}")
target_compile_options(Rayne PRIVATE ${COMPILE_OPTIONS})

#rayne_install(Rayne Rayne ${_TARGETS} LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/. DESTINATION include/Rayne)
