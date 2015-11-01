
macro(copy_resources _RESOURCES _TARGET)

    foreach(RESOURCE ${_RESOURCES})

        if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE}")
            add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE}")
        else()
            add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE}")
        endif()

    endforeach()

endmacro()


