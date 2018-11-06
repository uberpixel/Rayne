import os
import subprocess
import sys

args = [
	'extra_cflags=[\"/MDd\"]',
	'is_official_build=true',
	'is_debug=false',
	'is_component_build=false',
#	'skia_use_fontconfig=false',
	'skia_use_freetype=true',
	'skia_enable_atlas_text=false',
	'skia_use_system_freetype2=false',
	'skia_use_system_jsoncpp=false',
	'skia_enable_skshaper=true',
	'skia_use_icu=true',
	'skia_use_system_icu=false',
	'skia_use_system_libpng=false',
	'skia_use_system_zlib=false',
	'skia_use_system_expat=false',
	'skia_use_libjpeg_turbo=false',
	'skia_use_libpng=false',
	'skia_use_libwebp=false',
	'skia_use_lua=false',
	'skia_use_piex=false',
	'skia_use_zlib=false',
#	'skia_enable_gpu=false',
	'skia_enable_tools=true',
	'skia_enable_skottie=false',
	'skia_enable_pdf=false',
	'clang_win=\"C:/Program Files/LLVM\"'
]

def main():
	if sys.platform == 'darwin':
		args.append('extra_cflags_cc=["-mmacosx-version-min=10.11"]')

	argString = ' '.join(str(x) for x in args)

	path = os.path.dirname(os.path.realpath(__file__))
	skiaPath = os.path.join(path, 'skia')

	if os.path.exists(skiaPath) == False:
		print('Could not find skia path, please check it out')
		return

	os.chdir(skiaPath)

	subprocess.call(["bin/gn", "gen", "build/vs2015", "--args={0}".format(argString)])
	subprocess.call(["ninja", "-C", "build/vs2015", "skia", "skiashaper"])

if __name__ == '__main__':
	main()
