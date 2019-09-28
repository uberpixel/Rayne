cd /D "%~dp0"
@RD /S /Q "../Modules/D3D12/Resources"
python ../Tools/ShaderProcessor/convert.py Shadows/Shaders.json cso ../Modules/D3D12/Resources ":RayneD3D12:"
