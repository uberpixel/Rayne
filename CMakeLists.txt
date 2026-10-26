cmake_minimum_required(VERSION 3.28)
project(RayneAll VERSION 2.0.0)

option(FETCHCONTENT_UPDATES_DISCONNECTED "Prevent FetchContent updates from contacting remote servers when sources already exist" ON)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/Install" CACHE PATH "Rayne install prefix" FORCE)
endif()

# General settings, independent of target
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules;${CMAKE_MODULE_PATH}")

option(RAYNE_THREAD_SANITIZER "Enable Clang thread sanitizer if available" OFF)
option(RAYNE_ADDRESS_SANITIZER "Enable Clang address sanitizer if available" OFF)
option(RAYNE_MEMORY_SANITIZER "Enable Clang memory sanitizer if available" OFF)
option(RAYNE_VTUNE "Enable VTune if avaialable" ON)
option(RAYNE_UNIVERSE_SCALE "Enable universe-scale position support" OFF)

option(CMAKE_BUILD_TYPE "The Build Type (Debug/Release)" "Debug")

if(RAYNE_UNIVERSE_SCALE)
    set(RAYNE_ENABLE_UNIVERSE_SCALE 1)
else()
    set(RAYNE_ENABLE_UNIVERSE_SCALE 0)
endif()

if((CMAKE_SYSTEM_NAME STREQUAL "visionOS"))
    set(VISIONOS 1)
endif()

if(APPLE)
    if(IOS OR VISIONOS)
        set(RAYNE_DEFAULT_OSX_ARCHITECTURES "arm64")
    elseif(CMAKE_GENERATOR STREQUAL "Xcode")
        set(RAYNE_DEFAULT_OSX_ARCHITECTURES "$(ARCHS_STANDARD)")
    endif()

    if(DEFINED RAYNE_DEFAULT_OSX_ARCHITECTURES AND (NOT DEFINED CMAKE_OSX_ARCHITECTURES OR CMAKE_OSX_ARCHITECTURES STREQUAL ""))
        set(CMAKE_OSX_ARCHITECTURES "${RAYNE_DEFAULT_OSX_ARCHITECTURES}" CACHE STRING "OSX architectures to compile for" FORCE)
    endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Werror=return-type")

    if(MINGW)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
    endif()
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # Thread Sanitizer
    if(${RAYNE_THREAD_SANITIZER})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
    endif()

    # Address Sanitizer
    if(${RAYNE_ADDRESS_SANITIZER})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    endif()

    # Memory Sanitizer
    if(${RAYNE_MEMORY_SANITIZER})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory")
    endif()

    # Optimize compiler generation for better stack traces with memory and/or address sanitizer enabled
    if(${RAYNE_MEMORY_SANITIZER} OR ${RAYNE_ADDRESS_SANITIZER})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-optimize-sibling-calls -fno-omit-frame-pointer")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-optimize-sibling-calls -fno-omit-frame-pointer")
    endif()

    set(CMAKE_CXX_FLAGS_DEBUG "-fno-math-errno -funsafe-math-optimizations ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-fno-math-errno -funsafe-math-optimizations ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_CXX_FLAGS_RELEASE "-fno-math-errno -funsafe-math-optimizations ${CMAKE_CXX_FLAGS_RELEASE}")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${CMAKE_CXX_FLAGS_RELEASE}")
endif()

if(UNIX)
    if(APPLE)
        #execute_process(COMMAND xcrun --show-sdk-path --sdk macosx
        #        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        #        RESULT_VARIABLE OSX_SDK_RESULT
        #        OUTPUT_VARIABLE OSX_SDK_PATH)

        #if(NOT ${OSX_SDK_RESULT} MATCHES "0")
        #    message(FATAL_ERROR "xcrun, invalid return")
        #endif()

        #string(LENGTH ${OSX_SDK_PATH} OSX_SDK_PATH_LENGTH)
        #math(EXPR OSX_SDK_PATH_LENGTH "${OSX_SDK_PATH_LENGTH} - 1")

        #string(SUBSTRING ${OSX_SDK_PATH} 0 ${OSX_SDK_PATH_LENGTH} OSX_SDK_PATH)

        #set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)
        #set(CMAKE_IPHONEOS_DEPLOYMENT_TARGET 15.0)
        #set(CMAKE_OSX_SYSROOT ${OSX_SDK_PATH})

    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    endif()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)

set(CMAKE_DEBUG_POSTFIX "")
set(CMAKE_RELWITHDEBINFO_POSTFIX "")
set(CMAKE_MINSIZEREL_POSTFIX "")

# CMake policies
if(POLICY CMP0042)
    set(CMAKE_POLICY_DEFAULT_CMP0042 NEW)
endif()
if(POLICY CMP0026)
    set(CMAKE_POLICY_DEFAULT_CMP0026 OLD)
endif()
set(CMAKE_POLICY_DEFAULT_CMP0074 NEW)
cmake_policy(SET CMP0074 NEW)

# Include all Rayne targets
add_subdirectory("Vendor")
add_subdirectory("Source")
add_subdirectory("Modules")
add_subdirectory("Tests")

include(CMakePackageConfigHelpers)

set(RAYNE_CMAKE_INSTALL_DIR "cmake/Rayne")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/CMake/RayneConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/RayneConfig.cmake"
    INSTALL_DESTINATION "${RAYNE_CMAKE_INSTALL_DIR}")

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/RayneConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion)

install(EXPORT RayneTargets
    NAMESPACE Rayne::
    FILE RayneTargets.cmake
    DESTINATION "${RAYNE_CMAKE_INSTALL_DIR}")

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/RayneConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/RayneConfigVersion.cmake"
    DESTINATION "${RAYNE_CMAKE_INSTALL_DIR}")

add_custom_target(Rayne-Install make install COMMENT "Installing Rayne and modules...")
