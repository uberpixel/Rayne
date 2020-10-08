cd /D "%~dp0"
py -3 ../../../../Rayne/Tools/ShaderProcessor/convert.py Shaders.json cso,spirv,metal ../../Resources/shaders shaders
