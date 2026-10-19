cmake_minimum_required(VERSION 3.23)
project(Rayne-Vendor)

#Force better default values.
option(JANSSON_WITHOUT_TESTS "Build Jansson without tests" ON)
option(JANSSON_EXAMPLES "Build Jansson examples" OFF)
option(JANSSON_BUILD_DOCS "Build documentation (uses python-sphinx)." OFF)

option(PNG_SHARED "Build shared lib" OFF)
option(PNG_TESTS "Build libpng tests" OFF)
option(PNG_TOOLS "Build the libpng tools" OFF)
option(PNG_BUILD_ZLIB "Custom zlib location, else find_package is used" OFF)
option(SKIP_INSTALL_ALL "Skip PNG install" ON)
if(APPLE)
    if(NOT IOS AND NOT VISIONOS)
        #Universal macOS builds don't support libpng optimizations at all (would require a more complicated libpng build setup)
        option(PNG_HARDWARE_OPTIMIZATIONS "Enable hardware optimizations" OFF)
        set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -DPNG_ARM_NEON_OPT=0)
    endif()
endif()


option(ENABLE_COMMONCRYPTO "Enable use of CommonCrypto" OFF)
option(ENABLE_GNUTLS "Enable use of GnuTLS" OFF)
option(ENABLE_MBEDTLS "Enable use of mbed TLS" OFF)
option(ENABLE_OPENSSL "Enable use of OpenSSL" OFF)
option(ENABLE_WINDOWS_CRYPTO "Enable use of Windows cryptography libraries" OFF)
option(ENABLE_BZIP2 "Enable use of BZip2" OFF)
option(ENABLE_LZMA "Enable use of LZMA" OFF)
option(ENABLE_ZSTD "Enable use of Zstandard" OFF)
option(BUILD_TOOLS "Build tools in the src directory (zipcmp, zipmerge, ziptool)" OFF)
option(BUILD_REGRESS "Build regression tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_DOC "Build documentation" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(LIBZIP_DO_INSTALL "Install libzip and the related files" OFF)
option(SHARED_LIB_VERSIONNING "Add SO version in .so build" OFF)

option(ZLIB_BUILD_EXAMPLES "Enable Zlib Examples" OFF)
add_subdirectory("zlib" EXCLUDE_FROM_ALL)

#Force packages that call find_package(ZLIB) to use Rayne's vendored zlib.
add_library(ZLIB::ZLIB ALIAS zlibstatic)
set(ZLIB_LIBRARY zlibstatic)
set(ZLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib CACHE PATH "Path to directory containing zlib header files" FORCE)

add_subdirectory("jansson" EXCLUDE_FROM_ALL)
add_subdirectory("libpng" EXCLUDE_FROM_ALL)
add_subdirectory("libzip" EXCLUDE_FROM_ALL)
add_subdirectory("tracy" EXCLUDE_FROM_ALL)

#Otherwise libzip tries to pull in zlib with a wrong name (which is manually linked already anyway)
set_target_properties(zip PROPERTIES
  INTERFACE_LINK_LIBRARIES ""
)
set_target_properties(png_static PROPERTIES
  INTERFACE_LINK_LIBRARIES ""
)
set_target_properties(zip PROPERTIES
  INTERFACE_LINK_LIBRARIES ""
)