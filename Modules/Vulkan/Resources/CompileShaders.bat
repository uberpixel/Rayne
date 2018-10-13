for %%I in (Source\*.*) do (
	"%VULKAN_SDK%"\Bin\glslc.exe Source\%%~nxI
	move a.spv %%~nxI.spv
)