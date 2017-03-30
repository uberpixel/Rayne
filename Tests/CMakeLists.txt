cmake_minimum_required(VERSION 2.8.4)
project(Rayne-Tests)

option(BUILD_SAMPLES "Enable building the sample projects" ON)

if(BUILD_SAMPLES)
	add_subdirectory("Basic")
	add_subdirectory("Swords")
	add_subdirectory("Objects")
endif()