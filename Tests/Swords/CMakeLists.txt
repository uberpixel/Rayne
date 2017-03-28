
if(MSVC AND BUILD_D3D12_MODULE AND BUILD_OCULUS_MODULE)
	cmake_minimum_required(VERSION 2.8.4)
	project(Test-Swords)

	include(../../CMake/CopyResources.cmake)
	include(../../CMake/Rayne.cmake)

	set(SOURCES
	        Sources/SGApplication.cpp
	        Sources/SGWorld.cpp
	        Sources/SGPlayer.cpp
	        Sources/main.cpp)

	set(HEADERS
        Sources/SGApplication.h
        Sources/SGWorld.h
        Sources/SGPlayer.h)

	set(RESOURCES
		manifest.json
		Resources)

	add_executable(Swords "${SOURCES}" "${HEADERS}")

	rayne_link_with(Swords)
	rayne_use_modules(Swords RayneAssimp)
	rayne_use_modules(Swords RayneBullet)
	rayne_use_modules(Swords RayneOculus)
	rayne_use_modules(Swords RayneD3D12)
	
	rayne_copy_resources(Swords "${RESOURCES}")
endif()
