cmake_minimum_required(VERSION 3.10.1)
project(RayneMetal LANGUAGES OBJCXX)

include(../../CMake/LibraryTarget.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        Sources/RNMetalDevice.cpp
        Sources/RNMetalFramebuffer.cpp
        Sources/RNMetalGPUBuffer.cpp
        Sources/RNMetalInternals.cpp
        Sources/RNMetalRenderer.cpp
        Sources/RNMetalRendererDescriptor.cpp
        Sources/RNMetalShader.cpp
        Sources/RNMetalShaderLibrary.cpp
        Sources/RNMetalStateCoordinator.cpp
        Sources/RNMetalTexture.cpp
        Sources/RNMetalUniformBuffer.cpp
        Sources/RNMetalWindow.cpp
        Sources/RNMetalSwapChain.cpp
        Sources/RNMetalInit.cpp)

if(IOS)
        set(RESOURCES Resources_ios)
else()
        set(RESOURCES Resources_macos)
endif()

set(HEADERS
        Sources/RNMetal.h
        Sources/RNMetalDevice.h
        Sources/RNMetalFramebuffer.h
        Sources/RNMetalGPUBuffer.h
        Sources/RNMetalInternals.h
        Sources/RNMetalRenderer.h
        Sources/RNMetalRendererDescriptor.h
        Sources/RNMetalShader.h
        Sources/RNMetalShaderLibrary.h
        Sources/RNMetalStateCoordinator.h
        Sources/RNMetalTexture.h
        Sources/RNMetalUniformBuffer.h
        Sources/RNMetalWindow.h
        Sources/RNMetalSwapChain.h)

set(DEFINES
        "RN_BUILD_METAL")


set(RAYNE_LIBRARIES Rayne)
set(INCLUDE_DIRECTORIES ${Rayne_BINARY_DIR}/include)

find_library(METAL_LIBRARY Metal)
if(IOS)
        find_library(FOUNDATION_LIBRARY Foundation)
        find_library(UIKIT_LIBRARY UIKit)
        set(LINK_LIBRARIES ${FOUNDATION_LIBRARY} ${UIKIT_LIBRARY} ${METAL_LIBRARY})
elseif(APPLE)
        find_library(COCOA_LIBRARY Cocoa)
        find_library(QUARTZCORE_LIBRARY QuartzCore)
        set(LINK_LIBRARIES ${COCOA_LIBRARY} ${QUARTZCORE_LIBRARY} ${METAL_LIBRARY})
endif()

rayne_add_library(RayneMetal "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneMetal)
rayne_set_module_resources(RayneMetal ${RESOURCES})

target_include_directories(RayneMetal SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_link_libraries(RayneMetal PUBLIC "${LINK_LIBRARIES}")
target_compile_options(RayneMetal PRIVATE "-xobjective-c++")
target_compile_definitions(RayneMetal PRIVATE "${DEFINES}")

add_custom_command(TARGET RayneMetal PRE_BUILD COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:RayneMetal>/ResourceFiles/Resources")

if(IOS)
        add_custom_command(TARGET RayneMetal PRE_BUILD COMMAND ${CMAKE_COMMAND} -E rename "$<TARGET_FILE_DIR:RayneMetal>/ResourceFiles/Resources_ios" "$<TARGET_FILE_DIR:RayneMetal>/ResourceFiles/Resources")
else()
        add_custom_command(TARGET RayneMetal PRE_BUILD COMMAND ${CMAKE_COMMAND} -E rename "$<TARGET_FILE_DIR:RayneMetal>/Resources_macos" "$<TARGET_FILE_DIR:RayneMetal>/Resources")
endif()


rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneMetal/. DESTINATION lib/Rayne/RayneMetal)
