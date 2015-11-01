
macro(find_boost_library _NAME)

    set(_PATH ${VENDOR_BOOST_DIR}/${_NAME}/include)

    if(NOT (EXISTS ${_PATH} AND IS_DIRECTORY ${_PATH}))
        message(FATAL_ERROR "Could not find boost::${_NAME}")
    endif()

    set(BOOST_${_NAME}_INCLUDE ${_PATH})

endmacro()
