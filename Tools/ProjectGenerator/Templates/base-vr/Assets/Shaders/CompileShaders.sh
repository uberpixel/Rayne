cd $(dirname "$BASH_SOURCE")
python3 ../../../../Rayne/Tools/ShaderProcessor/convert.py Shaders.json cso,spirv,metal ../../Resources/shaders shaders
