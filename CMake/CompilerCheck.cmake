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
check_include_files(stddef.h HAVE_STDDEF_H)

#The following types are all supported since c++17
# 8 Bit types
set(RAYNE_INT8 int8_t)
set(RAYNE_UINT8 uint8_t)

# 16 bit types
set(RAYNE_INT16 int16_t)
set(RAYNE_UINT16 uint16_t)

# 32 bit types
set(RAYNE_INT32 int32_t)
set(RAYNE_UINT32 uint32_t)

# 64 bit
set(RAYNE_INT64 int64_t)
set(RAYNE_UINT64 uint64_t)


# Check language features

#The following attributes are all supported since c++17
set(RAYNE_ALIGNAS "alignas(x)")
set(RAYNE_NOEXCEPT noexcept)
set(RAYNE_UNUSED "[[maybe_unused]]")
set(RAYNE_CONSTEXPR constexpr)
set(RAYNE_NORETURN "[[noreturn]]")
set(RAYNE_SUPPORTS_TRIVIALLY_COPYABLE 1)

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
    set(RAYNE_PLATFORM_VISIONOS 0)
    set(RAYNE_PLATFORM_IOS 0)
    set(RAYNE_PLATFORM_WINDOWS 0)
    set(RAYNE_PLATFORM_LINUX 0)
    set(RAYNE_PLATFORM_POSIX 1)
    set(RAYNE_PLATFORM_ANDROID 0)
elseif((CMAKE_SYSTEM_NAME STREQUAL "iOS"))
    set(RAYNE_PLATFORM_OSX 0)
    set(RAYNE_PLATFORM_VISIONOS 0)
    set(RAYNE_PLATFORM_IOS 1)
    set(RAYNE_PLATFORM_WINDOWS 0)
    set(RAYNE_PLATFORM_LINUX 0)
    set(RAYNE_PLATFORM_POSIX 1)
    set(RAYNE_PLATFORM_ANDROID 0)
elseif((CMAKE_SYSTEM_NAME STREQUAL "visionOS"))
    set(RAYNE_PLATFORM_OSX 0)
    set(RAYNE_PLATFORM_VISIONOS 1)
    set(RAYNE_PLATFORM_IOS 0)
    set(RAYNE_PLATFORM_WINDOWS 0)
    set(RAYNE_PLATFORM_LINUX 0)
    set(RAYNE_PLATFORM_POSIX 1)
    set(RAYNE_PLATFORM_ANDROID 0)
elseif((CMAKE_SYSTEM_NAME STREQUAL "Windows"))
    set(RAYNE_PLATFORM_OSX 0)
    set(RAYNE_PLATFORM_VISIONOS 0)
    set(RAYNE_PLATFORM_IOS 0)
    set(RAYNE_PLATFORM_WINDOWS 1)
    set(RAYNE_PLATFORM_LINUX 0)
    set(RAYNE_PLATFORM_POSIX 0)
    set(RAYNE_PLATFORM_ANDROID 0)
elseif((CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    set(RAYNE_PLATFORM_OSX 0)
    set(RAYNE_PLATFORM_VISIONOS 0)
    set(RAYNE_PLATFORM_IOS 0)
    set(RAYNE_PLATFORM_WINDOWS 0)
    set(RAYNE_PLATFORM_LINUX 1)
    set(RAYNE_PLATFORM_POSIX 1)
    set(RAYNE_PLATFORM_ANDROID 0)
elseif((CMAKE_SYSTEM_NAME STREQUAL "Android"))
    set(RAYNE_PLATFORM_OSX 0)
    set(RAYNE_PLATFORM_VISIONOS 0)
    set(RAYNE_PLATFORM_IOS 0)
    set(RAYNE_PLATFORM_WINDOWS 0)
    set(RAYNE_PLATFORM_LINUX 0)
    set(RAYNE_PLATFORM_POSIX 1) #technically not correct, but keeps the code a little cleaner...
    set(RAYNE_PLATFORM_ANDROID 1)
else()
    message(FATAL_ERROR "Unknown system name ${CMAKE_SYSTEM_NAME}")
endif()
