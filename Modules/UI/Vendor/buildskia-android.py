import os
import subprocess
import sys
import shutil

args = [
	'ndk=\"/Users/slin/Library/Android/sdk/ndk-bundle\" target_cpu=\"arm64\"',
	'ndk_api=24',
	'is_official_build=true',
	'is_debug=false',
	'is_component_build=false',
	#'skia_use_fontconfig=false',
	'skia_use_freetype=true',
	'skia_enable_atlas_text=false',
	'skia_use_system_freetype2=false',
	'skia_use_system_expat=false',
	'skia_use_icu=false',
	'skia_use_system_libpng=false',
	'skia_use_system_zlib=false',
	'skia_use_libjpeg_turbo=false',
	'skia_use_libpng=false',
	'skia_use_libwebp=false',
	'skia_use_lua=false',
	'skia_use_piex=false',
	'skia_use_zlib=false',
#	'skia_enable_gpu=false',
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

	subprocess.call(["bin/gn", "gen", "build/android", "--args={0}".format(argString)])
	subprocess.call(["ninja", "-C", "build/android", "skia"])

	try:
		os.mkdir("../libskia/android/")
	except OSError as exc:
		pass
	
	shutil.copyfile("build/android/libskia.a", "../libskia/android/libskia.a")

if __name__ == '__main__':
	main()
