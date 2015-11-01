cmake_minimum_required(VERSION 2.8.4)
project(Rayne)

set(VERSION_MAJOR 2)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(VERSION_ABI 10)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

set(RAYNE_INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(RAYNE_INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(RAYNE_INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")

# CMake macros
include(../CMake/CompilerCheck.cmake)
include(../CMake/BoostInclude.cmake)

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
    Rendering/Metal/RNMetalGPUBuffer.cpp
    Rendering/Metal/RNMetalInternals.cpp
    Rendering/Metal/RNMetalRenderer.cpp
    Rendering/Metal/RNMetalRendererDescriptor.cpp
    Rendering/Metal/RNMetalShader.cpp
    Rendering/Metal/RNMetalShaderLibrary.cpp
    Rendering/Metal/RNMetalStateCoordinator.cpp
    Rendering/Metal/RNMetalTexture.cpp
    Rendering/Metal/RNMetalUniformBuffer.cpp
    Rendering/Metal/RNMetalWindow.cpp
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
    Rendering/Metal/RNMetalGPUBuffer.h
    Rendering/Metal/RNMetalRenderer.h
    Rendering/Metal/RNMetalRendererDescriptor.h
    Rendering/Metal/RNMetalShader.h
    Rendering/Metal/RNMetalShaderLibrary.h
    Rendering/Metal/RNMetalStateCoordinator.h
    Rendering/Metal/RNMetalTexture.h
    Rendering/Metal/RNMetalWindow.h
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
    Rendering/Metal/RNMetalInternals.h
    Objects/RNHashTableInternal.h
    Objects/RNObjectInternals.h
    Objects/RNStringInternal.h)


set(TARGET_FILES ${RAYNE_SOURCES} ${RAYNE_HEADERS} ${RAYNE_HEADERS_PRIVATE})
set(BUILD_64Bit YES)

if(NOT APPLE)
    set(BUILD_32Bit YES)
endif()


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/RayneConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/RayneConfig.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

include_directories("${jansson_BINARY_DIR}/include")
include_directories("${libpng_SOURCE_DIR}")
include_directories("${libpng_BINARY_DIR}")
include_directories("${zlib_SOURCE_DIR}")

boost_include(assert)
boost_include(static_assert)
boost_include(config)
boost_include(type_traits)
boost_include(mpl)
boost_include(preprocessor)
boost_include(array)
boost_include(atomic)
boost_include(core)
boost_include(throw_exception)
boost_include(functional)
boost_include(parameter)
boost_include(detail)
boost_include(utility)
boost_include(lockfree)

set(COMPILE_FLAGS "")
set(LINK_LIBRARIES "")

if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(METAL_LIBRARY Metal)
    find_library(QUARTZCORE_LIBRARY QuartzCore)

    set(COMPILE_FLAGS "-x objective-c++ -DRN_BUILD_LIBRARY=1")
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11)
    set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk)
    set(LINK_LIBRARIES ${COCOA_LIBRARY} ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY} ${METAL_LIBRARY} ${QUARTZCORE_LIBRARY})
endif()

set(LINK_LIBRARIES ${LINK_LIBRARIES} jansson png16_static zlibstatic)

add_definitions(${COMPILE_FLAGS})

# Helper macros
function(createTarget arg1 arg2 arg3)

    # Input check
    if(NOT ((arg2 EQUAL 64) OR (arg2 EQUAL 32)))
        message(FATAL_ERROR "Target must be either \"32\" or \"64\", is \"${arg2}\"")
    endif()

    if(NOT (("${arg3}" STREQUAL "STATIC") OR ("${arg3}" STREQUAL "SHARED")))
         message(FATAL_ERROR "Type must be either \"STATIC\" or \"SHARED\", is \"${arg3}\"")
    endif()

    # Create the target name
    set(TARGET_NAME "${arg1}")

    if("${arg3}" STREQUAL "STATIC")
        set(TARGET_NAME "${TARGET_NAME}-static")
    endif()

    if(arg2 EQUAL 64)
        set(TARGET_NAME "${TARGET_NAME}-x64")
    else()
        set(TARGET_NAME "${TARGET_NAME}-x86")
    endif()

    # Apply settings
    if(APPLE)

        set(CMAKE_OSX_ARCHITECTURES x86_64)

    elseif(UNIX)

        if(arg2 EQUAL 64)
            target_compile_options(${arg1} PUBLIC -m64)
        else()
            target_compile_options(${arg1} PUBLIC -m32)
        endif()

    elseif(WIN32)
    endif()

    add_library("${TARGET_NAME}" ${arg3} ${TARGET_FILES})
    target_link_libraries("${TARGET_NAME}" ${LINK_LIBRARIES})
    set_target_properties("${TARGET_NAME}" PROPERTIES VERSION ${VERSION_STRING} SOVERSION ${VERSION_ABI})
    set_target_properties("${TARGET_NAME}" PROPERTIES PUBLIC_HEADER "${RAYNE_HEADERS}")

    # Copy the headers
    foreach(HEADER ${RAYNE_HEADERS})
        add_custom_command(TARGET "${TARGET_NAME}" PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${HEADER}" "${CMAKE_CURRENT_BINARY_DIR}/include/${HEADER}")
    endforeach()

endfunction()

if(BUILD_64Bit)

    createTarget(Rayne 64 SHARED)
    createTarget(Rayne 64 STATIC)

endif()
if(BUILD_32Bit)

    if(APPLE)
        message(WARNING "x86 build on OS X not supported")
    else()
        createTarget(Rayne 32 SHARED)
        createTarget(Rayne 32 STATIC)
    endif()

endif()