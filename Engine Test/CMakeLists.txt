#Basic project setup
cmake_minimum_required(VERSION 2.6)
project(TestGame)

#Enable C++11
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

#Specify folder for custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/")

#Specify source files
set(TEST_GAME_SOURCES
	"Test Game/Classes/TGApplication.cpp"
	"Test Game/Classes/TGCutScene.cpp"
	"Test Game/Classes/TGDebugDrawer.cpp"
	"Test Game/Classes/TGFire.cpp"
	"Test Game/Classes/TGForestWorld.cpp"
	"Test Game/Classes/TGLoadingScreen.cpp"
	"Test Game/Classes/TGSmoke.cpp"
	"Test Game/Classes/TGSponzaWorld.cpp"
	"Test Game/Classes/TGSun.cpp"
	"Test Game/Classes/TGWorld.cpp")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Test Game/Classes/")
add_executable(TestGame "Test Game/Supporting files/main.cpp" ${TEST_GAME_SOURCES})

#Find Rayne and set it as dependency
find_package(RAYNE REQUIRED)
if(RAYNE_FOUND)
	include_directories(${RAYNE_INCLUDE_DIRS})
	target_link_libraries(TestGame ${RAYNE_LIBRARIES})
else(RAYNE_FOUND)
	MESSAGE(FATAL_ERROR "Library not found: Rayne")
endif(RAYNE_FOUND)

#Set defines
set_target_properties(TestGame PROPERTIES COMPILE_DEFINITIONS "DEBUG=1") 