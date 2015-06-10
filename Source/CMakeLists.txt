cmake_minimum_required(VERSION 2.8.4)
project(Rayne)

set(RAYNE_SOURCES
    Base/RNMemory.cpp
    Base/RNApplication.cpp
    Base/RNBase.cpp
    Base/RNException.cpp
    Base/RNKernel.cpp
    Base/RNUnicode.cpp
    Math/RNMath.cpp
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
    Objects/RNSet.cpp
    Objects/RNCountedSet.cpp
    Rendering/Metal/RNMetalRenderer.cpp
    Rendering/Metal/RNMetalRendererDescriptor.cpp
    Rendering/Metal/RNMetalWindow.cpp
    Rendering/RNRenderer.cpp
    Rendering/RNRenderingDescriptor.cpp
    Rendering/RNWindow.cpp
    System/RNFileManager.cpp
    System/RNScreen.cpp
    Threads/RNThread.cpp
    Threads/RNRunLoop.cpp
    Threads/RNSpinlock.cpp
    Threads/RNWorkGroup.cpp
    Threads/RNWorkQueue.cpp
    Threads/RNWorkSource.cpp)

set(RAYNE_HEADERS
    Rayne.h
    Base/RNApplication.h
    Base/RNBase.h
    Base/RNDefines.h
    Base/RNOptions.h
    Base/RNException.h
    Base/RNExpected.h
    Base/RNFunction.h
    Base/RNKernel.h
    Base/RNLockGuard.h
    Base/RNMemory.h
    Base/RNScopeGuard.h
    Base/RNSignal.h
    Base/RNSTL.h
    Base/RNTypeTranslator.h
    Base/RNUnicode.h
    Data/RNAny.h
    Data/RNRingBuffer.h
    Data/RNRRef.h
    Math/RNAlgorithm.h
    Math/RNColor.h
    Math/RNConstants.h
    Math/RNMath.h
    Math/RNMatrix.h
    Math/RNMatrixQuaternion.h
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
    Rendering/Metal/RNMetalRenderer.h
    Rendering/Metal/RNMetalRendererDescriptor.h
    Rendering/Metal/RNMetalWindow.h
    Rendering/RNRenderer.h
    Rendering/RNRenderingDescriptor.h
    Rendering/RNWindow.h
    System/RNFileManager.h
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
    Base/RNBaseInternal.h
    Objects/RNHashTableInternal.h
    Objects/RNObjectInternals.h
    Objects/RNStringInternal.h)


set(TARGET_FILES ${RAYNE_SOURCES} ${RAYNE_HEADERS} ${RAYNE_HEADERS_PRIVATE})
set(BUILD_64Bit YES)
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
    set(CMAKE_OSX_SYSROOT /Applications/Xcode-beta.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk)
    set(LINK_LIBRARIES ${COCOA_LIBRARY} ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY} ${METAL_LIBRARY} ${QUARTZCORE_LIBRARY})
endif()

if(NOT APPLE)
    set(BUILD_32Bit YES)
endif()

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

        if(POLICY CMP0042)
            cmake_policy(SET CMP0042 NEW) # Set MACOSX_RPATH=YES by default
        endif()

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

endfunction()


# Target specific stuff
if(APPLE)

    # Mac OS X portion
    # Assume Clang as target compiler

elseif(UNIX)

    # Linux/Cygwin portion
    # Assume GCC as target compiler


elseif(WIN32)

    # Windows portion
    # Assume MSVC as target compiler

endif()



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