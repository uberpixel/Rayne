cmake_minimum_required(VERSION 2.8.4)
project(RayneAll)

# General settings, independent of target
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/CMake")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

    if(MINGW)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
    endif()
endif()

if(UNIX)
    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11)
        set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk)
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
set(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/Vendor/zlib" "${CMAKE_BINARY_DIR}/Vendor/zlib")

# Disable Jansson tests
option(JANSSON_WITHOUT_TESTS "Build Jansson without tests" ON)
option(JANSSON_EXAMPLES "Build Jansson examples" OFF)

# Include all Rayne targets
add_subdirectory("Source")
add_subdirectory("Modules")
add_subdirectory("Tests")
add_subdirectory("Vendor")

add_custom_target(Rayne-Install make install COMMENT "Installing Rayne and modules...")
