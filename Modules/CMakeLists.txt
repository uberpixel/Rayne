cmake_minimum_required(VERSION 2.8.4)
project(Rayne-Modules)

if(APPLE)
    add_subdirectory("Metal")
endif()
if(MSVC)
	add_subdirectory("D3D12")
endif()

if(WIN32)
    add_subdirectory("Vulkan")
endif()

add_subdirectory("Assimp")
