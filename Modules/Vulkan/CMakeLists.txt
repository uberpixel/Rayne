cmake_minimum_required(VERSION 3.7)
project(Vulkan)

find_package(PythonInterp 3 REQUIRED)

include(../../CMake/LibraryTarget.cmake)
include(ExternalProject)

ExternalProject_Add(spirvcross
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
        GIT_TAG refs/tags/2020-04-03
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        BUILD_BYPRODUCTS
            ${CMAKE_CURRENT_BINARY_DIR}/spirvcross-prefix/src/spirvcross/spirv_cfg.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/spirvcross-prefix/src/spirvcross/spirv_cross_parsed_ir.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/spirvcross-prefix/src/spirvcross/spirv_parser.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/spirvcross-prefix/src/spirvcross/spirv_cross.cpp
        INSTALL_COMMAND "")

ExternalProject_Add(vulkan_headers
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
        GIT_TAG refs/tags/v1.3.210
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND "")

set(VMA_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/VulkanMemoryAllocator-prefix/src/VulkanMemoryAllocator)
ExternalProject_Add(VulkanMemoryAllocator
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG origin/master
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND "")

ExternalProject_Get_property(vulkan_headers SOURCE_DIR)
set(VulkanHeaders_SOURCE_DIR ${SOURCE_DIR})

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Sources/RNVulkanDispatchTable.h ${CMAKE_CURRENT_SOURCE_DIR}/Sources/RNVulkanDispatchTable.cpp
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/generators/generate-dispatch-table parse ${VulkanHeaders_SOURCE_DIR}/include/vulkan/vulkan.h
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/generators/generate-dispatch-table ${CMAKE_CURRENT_SOURCE_DIR}/Sources/RNVulkanDispatchTable.h
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/generators/generate-dispatch-table ${CMAKE_CURRENT_SOURCE_DIR}/Sources/RNVulkanDispatchTable.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/generators/generate-dispatch-table ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/generators/dispatchTableClasses.py ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/generators/dispatchTableParsedHeader.py
        COMMENT "Generate Vulkan Dispatch Table")

add_custom_target(VulkanGenerateDispatchTable DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Sources/RNVulkanDispatchTable.h ${CMAKE_CURRENT_SOURCE_DIR}/Sources/RNVulkanDispatchTable.cpp)


set(SOURCES
        Sources/RNVulkanDebug.cpp
        Sources/RNVulkanDevice.cpp
        Sources/RNVulkanDispatchTable.cpp
        Sources/RNVulkanFramebuffer.cpp
        Sources/RNVulkanGPUBuffer.cpp
        Sources/RNVulkanInstance.cpp
        Sources/RNVulkanRenderer.cpp
        Sources/RNVulkanRendererDescriptor.cpp
        Sources/RNVulkanTexture.cpp
        Sources/RNVulkanWindow.cpp
        Sources/RNVulkanSwapChain.cpp
        Sources/RNVulkanInit.cpp
        Sources/RNVulkanInternals.cpp
        Sources/RNVulkanShader.cpp
        Sources/RNVulkanShaderLibrary.cpp
        Sources/RNVulkanStateCoordinator.cpp
        Sources/RNVulkanDynamicBuffer.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/spirvcross-prefix/src/spirvcross/spirv_cfg.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/spirvcross-prefix/src/spirvcross/spirv_cross_parsed_ir.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/spirvcross-prefix/src/spirvcross/spirv_parser.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/spirvcross-prefix/src/spirvcross/spirv_cross.cpp)

set(HEADERS
        Sources/RNVulkan.h
        Sources/RNVulkanDebug.h
        Sources/RNVulkanDevice.h
        Sources/RNVulkanDispatchTable.h
        Sources/RNVulkanFramebuffer.h
        Sources/RNVulkanGPUBuffer.h
        Sources/RNVulkanInstance.h
        Sources/RNVulkanRenderer.h
        Sources/RNVulkanRendererDescriptor.h
        Sources/RNVulkanTexture.h
        Sources/RNVulkanWindow.h
        Sources/RNVulkanSwapChain.h
        Sources/RNVulkanInternals.h
        Sources/RNVulkanShader.h
        Sources/RNVulkanShaderLibrary.h
        Sources/RNVulkanStateCoordinator.h
        Sources/RNVulkanDynamicBuffer.h
        #../../Vendor/SPIRV-Cross/spirv.hpp
        #../../Vendor/SPIRV-Cross/spirv_common.hpp
        #../../Vendor/SPIRV-Cross/spirv_cfg.hpp
        #../../Vendor/SPIRV-Cross/spirv_cross_parsed_ir.hpp
        #../../Vendor/SPIRV-Cross/spirv_parser.hpp
        #../../Vendor/SPIRV-Cross/spirv_cross.hpp
        )

set(RESOURCES
        Resources)

set(DEFINES "RN_BUILD_VULKAN")

if(WIN32)
    set(DEFINES ${DEFINES} "VK_USE_PLATFORM_WIN32_KHR")
elseif(ANDROID)
    set(DEFINES ${DEFINES} "VK_USE_PLATFORM_ANDROID_KHR")
elseif(UNIX AND NOT APPLE)
    set(DEFINES ${DEFINES} "VK_USE_PLATFORM_XCB_KHR")
endif()

set(RAYNE_LIBRARIES Rayne)

rayne_add_library(RayneVulkan "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneVulkan)
rayne_set_module_resources(RayneVulkan ${RESOURCES})

add_dependencies(VulkanGenerateDispatchTable vulkan_headers)
add_dependencies(RayneVulkan_copyHeaderTarget VulkanGenerateDispatchTable)
add_dependencies(RayneVulkan_copyHeaderTarget spirvcross)
add_dependencies(RayneVulkan_copyHeaderTarget VulkanMemoryAllocator)

target_include_directories(RayneVulkan SYSTEM PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/spirvcross-prefix/src/spirvcross)
target_include_directories(RayneVulkan SYSTEM PRIVATE ${VMA_SOURCE_DIR}/include)
target_include_directories(RayneVulkan SYSTEM PRIVATE ${Rayne_BINARY_DIR}/include)
target_include_directories(RayneVulkan SYSTEM PRIVATE ${VulkanHeaders_SOURCE_DIR}/include/vulkan)
target_compile_definitions(RayneVulkan PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneVulkan/. DESTINATION lib/Rayne/RayneVulkan)
