cd $(dirname "$BASH_SOURCE")
rm -rf ../../Resources/shaders
mkdir ../../Resources/shaders
python ../../../../Rayne/Tools/ShaderProcessor/convert.py Shaders.json cso,spirv,metal ../../Resources/shaders shaders