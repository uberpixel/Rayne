cmake_minimum_required(VERSION 2.8.4)
project(RayneAll)

# General settings, independent of target
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules;${CMAKE_MODULE_PATH}")

option(RAYNE_THREAD_SANITIZER "Enable Clang thread sanitizer if available" OFF)
option(RAYNE_ADDRESS_SANITIZER "Enable Clang address sanitizer if available" OFF)
option(RAYNE_MEMORY_SANITIZER "Enable Clang memory sanitizer if available" OFF)
option(RAYNE_VTUNE "Enable VTune if avaialable" ON)

option(CMAKE_BUILD_TYPE "The Build Type (Debug/Release)" "Debug")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

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
endif()


if(UNIX)
    if(APPLE)
        execute_process(COMMAND xcrun --show-sdk-path --sdk macosx
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE OSX_SDK_RESULT
                OUTPUT_VARIABLE OSX_SDK_PATH)

        if(NOT ${OSX_SDK_RESULT} MATCHES "0")
            message(FATAL_ERROR "xcrun, invalid return")
        endif()

        string(LENGTH ${OSX_SDK_PATH} OSX_SDK_PATH_LENGTH)
        math(EXPR OSX_SDK_PATH_LENGTH "${OSX_SDK_PATH_LENGTH} - 1")

        string(SUBSTRING ${OSX_SDK_PATH} 0 ${OSX_SDK_PATH_LENGTH} OSX_SDK_PATH)

        set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12)
        set(CMAKE_OSX_SYSROOT ${OSX_SDK_PATH})

    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -static-libstdc++")
    endif()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)

# Boost
set(VENDOR_BOOST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/boost/libs)

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

#Force default to build static libraries.
option(BUILD_SHARED_LIBS "Build shared libraries." OFF)
option(SKIP_INSTALL_ALL "Skip PNG intall" ON)

#Force static zlib to be used
set(ZLIB_FOUND 1)
set(ZLIB_LIBRARIES zlibstatic)
set(ZLIB_LIBRARY zlibstatic)
set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/zlib" "${CMAKE_CURRENT_BINARY_DIR}/Vendor/zlib")

# Disable Jansson tests
option(JANSSON_WITHOUT_TESTS "Build Jansson without tests" ON)
option(JANSSON_EXAMPLES "Build Jansson examples" OFF)

# Include all Rayne targets
add_subdirectory("Source")
add_subdirectory("Modules")
add_subdirectory("Tests")
add_subdirectory("Vendor")

add_custom_target(Rayne-Install make install COMMENT "Installing Rayne and modules...")
