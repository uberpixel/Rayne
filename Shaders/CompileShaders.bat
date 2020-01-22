cd /D "%~dp0"
@RD /S /Q "../Modules/Vulkan/Resources"
python ../Tools/ShaderProcessor/convert.py Shadows/Shaders.json spirv ../Modules/Vulkan/Resources ":RayneVulkan:"
