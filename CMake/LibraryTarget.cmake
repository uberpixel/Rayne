
macro(__rayne_create_target _NAME _ARCH _TYPE _SOURCES _HEADERS _RAYNE_LIBRARIES _VERSION _ABI)

    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)

    # Input check
    if(NOT ((${_ARCH} EQUAL 64) OR (${_ARCH} EQUAL 32)))
        message(FATAL_ERROR "Target must be either \"32\" or \"64\", is \"${_ARCH}\"")
    endif()

    if(NOT (("${_TYPE}" STREQUAL "STATIC") OR ("${_TYPE}" STREQUAL "SHARED")))
        message(FATAL_ERROR "Type must be either \"STATIC\" or \"SHARED\", is \"${_TYPE}\"")
    endif()

    # Create the target name
    set(TARGET_NAME "${_NAME}")

    if("${_TYPE}" STREQUAL "STATIC")
        set(TARGET_NAME "${TARGET_NAME}-static")
    endif()

    if(${_ARCH} EQUAL 64)
        set(TARGET_NAME "${TARGET_NAME}-x64")
    else()
        set(TARGET_NAME "${TARGET_NAME}-x86")
    endif()

    # Create the target
    if(APPLE)

        set(CMAKE_OSX_ARCHITECTURES x86_64)

        if(POLICY CMP0042)
            cmake_policy(SET CMP0042 NEW) # Set MACOSX_RPATH=YES by default
        endif()

    elseif(UNIX)
    elseif(WIN32)
    endif()

    set(ALL_SOURCES "${_SOURCES}" "${_HEADERS}")

    add_library("${TARGET_NAME}" ${_TYPE} ${ALL_SOURCES})
    #set_target_properties("${TARGET_NAME}" PROPERTIES VERSION ${_VERSION} SOVERSION ${_ABI})
    #set_target_properties("${TARGET_NAME}" PROPERTIES PUBLIC_HEADER "${_HEADERS}")

    if(UNIX)
        if(${_ARCH} EQUAL 64)
            target_compile_options(${TARGET_NAME} PUBLIC -m64)
        else()
            target_compile_options(${TARGET_NAME} PUBLIC -m32)
        endif()
    endif()

    if(NOT ("${_HEADERS}" STREQUAL ""))
        foreach(HEADER ${_HEADERS})
            add_custom_command(TARGET "${TARGET_NAME}" PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${HEADER}" "${CMAKE_CURRENT_BINARY_DIR}/include/${HEADER}")
        endforeach()
    endif()

    if(NOT ("${_RAYNE_LIBRARIES}" STREQUAL ""))
        foreach(LIBRARY ${_RAYNE_LIBRARIES})

            set(LIBRARY_NAME ${LIBRARY})

            if("${_TYPE}" STREQUAL "STATIC")
                set(LIBRARY_NAME "${LIBRARY_NAME}-static")
            endif()

            if(${_ARCH} EQUAL 64)
                set(LIBRARY_NAME "${LIBRARY_NAME}-x64")
            else()
                set(LIBRARY_NAME "${LIBRARY_NAME}-x86")
            endif()

            target_link_libraries("${TARGET_NAME}" ${LIBRARY_NAME})
        endforeach()
    endif()

endmacro()

macro(__rayne_target_names _NAME _NAMES)

    if(APPLE)

        set(${_NAMES}
                "${_NAME}-x64"
                "${_NAME}-static-x64")

    else()

        set(${_NAMES}
                "${_NAME}-x64"
                "${_NAME}-static-x64"
                "${_NAME}-x86"
                "${_NAME}-static-x86")

    endif()

endmacro()

macro(rayne_add_library _NAME _SOURCES _HEADERS _RAYNE_LIBRARIES _VERSION _ABI)

    if(APPLE)

        __rayne_create_target(${_NAME} 64 SHARED "${_SOURCES}" "${_HEADERS}" "${_RAYNE_LIBRARIES}" "${_VERSION}" "${_ABI}")
        __rayne_create_target(${_NAME} 64 STATIC "${_SOURCES}" "${_HEADERS}" "${_RAYNE_LIBRARIES}" "${_VERSION}" "${_ABI}")

        set(LIBRARIES
                "${_NAME}-x64"
                "${_NAME}-static-x64")

    else()

        __rayne_create_target(${_NAME} 32 SHARED "${_SOURCES}" "${_HEADERS}" "${_RAYNE_LIBRARIES}" "${_VERSION}" "${_ABI}")
        __rayne_create_target(${_NAME} 32 STATIC "${_SOURCES}" "${_HEADERS}" "${_RAYNE_LIBRARIES}" "${_VERSION}" "${_ABI}")

        __rayne_create_target(${_NAME} 64 SHARED "${_SOURCES}" "${_HEADERS}" "${_RAYNE_LIBRARIES}" "${_VERSION}" "${_ABI}")
        __rayne_create_target(${_NAME} 64 STATIC "${_SOURCES}" "${_HEADERS}" "${_RAYNE_LIBRARIES}" "${_VERSION}" "${_ABI}")

        set(LIBRARIES
                "${_NAME}-x64"
                "${_NAME}-static-x64"
                "${_NAME}-x86"
                "${_NAME}-static-x86")

    endif()

    add_custom_target(${_NAME})

    foreach(LIBRARY ${LIBRARIES})
        add_dependencies(${_NAME} "${LIBRARY}")
    endforeach()

endmacro()

macro(rayne_target_add_definitions _TARGET _DEFINITIONS)

    __rayne_target_names(${_TARGET} TARGETS)

    foreach(TARGET ${TARGETS})

        foreach(DEFINITION ${_DEFINITIONS})
            target_compile_definitions(${TARGET} PRIVATE "${DEFINITION}")
        endforeach()

    endforeach()

endmacro()

macro(rayne_target_add_options _TARGET _OPTIONS)

    __rayne_target_names(${_TARGET} TARGETS)

    foreach(TARGET ${TARGETS})

        foreach(OPTION ${_OPTIONS})
            target_compile_options(${TARGET} PRIVATE "${OPTION}")
        endforeach()

    endforeach()

endmacro()

macro(rayne_target_link_libraries _TARGET _LIBRARIES)

    __rayne_target_names(${_TARGET} TARGETS)

    foreach(TARGET ${TARGETS})
        target_link_libraries(${TARGET} "${_LIBRARIES}")
    endforeach()

endmacro()

macro(rayne_target_include_directories _TARGET _DIRECTORIES)

    __rayne_target_names(${_TARGET} TARGETS)

    foreach(TARGET ${TARGETS})
        target_include_directories("${TARGET}" PRIVATE "${_DIRECTORIES}")
    endforeach()

endmacro()

macro(rayne_copy_resources _TARGET _RESOURCES)

    __rayne_target_names(${_TARGET} _TARGETS)

    foreach(_RTARGET ${_TARGETS})

        foreach(_RESOURCE ${_RESOURCES})

            if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}")
                add_custom_command(TARGET ${_RTARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/Build/${_RESOURCE}")
            else()
                add_custom_command(TARGET ${_RTARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/Build/${_RESOURCE}")
            endif()

        endforeach()

    endforeach()

endmacro()
