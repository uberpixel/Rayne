cmake_minimum_required(VERSION 3.10.1)
project(RayneD3D12)

include(../../CMake/LibraryTarget.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        Sources/RND3D12.cpp
        Sources/RND3D12Device.cpp
        Sources/RND3D12Framebuffer.cpp
        Sources/RND3D12Resource.cpp
        Sources/RND3D12GPUBuffer.cpp
        Sources/RND3D12Internals.cpp
        Sources/RND3D12Renderer.cpp
        Sources/RND3D12RendererDescriptor.cpp
        Sources/RND3D12Shader.cpp
        Sources/RND3D12ShaderLibrary.cpp
        Sources/RND3D12StateCoordinator.cpp
        Sources/RND3D12Texture.cpp
        Sources/RND3D12UniformBuffer.cpp
        Sources/RND3D12SwapChain.cpp
        Sources/RND3D12Window.cpp
        Sources/RND3D12.cpp)

set(HEADERS
        Sources/d3dx12.h
        Sources/RND3D12.h
        Sources/RND3D12Device.h
        Sources/RND3D12Framebuffer.h
        Sources/RND3D12Resource.h
        Sources/RND3D12GPUBuffer.h
        Sources/RND3D12Internals.h
        Sources/RND3D12Renderer.h
        Sources/RND3D12RendererDescriptor.h
        Sources/RND3D12Shader.h
        Sources/RND3D12ShaderLibrary.h
        Sources/RND3D12StateCoordinator.h
        Sources/RND3D12Texture.h
        Sources/RND3D12UniformBuffer.h
        Sources/RND3D12SwapChain.h
        Sources/RND3D12Window.h)

set(RESOURCES Resources)
set(DEFINES "RN_BUILD_D3D12")

set(RAYNE_LIBRARIES Rayne)
set(INCLUDE_DIRECTORIES ${Rayne_BINARY_DIR}/include)

set(LINK_LIBRARIES d3d12.lib d3dcompiler.lib dxgi.lib)

rayne_add_library(RayneD3D12 "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneD3D12)
rayne_set_module_resources(RayneD3D12 ${RESOURCES})

target_include_directories(RayneD3D12 SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_link_libraries(RayneD3D12 "${LINK_LIBRARIES}")
target_compile_definitions(RayneD3D12 PRIVATE "${DEFINES}")

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Build/. DESTINATION lib/Rayne/RayneD3D12)
