
set(DIR_OF_RAYNE_CMAKE ${CMAKE_CURRENT_LIST_DIR})

find_package(PythonInterp 3 REQUIRED)

macro(rayne_link_with _TARGET)
    if(IOS)
        get_target_property(CURRENT_EMBED_FRAMEWORKS ${_TARGET} XCODE_EMBED_FRAMEWORKS)
        if(CURRENT_EMBED_FRAMEWORKS)
            set_property(TARGET ${_TARGET} PROPERTY XCODE_EMBED_FRAMEWORKS ${CURRENT_EMBED_FRAMEWORKS} Rayne ${_TARGET}Lib)
        else()
            set_property(TARGET ${_TARGET} PROPERTY XCODE_EMBED_FRAMEWORKS Rayne ${_TARGET}Lib)
        endif()
        set_target_properties(${_TARGET} PROPERTIES XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY ON)
        set_target_properties(${_TARGET} PROPERTIES XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON)

        target_link_libraries(${_TARGET}Lib PUBLIC Rayne)
        target_include_directories(${_TARGET}Lib SYSTEM PRIVATE ${Rayne_BINARY_DIR}/include)
        target_include_directories(${_TARGET} SYSTEM PRIVATE ${Rayne_BINARY_DIR}/include)
    else()
        add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:Rayne>" "$<TARGET_FILE_DIR:${_TARGET}>/$<TARGET_FILE_NAME:Rayne>")
        target_link_libraries(${_TARGET} PUBLIC Rayne)
        target_include_directories(${_TARGET} SYSTEM PRIVATE ${Rayne_BINARY_DIR}/include)
    endif()

    if(ANDROID)
        target_include_directories(${_TARGET} SYSTEM PRIVATE ${Rayne_BINARY_DIR}/include ${DIR_OF_RAYNE_CMAKE}/../Vendor/android_native_app_glue)

        add_library(android-app-glue STATIC ${DIR_OF_RAYNE_CMAKE}/../Vendor/android_native_app_glue/android_native_app_glue.c)
        target_link_libraries(${_TARGET} PUBLIC android-app-glue android log)

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

macro(INTERNAL_rayne_copy_module_resources _TARGET _MODULE _DIRECTORY)
    foreach(_RESOURCE ${${_MODULE}_MODULE_RESOURCES})
        add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E echo "from $<TARGET_FILE_DIR:${_MODULE}>/${_RESOURCE} to ${_DIRECTORY}/${_MODULE}/${_RESOURCE}")
        add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${_MODULE}>/${_RESOURCE}" "${_DIRECTORY}/${_MODULE}/${_RESOURCE}")
    endforeach()
endmacro()

macro(rayne_use_modules _TARGET _MODULES)
    foreach(_MODULE ${_MODULES})
        set(_MODULE_TARGET "${_MODULE}")
        
        if(IOS)
            target_link_libraries(${_TARGET}Lib PUBLIC ${_MODULE_TARGET})
            target_include_directories(${_TARGET}Lib SYSTEM PRIVATE ${${_MODULE_TARGET}_BINARY_DIR}/include)
            target_include_directories(${_TARGET} SYSTEM PRIVATE ${${_MODULE_TARGET}_BINARY_DIR}/include)

            get_target_property(CURRENT_EMBED_FRAMEWORKS ${_TARGET} XCODE_EMBED_FRAMEWORKS)
            if(CURRENT_EMBED_FRAMEWORKS)
                set_property(TARGET ${_TARGET} PROPERTY XCODE_EMBED_FRAMEWORKS ${CURRENT_EMBED_FRAMEWORKS} ${_MODULE_TARGET})
            else()
                set_property(TARGET ${_TARGET} PROPERTY XCODE_EMBED_FRAMEWORKS ${_MODULE_TARGET})
            endif()
            set_target_properties(${_TARGET} PROPERTIES XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY ON)
            set_target_properties(${_TARGET} PROPERTIES XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON)
        else()
            target_link_libraries(${_TARGET} PUBLIC ${_MODULE_TARGET})
            target_include_directories(${_TARGET} SYSTEM PRIVATE ${${_MODULE_TARGET}_BINARY_DIR}/include)
        endif()


        if(APPLE)
            if(IOS)
                #Don't copy here for iOS, they will already be embeded in the frameworks directory
                #add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${_MODULE_TARGET}>" "$<TARGET_BUNDLE_CONTENT_DIR:${_TARGET}>/ResourceFiles/Resources/Modules/${_MODULE}/${_MODULE}.framework")
            else()
                add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${_MODULE_TARGET}>" "$<TARGET_BUNDLE_CONTENT_DIR:${_TARGET}>/ResourceFiles/Resources/Modules/${_MODULE}")
            endif()
        elseif(NOT ANDROID)
            INTERNAL_rayne_copy_module_resources(${_TARGET} ${_MODULE_TARGET} "$<TARGET_FILE_DIR:${_TARGET}>/Resources/Modules")
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

            INTERNAL_rayne_copy_module_resources(${_TARGET} ${_MODULE} ${android-assets-dir}/Resources/Modules)
        endif()
    endforeach()
endmacro()

macro(rayne_copy_resources _TARGET _RESOURCES _ADDITIONAL_PACK_PARAMS)
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
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${PYTHON_EXECUTABLE} "${DIR_OF_RAYNE_CMAKE}/../Tools/ResourcePacker/pack.py" "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "${android-assets-dir}/${_RESOURCE}" android ${_ADDITIONAL_PACK_PARAMS})
            else()
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "${android-assets-dir}/${_RESOURCE}")
            endif()
        elseif(APPLE)
            if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}")
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${PYTHON_EXECUTABLE} "${DIR_OF_RAYNE_CMAKE}/../Tools/ResourcePacker/pack.py" "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_BUNDLE_CONTENT_DIR:${_TARGET}>/ResourceFiles/${_RESOURCE}" macos ${_ADDITIONAL_PACK_PARAMS})
            else()
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_BUNDLE_CONTENT_DIR:${_TARGET}>/ResourceFiles/${_RESOURCE}")
            endif()
        elseif(WIN32)
            if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}")
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${PYTHON_EXECUTABLE} "${DIR_OF_RAYNE_CMAKE}/../Tools/ResourcePacker/pack.py" "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_FILE_DIR:${_TARGET}>/${_RESOURCE}" windows ${_ADDITIONAL_PACK_PARAMS})
            else()
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_FILE_DIR:${_TARGET}>/${_RESOURCE}")
            endif()
        else()
            if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}")
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${PYTHON_EXECUTABLE} "${DIR_OF_RAYNE_CMAKE}/../Tools/ResourcePacker/pack.py" "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_FILE_DIR:${_TARGET}>/${_RESOURCE}" linux ${_ADDITIONAL_PACK_PARAMS})
            else()
                add_custom_command(TARGET ${_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${_RESOURCE}" "$<TARGET_FILE_DIR:${_TARGET}>/${_RESOURCE}")
            endif()
        endif()
    endforeach()
endmacro()
