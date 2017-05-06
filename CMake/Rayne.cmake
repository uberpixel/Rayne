
macro(rayne_link_with _TARGET)
    target_link_libraries(${_TARGET} Rayne)
    add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:Rayne>" "$<TARGET_FILE_DIR:${_TARGET}>/$<TARGET_FILE_NAME:Rayne>")

    target_include_directories(${_TARGET} SYSTEM PRIVATE ${Rayne_BINARY_DIR}/include)

    if(WIN32)
        set_target_properties(${_TARGET} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/${_TARGET}"
                LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Debug/${_TARGET}"
                ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/Debug/${_TARGET}")

        set_target_properties(${_TARGET} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/RelWithDebInfo/${_TARGET}"
                RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/RelWithDebInfo/${_TARGET}"
                RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/RelWithDebInfo/${_TARGET}")

        set_target_properties(${_TARGET} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/${_TARGET}"
                LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release/${_TARGET}"
                ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/Release/${_TARGET}")
    else()
        set_target_properties(${_TARGET} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${_TARGET}"
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${_TARGET}"
                ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${_TARGET}")
    endif()
endmacro()

macro(rayne_use_modules _TARGET _MODULES)
    foreach(_MODULE ${_MODULES})
        set(_MODULE_TARGET "${_MODULE}")
        target_link_libraries(${_TARGET} ${_MODULE_TARGET})
        target_include_directories(${_TARGET} SYSTEM PRIVATE ${${_MODULE_TARGET}_BINARY_DIR}/include)

        add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${_MODULE_TARGET}>" "$<TARGET_FILE_DIR:${_TARGET}>/Resources/Modules/${_MODULE}")
        if(WIN32)
            add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${_MODULE_TARGET}>" "$<TARGET_FILE_DIR:${_TARGET}>/$<TARGET_FILE_NAME:${_MODULE_TARGET}>")
        endif()
    endforeach()
endmacro()

macro(rayne_copy_resources _TARGET _RESOURCES)
    foreach(_RESOURCE ${_RESOURCES})
        if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}")
            add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_FILE_DIR:${_TARGET}>/${_RESOURCE}")
        else()
            add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_FILE_DIR:${_TARGET}>/${_RESOURCE}")
        endif()
    endforeach()
endmacro()
