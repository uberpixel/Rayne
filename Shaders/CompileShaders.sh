cd $(dirname "$BASH_SOURCE")
python3 ../Tools/ShaderProcessor/convert.py Shaders.json spirv ../Modules/Vulkan/Resources :RayneVulkan:
python3 ../Tools/ShaderProcessor/convert.py Shaders.json metal_macos ../Modules/Metal/Resources_macos :RayneMetal:
python3 ../Tools/ShaderProcessor/convert.py Shaders.json metal_ios ../Modules/Metal/Resources_ios :RayneMetal:
python3 ../Tools/ShaderProcessor/convert.py Shaders.json metal_ios_sim ../Modules/Metal/Resources_ios_sim :RayneMetal:
python3 ../Tools/ShaderProcessor/convert.py Shaders.json metal_visionos ../Modules/Metal/Resources_visionos :RayneMetal:
python3 ../Tools/ShaderProcessor/convert.py Shaders.json metal_visionos_sim ../Modules/Metal/Resources_visionos_sim :RayneMetal:
