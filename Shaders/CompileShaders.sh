cd $(dirname "$BASH_SOURCE")
rm -rf ../Modules/Vulkan/Resources
rm -rf ../Modules/Metal/Resources
mkdir ../Modules/Vulkan/Resources
mkdir ../Modules/Metal/Resources
python ../Tools/ShaderProcessor/convert.py Shaders.json spirv ../Modules/Vulkan/Resources :RayneVulkan:
python ../Tools/ShaderProcessor/convert.py Shaders.json metal ../Modules/Metal/Resources :RayneMetal: