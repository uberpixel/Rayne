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

check_cxx_source_compiles("void foo() noexcept { } int main() { foo(); }" HAVE_NOEXCEPT)
check_cxx_source_compiles("void foo() NOEXCEPT { } int main() { foo(); }" HAVE_MSVC_NOEXCEPT)

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
