cmake_minimum_required(VERSION 2.8.4)
project(RayneAll)

# General settings, independent of target
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11)
set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk)

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

# Include all Rayne targets
add_subdirectory("Source")
add_subdirectory("Modules")
add_subdirectory("Tests")
add_subdirectory("Vendor")