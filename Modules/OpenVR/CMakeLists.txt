cmake_minimum_required(VERSION 2.8.4)
project(RayneOpenVR)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

if(NOT UNIX OR APPLE)
    set(USE_LIBCXX 1)
else()
    set(USE_LIBCXX 0)
endif()

if(WIN32)
  set(PLATFORM_NAME "win64")
elseif(UNIX AND NOT APPLE)
  if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
    set(PLATFORM_NAME "linux64")
  endif()
elseif(APPLE)
  if(CMAKE_SYSTEM_NAME MATCHES ".*Darwin.*" OR CMAKE_SYSTEM_NAME MATCHES ".*MacOS.*")
    set(PLATFORM_NAME "osx64")
  endif()
endif()

ExternalProject_Add(openvr
        GIT_REPOSITORY https://github.com/ValveSoftware/openvr.git
        GIT_TAG refs/tags/v1.11.11
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CMAKE_CACHE_ARGS
                "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
                "-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH:BOOL=False"
                "-DBUILD_UNIVERSAL:BOOL=False"
                "-DBUILD_SHARED:BOOL=False"
                "-DUSE_LIBCXX:BOOL=${USE_LIBCXX}"
                #"-DCMAKE_CXX_FLAGS:STRING=-DVR_API_PUBLIC -DOPENVR_API_EXPORTS -DWIN32 /EHsc"
        BUILD_BYPRODUCTS
                "${CMAKE_CURRENT_BINARY_DIR}/openvr-prefix/src/openvr/bin/${PLATFORM_NAME}/Debug/libopenvr_api.a"
                "${CMAKE_CURRENT_BINARY_DIR}/openvr-prefix/src/openvr/bin/${PLATFORM_NAME}/Release/libopenvr_api.a"
                "${CMAKE_CURRENT_BINARY_DIR}/openvr-prefix/src/openvr/bin/${PLATFORM_NAME}/Debug/openvr_api64.lib"
                "${CMAKE_CURRENT_BINARY_DIR}/openvr-prefix/src/openvr/bin/${PLATFORM_NAME}/Release/openvr_api64.lib"
        INSTALL_COMMAND "")


set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNOpenVRInit.cpp
        RNOpenVRWindow.cpp
        RNOpenVRSwapChain.cpp)

set(HEADERS
		RNOpenVR.h
        RNOpenVRWindow.h
        RNOpenVRSwapChain.h)

set(RESOURCES
        Resources)

set(DEFINES RN_BUILD_OPENVR OPENVR_BUILD_STATIC)

set(RAYNE_LIBRARIES Rayne RayneVR)
set(INCLUDE_DIRECTORIES
        "${Rayne_BINARY_DIR}/include"
        "${CMAKE_CURRENT_BINARY_DIR}/openvr-prefix/src/openvr/headers"
        "${CMAKE_CURRENT_BINARY_DIR}/../VRWrapper/include")


if(WIN32)
    if(${RN_BUILD_D3D12_MODULE})
        set(SOURCES ${SOURCES}  RNOpenVRD3D12SwapChain.cpp)
        set(HEADERS ${HEADERS} RNOpenVRD3D12SwapChain.h)

        set(RAYNE_LIBRARIES ${RAYNE_LIBRARIES} RayneD3D12)
        set(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} "${CMAKE_CURRENT_BINARY_DIR}/../D3D12/include/Sources")
    endif()

    if(${RN_BUILD_VULKAN_MODULE})
        set(SOURCES ${SOURCES} RNOpenVRVulkanSwapChain.cpp)
        set(HEADERS ${HEADERS} RNOpenVRVulkanSwapChain.h)

        set(RAYNE_LIBRARIES ${RAYNE_LIBRARIES} RayneVulkan)
        set(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} "${CMAKE_CURRENT_BINARY_DIR}/../Vulkan/include/Sources" ${Vulkan_INCLUDE_DIR})
    endif()
elseif(APPLE AND ${RN_BUILD_METAL_MODULE})
    set(SOURCES
        ${SOURCES}
        RNOpenVRMetalSwapChain.cpp)

    set(HEADERS
        ${HEADERS}
        RNOpenVRMetalSwapChain.h)

    set(RAYNE_LIBRARIES ${RAYNE_LIBRARIES} RayneMetal)

    set(INCLUDE_DIRECTORIES
        ${INCLUDE_DIRECTORIES}
        "${CMAKE_CURRENT_BINARY_DIR}/../Metal/include/Sources")
elseif((UNIX AND NOT APPLE) AND ${RN_BUILD_VULKAN_MODULE})
    set(SOURCES
            ${SOURCES}
            RNOpenVRVulkanSwapChain.cpp)

    set(HEADERS
            ${HEADERS}
            RNOpenVRVulkanSwapChain.h)

    set(RAYNE_LIBRARIES ${RAYNE_LIBRARIES} RayneVulkan)

    set(INCLUDE_DIRECTORIES
            ${INCLUDE_DIRECTORIES}
            "${CMAKE_CURRENT_BINARY_DIR}/../Vulkan/include/Sources"
            ${Vulkan_INCLUDE_DIR})
endif()

rayne_add_library(RayneOpenVR "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneOpenVR)
rayne_set_module_resources(RayneOpenVR ${RESOURCES})

if(WIN32)
    if(${RN_BUILD_D3D12_MODULE})
        target_compile_definitions(RayneOpenVR PRIVATE -DRN_OPENVR_SUPPORTS_D3D12)
        target_link_libraries(RayneOpenVR RayneD3D12)
        target_include_directories(RayneOpenVR SYSTEM PRIVATE ${RayneD3D12_BINARY_DIR}/include)
    endif()

    if(${RN_BUILD_VULKAN_MODULE})
        target_compile_definitions(RayneOpenVR PRIVATE -DRN_OPENVR_SUPPORTS_VULKAN)
        target_link_libraries(RayneOpenVR RayneVulkan)
        target_include_directories(RayneOpenVR SYSTEM PRIVATE ${RayneVulkan_BINARY_DIR}/include)
    endif()
elseif(APPLE AND ${RN_BUILD_METAL_MODULE})
    target_compile_definitions(RayneOpenVR PRIVATE -DRN_OPENVR_SUPPORTS_METAL)
    target_link_libraries(RayneOpenVR RayneMetal)
    target_include_directories(RayneOpenVR SYSTEM PRIVATE ${RayneMetal_BINARY_DIR}/include)
    target_compile_options(RayneOpenVR PRIVATE "-xobjective-c++")
    FIND_LIBRARY(IOSURFACE_LIBRARY IOSurface)
    target_link_libraries(RayneOpenVR ${IOSURFACE_LIBRARY})
elseif((UNIX AND NOT APPLE) AND ${RN_BUILD_VULKAN_MODULE})
    target_compile_definitions(RayneOpenVR PRIVATE -DRN_OPENVR_SUPPORTS_VULKAN)
    target_link_libraries(RayneOpenVR RayneVulkan)
    target_include_directories(RayneOpenVR SYSTEM PRIVATE ${RayneVulkan_BINARY_DIR}/include)
endif()

target_include_directories(RayneOpenVR SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneOpenVR PRIVATE "${DEFINES}")

add_dependencies(RayneOpenVR openvr)
if(WIN32)
    target_link_libraries(RayneOpenVR 
        debug "${CMAKE_CURRENT_BINARY_DIR}/openvr-prefix/src/openvr/bin/${PLATFORM_NAME}/Debug/openvr_api64.lib"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/openvr-prefix/src/openvr/bin/${PLATFORM_NAME}/Release/openvr_api64.lib")
else()
    target_link_libraries(RayneOpenVR 
        debug "${CMAKE_CURRENT_BINARY_DIR}/openvr-prefix/src/openvr/bin/${PLATFORM_NAME}/Debug/libopenvr_api.a"
        optimized "${CMAKE_CURRENT_BINARY_DIR}/openvr-prefix/src/openvr/bin/${PLATFORM_NAME}/Release/libopenvr_api.a")
endif()

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneOpenVR/. DESTINATION lib/Rayne/RayneOpenVR)
