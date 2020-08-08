cmake_minimum_required(VERSION 2.8.4)
project(Rayne-Tests)

option(RN_BUILD_TESTS "Enable building the sample projects" OFF)

if(RN_BUILD_TESTS)
	message(FATAL_ERROR "gtest needs to be added correctly as external project!")

	#This is close, but untested and unlikely to work as is
	include(ExternalProject)
	set(gtest_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/gtest-prefix/src/gtest)
	set(gtest_LIBRARIES
		${CMAKE_CURRENT_BINARY_DIR}/gtest-prefix/src/gtest-build/libgtest.a
		${CMAKE_CURRENT_BINARY_DIR}/gtest-prefix/src/gtest-build/libgtest_main.a)
	ExternalProject_Add(gtest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG refs/tags/release-1.10.0
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        BUILD_BYPRODUCTS ${gtest_LIBRARIES}
        INSTALL_COMMAND "")

	add_subdirectory("Objects")
endif()