cmake_minimum_required(VERSION 3.10.1)
project(RayneBHaptics)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNBHapticsInit.cpp
        RNBHapticsTypes.cpp
        RNBHapticsManager.cpp
        RNBHapticsAndroidWrapper.cpp)

set(HEADERS
	RNBHaptics.h
        RNBHapticsTypes.h
        RNBHapticsManager.h
        RNBHapticsAndroidWrapper.h)

set(DEFINES RN_BUILD_BHAPTICS)

set(COMPILE_OPTIONS "")
set(RAYNE_LIBRARIES Rayne)

set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include")

if(WIN32)
        set(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} "${CMAKE_CURRENT_BINARY_DIR}/bhaptics-prefix/src/bhaptics/include/shared")
endif()

rayne_add_library(RayneBHaptics "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneBHaptics)

target_include_directories(RayneBHaptics SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneBHaptics PRIVATE "${DEFINES}")
target_compile_options(RayneBHaptics PRIVATE ${COMPILE_OPTIONS})



if(WIN32)
        add_library(BHapticsLib STATIC IMPORTED)

        set(BHAPTICS_DYNAMIC_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/BhapticsCPP/Win64/bhaptics_library.dll)
        set_property(TARGET BHapticsLib PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/BhapticsCPP/Win64/bhaptics_library.lib) #${BHAPTICS_DYNAMIC_LIBRARY})
        set_property(TARGET BHapticsLib PROPERTY IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/BhapticsCPP/Win64/bhaptics_library.lib)
        set_property(TARGET BHapticsLib PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/BhapticsCPP)

        add_dependencies(BHapticsLib bhaptics)
        target_link_libraries(RayneBHaptics PUBLIC BHapticsLib)

        add_custom_command(TARGET RayneBHaptics POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${BHAPTICS_DYNAMIC_LIBRARY}" "$<TARGET_FILE_DIR:RayneBHaptics>")
endif()

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneBHaptics/. DESTINATION lib/Rayne/RayneBHaptics)
