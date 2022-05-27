project(ImageResizer)

cmake_minimum_required(VERSION 3.10)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Vendor/zlib" ${CMAKE_CURRENT_BINARY_DIR}/zlib)

option(PNG_SHARED "Build shared lib" OFF)
option(PNG_TESTS "Build libpng tests" OFF)
option(PNG_EXECUTABLES "Build libpng executables" OFF)
option(PNG_BUILD_ZLIB "Custom zlib location" ON)
set(ZLIB_LIBRARY "zlibstatic" CACHE STRING "hello")
set(ZLIB_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../Vendor/zlib;${CMAKE_CURRENT_BINARY_DIR}/zlib" CACHE STRING "yo")
set(PNG_ARM_NEON "off" CACHE STRING "Enable ARM NEON optimizations: check|on|off; check is default")
option(SKIP_INSTALL_ALL "Skip PNG install" ON)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Vendor/libpng" ${CMAKE_CURRENT_BINARY_DIR}/libpng)

if(PNG_SHARED)
    set_target_properties(png16 PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(png16 PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE)
endif()
if(PNG_TESTS)
    set_target_properties(pngstest PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(pngstest PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE)
    set_target_properties(pngtest PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(pngtest PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE)
    set_target_properties(pngvalid PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(pngvalid PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE)
endif()

set(IMAGERESIZER_SOURCE
		main.cpp
	)

add_executable(resizer ${IMAGERESIZER_SOURCE})
target_link_libraries(resizer png_static zlibstatic)
target_include_directories(resizer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../Vendor/libpng" ${CMAKE_CURRENT_BINARY_DIR}/libpng)

