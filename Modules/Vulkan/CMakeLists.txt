cmake_minimum_required(VERSION 2.8.4)
project(Vulkan)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/CopyResources.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        Sources/RNVulkanDispatchTable.cpp
        Sources/RNVulkanRendererDescriptor.cpp
        Sources/RNVulkanInit.cpp)

set(RESOURCES
        Resources)

set(HEADERS "")

set(RAYNE_LIBRARIES Rayne)
set(INCLUDE_DIRECTORIES "${Rayne_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/Vendor")

rayne_add_library(RayneVulkan "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_target_include_directories(RayneVulkan "${INCLUDE_DIRECTORIES}")
rayne_target_add_definitions(RayneVulkan "VK_USE_PLATFORM_WIN32_KHR")
rayne_set_module_output_directory(RayneVulkan)
rayne_copy_resources(RayneVulkan ${RESOURCES})

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneVulkan/. DESTINATION lib/Rayne/RayneVulkan)
