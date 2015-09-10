cmake_minimum_required(VERSION 2.8.4)
project(RayneAll)

# General settings, independent of target
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Boost
set(VENDOR_BOOST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/boost/libs)

macro(check_boost_include _NAME)

    set(_PATH ${VENDOR_BOOST_DIR}/${_NAME}/include)

    if(NOT (EXISTS ${_PATH} AND IS_DIRECTORY ${_PATH}))

        message(FATAL_ERROR "Could not find boost::${_NAME}")

    endif()

endmacro()

check_boost_include(assert)
check_boost_include(static_assert)
check_boost_include(config)
check_boost_include(type_traits)
check_boost_include(mpl)
check_boost_include(preprocessor)
check_boost_include(array)
check_boost_include(atomic)
check_boost_include(core)
check_boost_include(throw_exception)
check_boost_include(functional)
check_boost_include(parameter)
check_boost_include(detail)
check_boost_include(utility)
check_boost_include(lockfree)

# Include all Rayne targets
add_subdirectory("Source")
add_subdirectory("Tests")
add_subdirectory("Vendor")