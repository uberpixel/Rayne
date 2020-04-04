import os
import subprocess
import sys
import shutil

args = [
    'cc=\"clang\"',
	'cxx=\"clang++\"',
	'is_official_build=true',
	'is_debug=false',
	'is_component_build=false',
	'skia_use_fontconfig=false',
	'skia_use_freetype=true',
	'skia_enable_atlas_text=false',
	'skia_use_system_freetype2=false',
	'skia_use_system_expat=false',
	'skia_use_icu=false',
	'skia_use_system_libpng=false',
	'skia_use_system_zlib=false',
	'skia_use_libjpeg_turbo_decode=false',
	'skia_use_libjpeg_turbo_encode=false',
	'skia_use_libpng_decode=false',
	'skia_use_libpng_encode=false',
	'skia_use_libwebp_decode=false',
	'skia_use_libwebp_encode=false',
	'skia_use_lua=false',
	'skia_use_piex=false',
	'skia_use_zlib=false',
	'skia_enable_gpu=false',
	'skia_use_vulkan=false',
	'skia_enable_tools=false',
	'skia_enable_pdf=false'
]

def main():
	argString = ' '.join(str(x) for x in args)

	path = os.path.dirname(os.path.realpath(__file__))
	skiaPath = os.path.join(path, 'skia')

	if os.path.exists(skiaPath) == False:
		print('Could not find skia path, please check it out')
		return

	os.chdir(skiaPath)

	subprocess.call(["python", "tools/git-sync-deps"])

	subprocess.call(["bin/gn", "gen", "build/linux", "--args={0}".format(argString)])
	subprocess.call(["ninja", "-C", "build/linux", "skia"])

	try:
		os.mkdir("../libskia/android/")
	except OSError as exc:
		pass
	
	shutil.copyfile("build/linux/libskia.a", "../libskia/linux/libskia.a")

if __name__ == '__main__':
	main()
