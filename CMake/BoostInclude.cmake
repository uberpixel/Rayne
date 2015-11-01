
macro(boost_include _NAME _INCLUDE_DIRS)

    set(_PATH ${VENDOR_BOOST_DIR}/${_NAME}/include)

    if(NOT (EXISTS ${_PATH} AND IS_DIRECTORY ${_PATH}))
        message(FATAL_ERROR "Could not find boost::${_NAME}")
    endif()

    set(_INCLUDE_DIRS ${_INCLUDE_DIRS} ${_PATH})

endmacro()
