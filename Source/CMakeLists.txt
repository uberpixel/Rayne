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

set(RAYNE_INSTALL_LIB_DIR       lib CACHE PATH "Installation directory for libraries")
set(RAYNE_INSTALL_BIN_DIR       bin CACHE PATH "Installation directory for executables")
set(RAYNE_INSTALL_INCLUDE_DIR   include CACHE PATH "Installation directory for header files")

# Compiler support

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckTypeSize)

macro(check_cxx_compiler_attribute _ATTRIBUTE _RESULT)
  check_cxx_source_compiles("${_ATTRIBUTE} int somefunc() { return 0; }
    int main() { return somefunc(); }" ${_RESULT}
    # Some compilers do not fail with a bad flag
    FAIL_REGEX "unrecognized .*option"                     # GNU
    FAIL_REGEX "ignoring unknown option"                   # MSVC
    FAIL_REGEX "warning D9002"                             # MSVC, any lang
  )
endmacro()

macro(check_cxx_compiler_type_attribute _ATTRIBUTE _RESULT)
  check_cxx_source_compiles("int main() { ${_ATTRIBUTE} int bar; return 0; }" ${_RESULT}
    # Some compilers do not fail with a bad flag
    FAIL_REGEX "unrecognized .*option"                     # GNU
    FAIL_REGEX "ignoring unknown option"                   # MSVC
    FAIL_REGEX "warning D9002"                             # MSVC, any lang
  )
endmacro()

check_include_files(stdint.h HAVE_STDINT_H)

# 8 Bit types

check_type_size(int8_t INT8_T)
check_type_size(__int8 __INT8)

if(HAVE_INT8_T)
   set(RAYNE_INT8 int8_t)
elseif(HAVE___INT8)
   set(RAYNE_INT8 __int8)
else()
   set(RAYNE_INT8 char)
endif()

check_type_size(uint8_t UINT8_T)
check_type_size(__uint8 __UINT8)

if(HAVE_UINT8_T)
   set(RAYNE_UINT8 uint8_t)
elseif(HAVE___UINT8)
   set(RAYNE_UINT8 __uint8)
else()
   set(RAYNE_UINT8 "unsigned char")
endif()

# 16 bit types

check_type_size(int16_t INT16_T)
check_type_size(__int16 __INT16)
check_type_size(short SHORT)

if(HAVE_INT16_T)
   set(RAYNE_INT16 int16_t)
elseif(HAVE___INT16)
   set(RAYNE_INT16 __int16)
elseif(HAVE_SHORT AND (${SHORT} EQUAL 2))
   set(RAYNE_INT16 short)
else()
   message(FATAL_ERROR "Could not detect a valid 16-bit integer type")
endif()

check_type_size(uint16_t UINT16_T)
check_type_size(__uint16 __UINT16)
check_type_size("unsigned short" UNSIGNED_SHORT)

if(HAVE_UINT16_T)
   set(RAYNE_UINT16 uint16_t)
elseif(HAVE___UINT16)
   set(RAYNE_UINT16 __uint16)
elseif(HAVE_UNSIGNED_SHORT AND (${UNSIGNED_SHORT} EQUAL 2))
   set(RAYNE_UINT16 "unsigned short")
else()
   message(FATAL_ERROR "Could not detect a valid unsigned 16-bit integer type")
endif()

# 32 bit types

check_type_size(int32_t INT32_T)
check_type_size(__int32 __INT32)
check_type_size(long LONG_INT)
check_type_size(int INT)

if(HAVE_INT32_T)
   set(RAYNE_INT32 int32_t)
elseif(HAVE___INT32)
   set(RAYNE_INT32 __int32)
elseif(HAVE_LONG_INT AND (${LONG_INT} EQUAL 4))
   set(RAYNE_INT32 long)
elseif(HAVE_INT AND (${INT} EQUAL 4))
   set(RAYNE_INT32 int)
else()
   message(FATAL_ERROR "Could not detect a valid 32-bit integer type")
endif()

check_type_size(uint32_t UINT32_T)
check_type_size(__uint32 __UINT32)
check_type_size("unsigned long" UNSIGNED_LONG_INT)
check_type_size("unsigned int" UNSIGNED_INT)

if(HAVE_UINT32_T)
   set(RAYNE_UINT32 uint32_t)
elseif(HAVE___UINT32)
   set(RAYNE_UINT32 __uint32)
elseif(HAVE_UNSIGNED_LONG_INT AND (${UNSIGNED_LONG_INT} EQUAL 4))
   set(RAYNE_UINT32 "unsigned long")
elseif(HAVE_UNSIGNED_INT AND (${UNSIGNED_INT} EQUAL 4))
   set(RAYNE_UINT32 "unsigned int")
else()
   message(FATAL_ERROR "Could not detect a valid unsigned 32-bit integer type")
endif()

# 64 bit

check_type_size(int64_t INT64_T)
check_type_size(__int64 __INT64)
check_type_size("long long" LONG_LONG_INT)

if(HAVE_INT64_T)
   set(RAYNE_INT64 int64_t)
elseif(HAVE___INT64)
   set(RAYNE_INT64 __int64)
elseif(HAVE_LONG_LONG_INT AND (${LONG_LONG_INT} EQUAL 8))
   set(RAYNE_INT64 "long long")
else()
   message(FATAL_ERROR "Could not detect a valid 64-bit integer type")
endif()

check_type_size(uint64_t UINT64_T)
check_type_size(__uint64 __UINT64)
check_type_size("unsigned long long" UNSIGNED_LONG_LONG_INT)

if(HAVE_UINT64_T)
   set(RAYNE_UINT64 uint64_t)
elseif(HAVE___UINT64)
   set(RAYNE_UINT64 __uint64)
elseif(HAVE_UNSIGNED_LONG_LONG_INT AND (${UNSIGNED_LONG_LONG_INT} EQUAL 8))
   set(RAYNE_UINT64 "unsigned long long")
else()
   message(FATAL_ERROR "Could not detect a valid unsiged 64-bit integer type")
endif()

# Check language features

check_c_source_compiles("int main() { if(__PRETTY_FUNCTION__) {} }" HAVE_PRETTY_FUNCTION)
check_c_source_compiles("int main() { if(__FUNCTION__) {} }" HAVE_FUNCTION)

if(HAVE_PRETTY_FUNCTION)
    set(RAYNE_FUNCTION_SIGNATURE __PRETTY_FUNCTION__)
elseif(HAVE_FUNCTION)
    set(RAYNE_FUNCTION_SIGNATURE __FUNCTION__)
else()
    message(FATAL_ERROR "No function signature macro detected")
endif()

check_c_source_compiles("int main() { int x = 0; if(__builtin_expect(!!(x), 0)) {} }" HAVE_BUILTIN_EXPECT)

if(HAVE_BUILTIN_EXPECT)
    set(RAYNE_EXPECT_TRUE "__builtin_expect(!!(x), 1)")
    set(RAYNE_EXPECT_FALSE "__builtin_expect(!!(x), 0)")
else()
    set(RAYNE_EXPECT_TRUE "(x)")
    set(RAYNE_EXPECT_FALSE "(x)")
endif()

check_cxx_compiler_type_attribute("alignas(16)" HAVE_ALIGNAS)
check_cxx_compiler_type_attribute("__declspec(align(16))" HAVE_DECLSPEC_ALIGNAS)

if(HAVE_ALIGNAS)
    set(RAYNE_ALIGNAS "alignas(x)")
elseif(HAVE_DECLSPEC_ALIGNAS)
    set(RAYNE_ALIGNAS "__declspec(align(x))")
else()
    set(RAYNE_ALIGNAS "alignas(x)")
    #message(FATAL_ERROR "Could not detect alignas() equivalent")
endif()

check_cxx_source_compiles("void foo() noexcept; void foo() { }; int main() { foo(); }" HAVE_NOEXCEPT)
check_cxx_source_compiles("void foo() NOEXCEPT; void foo() { }; int main() { foo(); }" HAVE_MSVC_NOEXCEPT)

if(HAVE_NOEXCEPT)
    set(RAYNE_NOEXCEPT noexcept)
elseif(HAVE_MSVC_NOEXCEPT)
    set(RAYNE_NOEXCEPT NOEXCEPT)
else()
    set(RAYNE_NOEXCEPT "")
endif()

check_cxx_source_compiles("constexpr int factorial(int n) { return n <= 1 ? 1 : (n * factorial(n-1)); } int main() {}" HAVE_CONSTEXPR)

if(HAVE_CONSTEXPR)
    set(RAYNE_CONSTEXPR constexpr)
else()
    set(RAYNE_CONSTEXPR const)
endif()

check_cxx_compiler_attribute("__attribute__((noreturn))" HAVE_ATTR_NORETURN)
check_cxx_compiler_attribute("__declspec(noreturn)" HAVE_DECLSPEC_NORETURN)

if(HAVE_ATTR_NORETURN)
    set(RAYNE_NORETURN "__attribute__((noreturn))")
elseif(HAVE_DECLSPEC_NORETURN)
    set(RAYNE_NORETURN "__declspec(noreturn)")
else()
    message(FATAL_ERROR "No noreturn attribute found")
endif()

check_cxx_compiler_attribute("__attribute__((noreturn))" HAVE_ATTR_NORETURN)
check_cxx_compiler_attribute("__declspec(noreturn)" HAVE_DECLSPEC_NORETURN)

if(WIN32)
    set(RAYNE_RNAPI_EXPORT "__declspec(dllexport)")
    set(RAYNE_RNAPI_IMPORT "__declspec(dllimport)")
else()
    set(RAYNE_RNAPI_EXPORT "")
    set(RAYNE_RNAPI_IMPORT "")
endif()

check_cxx_compiler_attribute("__attribute__((__always_inline__))" HAVE_ATTR_INLINE)
check_cxx_compiler_attribute("__attribute__((noinline))" HAVE_ATTR_NOINLINE)
check_cxx_compiler_attribute("__declspec(noinline)" HAVE_DECLSPEC_NOINLINE)

if(HAVE_ATTR_INLINE)
    set(RAYNE_INLINE "inline __attribute__((__always_inline__))")
else()
    set(RAYNE_INLINE "inline")
endif()

if(HAVE_ATTR_NOINLINE)
    set(RAYNE_NOINLINE "__attribute__((noinline))")
elseif(HAVE_DECLSPEC_NOINLINE)
    set(RAYNE_NOINLINE "__declspec(noinline)")
else()
    message(FATAL_ERROR "No noinline attribute available")
endif()

# Target

if((CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
    set(RAYNE_PLATFORM_OSX 1)
    set(RAYNE_PLATFORM_WINDOWS 0)
    set(RAYNE_PLATFORM_LINUX 0)
    set(RAYNE_PLATFORM_POSIX 1)
elseif((CMAKE_SYSTEM_NAME STREQUAL "Windows"))
    set(RAYNE_PLATFORM_OSX 0)
    set(RAYNE_PLATFORM_WINDOWS 1)
    set(RAYNE_PLATFORM_LINUX 0)
    set(RAYNE_PLATFORM_POSIX 0)
elseif((CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    set(RAYNE_PLATFORM_OSX 0)
    set(RAYNE_PLATFORM_WINDOWS 0)
    set(RAYNE_PLATFORM_LINUX 1)
    set(RAYNE_PLATFORM_POSIX 1)
else()
    message(FATAL_ERROR "Unknown system name ${CMAKE_SYSTEM_NAME}")
endif()


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
set(COMPILE_FLAGS "")
set(LINK_LIBRARIES "")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/RayneConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/RayneConfig.h)

include_directories("${jansson_BINARY_DIR}/include")
include_directories("${libpng_SOURCE_DIR}")
include_directories("${libpng_BINARY_DIR}")
include_directories("${zlib_SOURCE_DIR}")
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(${VENDOR_BOOST_DIR}/assert/include)
include_directories(${VENDOR_BOOST_DIR}/static_assert/include)
include_directories(${VENDOR_BOOST_DIR}/config/include)
include_directories(${VENDOR_BOOST_DIR}/type_traits/include)
include_directories(${VENDOR_BOOST_DIR}/mpl/include)
include_directories(${VENDOR_BOOST_DIR}/preprocessor/include)
include_directories(${VENDOR_BOOST_DIR}/array/include)
include_directories(${VENDOR_BOOST_DIR}/atomic/include)
include_directories(${VENDOR_BOOST_DIR}/core/include)
include_directories(${VENDOR_BOOST_DIR}/throw_exception/include)
include_directories(${VENDOR_BOOST_DIR}/functional/include)
include_directories(${VENDOR_BOOST_DIR}/parameter/include)
include_directories(${VENDOR_BOOST_DIR}/detail/include)
include_directories(${VENDOR_BOOST_DIR}/utility/include)
include_directories(${VENDOR_BOOST_DIR}/lockfree/include)

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