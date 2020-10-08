cd /D "%~dp0"
start py -3 ../Tools/ShaderProcessor/convert.py Shaders.json spirv ../Modules/Vulkan/Resources ":RayneVulkan:"
start py -3 ../Tools/ShaderProcessor/convert.py Shaders.json cso ../Modules/D3D12/Resources ":RayneD3D12:"
