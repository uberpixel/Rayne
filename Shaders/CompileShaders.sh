cd $(dirname "$BASH_SOURCE")
python3 ../Tools/ShaderProcessor/convert.py Shaders.json spirv ../Modules/Vulkan/Resources :RayneVulkan:
python3 ../Tools/ShaderProcessor/convert.py Shaders.json metal ../Modules/Metal/Resources :RayneMetal:
