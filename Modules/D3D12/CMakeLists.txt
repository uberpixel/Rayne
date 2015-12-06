cmake_minimum_required(VERSION 2.8.4)
project(D3D12)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/CopyResources.cmake)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        Sources/RND3D12GPUBuffer.cpp
        Sources/RND3D12Internals.cpp
        Sources/RND3D12Renderer.cpp
        Sources/RND3D12RendererDescriptor.cpp
        Sources/RND3D12Shader.cpp
        Sources/RND3D12ShaderLibrary.cpp
        Sources/RND3D12StateCoordinator.cpp
        Sources/RND3D12Texture.cpp
        Sources/RND3D12UniformBuffer.cpp
        Sources/RND3D12Window.cpp
        Sources/init.cpp)

set(RESOURCES Resources)

set(HEADERS
        Sources/d3dx12.h
        Sources/RND3D12GPUBuffer.h
        Sources/RND3D12Internals.h
        Sources/RND3D12Renderer.h
        Sources/RND3D12RendererDescriptor.h
        Sources/RND3D12Shader.h
        Sources/RND3D12ShaderLibrary.h
        Sources/RND3D12StateCoordinator.h
        Sources/RND3D12Texture.h
        Sources/RND3D12UniformBuffer.h
        Sources/RND3D12Window.h)

set(RAYNE_LIBRARIES Rayne)
set(INCLUDE_DIRECTORIES ${Rayne_BINARY_DIR}/include)

set(LINK_LIBRARIES d3d12.lib d3dcompiler.lib dxgi.lib)

rayne_add_library(RayneD3D12 "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_target_include_directories(RayneD3D12 "${INCLUDE_DIRECTORIES}")
rayne_target_link_libraries(RayneD3D12 "${LINK_LIBRARIES}")
rayne_set_module_output_directory(RayneD3D12)
rayne_copy_resources(RayneD3D12 ${RESOURCES})

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Build/. DESTINATION lib/Rayne/RayneD3D12)
