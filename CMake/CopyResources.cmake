
macro(copy_resources _RESOURCES _TARGET)

    foreach(_RESOURCE ${_RESOURCES})

        if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}")
            add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/${_RESOURCE}")
        else()
            add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/${_RESOURCE}")
        endif()

    endforeach()

endmacro()
