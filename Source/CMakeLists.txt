cmake_minimum_required(VERSION 2.8.4)
project(Rayne)

set(RAYNE_SOURCES
    RNBase.cpp
    RNObject.cpp)

set(BUILD_64Bit YES)

if(NOT APPLE)
    set(BUILD_32Bit YES)
endif()

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

        if(POLICY CMP0042)
            cmake_policy(SET CMP0042 NEW) # Set MACOSX_RPATH=YES by default
        endif()

        set(CMAKE_OSX_ARCHITECTURES x86_64)

    elseif(UNIX)

        if(arg2 EQUAL 64)
            target_compile_options(${arg1} PUBLIC -m64)
        else()
            target_compile_options(${arg1} PUBLIC -m32)
        endif()

    elseif(WIN32)
    endif()

    add_library("${TARGET_NAME}" ${arg3} ${RAYNE_SOURCES})

endfunction()


# Target specific stuff
if(APPLE)

    # Mac OS X portion
    # Assume Clang as target compiler

elseif(UNIX)

    # Linux/Cygwin portion
    # Assume GCC as target compiler


elseif(WIN32)

    # Windows portion
    # Assume MSVC as target compiler

endif()



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