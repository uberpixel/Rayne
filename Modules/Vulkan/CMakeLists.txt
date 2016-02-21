cmake_minimum_required(VERSION 2.8.4)
project(Vulkan)

if(NOT PYTHON_EXECUTABLE)

    if(WIN32)
        set(PYTHON_EXECUTABLE "C:/Program Files (x86)/Python 3.5/python.exe")
    endif()

endif()

find_package(PythonInterp 3 REQUIRED)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/CopyResources.cmake)

macro(generate_dispatch_table out)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Sources/${out}
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/generators/generate-dispatch-table ${CMAKE_CURRENT_SOURCE_DIR}/Sources/${out}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/generators/generate-dispatch-table)
endmacro()

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

generate_dispatch_table(RNVulkanDispatchTable.h)
generate_dispatch_table(RNVulkanDispatchTable.cpp)

set(SOURCES
        Sources/RNVulkanBackBuffer.cpp
        Sources/RNVulkanBuffer.cpp
        Sources/RNVulkanDevice.cpp
        Sources/RNVulkanDispatchTable.cpp
        Sources/RNVulkanDispatchTable.h
        Sources/RNVulkanInstance.cpp
        Sources/RNVulkanRenderer.cpp
        Sources/RNVulkanRendererDescriptor.cpp
        Sources/RNVulkanWindow.cpp
        Sources/RNVulkanInit.cpp)

set(HEADERS
        Sources/RNVulkanBackBuffer.h
        Sources/RNVulkanBuffer.h
        Sources/RNVulkanDevice.h
        Sources/RNVulkanDispatchTable.h
        Sources/RNVulkanInstance.h
        Sources/RNVulkanRenderer.h
        Sources/RNVulkanRendererDescriptor.h
        Sources/RNVulkanWindow.h)

set(RESOURCES
        Resources)

set(RAYNE_LIBRARIES Rayne)
set(INCLUDE_DIRECTORIES "${Rayne_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/Vendor")

rayne_add_library(RayneVulkan "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_target_include_directories(RayneVulkan "${INCLUDE_DIRECTORIES}")
rayne_target_add_definitions(RayneVulkan "VK_USE_PLATFORM_WIN32_KHR")
rayne_set_module_output_directory(RayneVulkan)
rayne_copy_resources(RayneVulkan ${RESOURCES})

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneVulkan/. DESTINATION lib/Rayne/RayneVulkan)
