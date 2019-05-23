cmake_minimum_required(VERSION 3.7)
project(Vulkan)

find_package(PythonInterp 3)
find_package(Vulkan REQUIRED)

if(NOT PYTHONINTERP_FOUND OR UNIX)
#    if(APPLE)
        set(PYTHON_EXECUTABLE python)
#    else()
#        message( FATAL_ERROR "Python 3 not found" )
#    endif()
endif()


include(../../CMake/LibraryTarget.cmake)

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
        Sources/RNVulkanDebug.cpp
        Sources/RNVulkanDevice.cpp
        Sources/RNVulkanDispatchTable.cpp
        Sources/RNVulkanDispatchTable.h # Required so the custom command triggers
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
        Sources/RNVulkanConstantBuffer.cpp
        ../../Vendor/SPIRV-Cross/spirv_cross.cpp
        ../../Vendor/SPIRV-Cross/spirv_cfg.cpp)

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
        Sources/RNVulkanConstantBuffer.h
        ../../Vendor/SPIRV-Cross/spirv_cross.hpp)

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
#set(INCLUDE_DIRECTORIES "${Rayne_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/Vendor" "${CMAKE_CURRENT_SOURCE_DIR}/../../Vendor/SPIRV-Cross")

rayne_add_library(RayneVulkan "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneVulkan)
rayne_copy_resources(RayneVulkan ${RESOURCES})

target_include_directories(RayneVulkan SYSTEM PRIVATE ${Rayne_BINARY_DIR}/include)
target_include_directories(RayneVulkan SYSTEM PRIVATE ${Vulkan_INCLUDE_DIR})
target_compile_definitions(RayneVulkan PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneVulkan/. DESTINATION lib/Rayne/RayneVulkan)
