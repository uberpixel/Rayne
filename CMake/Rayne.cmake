
set(DIR_OF_RAYNE_CMAKE ${CMAKE_CURRENT_LIST_DIR})

macro(rayne_link_with _TARGET)
    target_link_libraries(${_TARGET} Rayne)
    add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:Rayne>" "$<TARGET_FILE_DIR:${_TARGET}>/$<TARGET_FILE_NAME:Rayne>")

    target_include_directories(${_TARGET} SYSTEM PRIVATE ${Rayne_BINARY_DIR}/include)

    if(ANDROID)
        target_include_directories(${_TARGET} SYSTEM PRIVATE ${Rayne_BINARY_DIR}/include ${DIR_OF_RAYNE_CMAKE}/../Vendor/android_native_app_glue)

        add_library(android-app-glue STATIC ${DIR_OF_RAYNE_CMAKE}/../Vendor/android_native_app_glue/android_native_app_glue.c)
        target_link_libraries(${_TARGET} android-app-glue android log)

        set_property(TARGET "${_TARGET}" APPEND_STRING PROPERTY LINK_FLAGS " -u ANativeActivity_onCreate")
    endif()

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

        if(APPLE)
            add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${_MODULE_TARGET}>" "$<TARGET_BUNDLE_CONTENT_DIR:${_TARGET}>/Resources/Resources/Modules/${_MODULE}")
        else()
            add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${_MODULE_TARGET}>" "$<TARGET_FILE_DIR:${_TARGET}>/Resources/Modules/${_MODULE}")
        endif()

        if(WIN32 OR (UNIX AND NOT APPLE AND NOT ANDROID))
            add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${_MODULE_TARGET}>" "$<TARGET_FILE_DIR:${_TARGET}>/$<TARGET_FILE_NAME:${_MODULE_TARGET}>")
        endif()

        if(ANDROID)
            list(GET ANDROID_ASSETS_DIRECTORIES 0 android-assets-dir)
            string(LENGTH ${android-assets-dir} length)
            MATH(EXPR length "${length}-2")
            string(SUBSTRING ${android-assets-dir} 1 ${length} android-assets-dir)
            add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${android-assets-dir})

            add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${_MODULE_TARGET}>" "${android-assets-dir}/Resources/Modules/${_MODULE}")
        endif()
    endforeach()
endmacro()

macro(rayne_copy_resources _TARGET _RESOURCES)
    if(ANDROID)
        list(GET ANDROID_ASSETS_DIRECTORIES 0 android-assets-dir)
        string(LENGTH ${android-assets-dir} length)
        MATH(EXPR length "${length}-2")
        string(SUBSTRING ${android-assets-dir} 1 ${length} android-assets-dir)
        add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${android-assets-dir})
    endif()

    foreach(_RESOURCE ${_RESOURCES})
        if(ANDROID)
            if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}")
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND python "${DIR_OF_RAYNE_CMAKE}/../Tools/ResourcePacker/pack.py" "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "${android-assets-dir}/${_RESOURCE}" mobile)
                #add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "${android-assets-dir}/${_RESOURCE}")

                #message(FATAL_ERROR "info: ${android-assets-dir}/${_RESOURCE}")
            else()
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "${android-assets-dir}/${_RESOURCE}")
            endif()
        elseif(APPLE)
            if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}")
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND python "${DIR_OF_RAYNE_CMAKE}/../Tools/ResourcePacker/pack.py" "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_BUNDLE_CONTENT_DIR:${_TARGET}>/Resources/${_RESOURCE}" pc)
                #add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_BUNDLE_CONTENT_DIR:${_TARGET}>/Resources/${_RESOURCE}")
            else()
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_BUNDLE_CONTENT_DIR:${_TARGET}>/Resources/${_RESOURCE}")
            endif()
        else()
            if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}")
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND python "${DIR_OF_RAYNE_CMAKE}/../Tools/ResourcePacker/pack.py" "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_FILE_DIR:${_TARGET}>/${_RESOURCE}" pc)
                #add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_FILE_DIR:${_TARGET}>/${_RESOURCE}")
            else()
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_FILE_DIR:${_TARGET}>/${_RESOURCE}")
            endif()
        endif()
    endforeach()
endmacro()
