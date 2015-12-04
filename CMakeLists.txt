cmake_minimum_required(VERSION 2.8.4)
project(RayneAll)

# General settings, independent of target

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SCL_SECURE_NO_WARNINGS")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11)
    set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk)
endif()

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

#Exclude all install settings
macro(install)
endmacro()

#Force default to build static libraries.
option(BUILD_SHARED_LIBS "Build shared libraries." OFF)

#Force static zlib to be used
set(ZLIB_FOUND 1)
set(ZLIB_LIBRARIES zlibstatic)
set(ZLIB_LIBRARY zlibstatic)
set(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/Vendor/zlib" "${CMAKE_BINARY_DIR}/Vendor/zlib")

# Include all Rayne targets
add_subdirectory("Source")
add_subdirectory("Modules")
add_subdirectory("Tests")
add_subdirectory("Vendor")

add_custom_target(Rayne-Install make install COMMENT "Installing Rayne and modules...")
