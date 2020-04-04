cd /D "%~dp0"
@RD /S /Q "../Modules/Vulkan/Resources"
start python ../Tools/ShaderProcessor/convert.py Shadows/Shaders.json spirv ../Modules/Vulkan/Resources ":RayneVulkan:"
start python ../Tools/ShaderProcessor/convert.py Shadows/Shaders.json cso ../Modules/D3D12/Resources ":RayneD3D12:"
