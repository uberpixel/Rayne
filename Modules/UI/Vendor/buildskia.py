import os
import sys

args = [
	'is_official_build=true',
	'is_debug=false',
	'is_component_build=false',
	'skia_use_fontconfig=false',
	'skia_use_freetype=false',
	'skia_use_gdi=false',
	'skia_use_icu=false',
	'skia_use_libjpeg_turbo=false',
	'skia_use_libpng=false',
	'skia_use_libwebp=false',
	'skia_use_lua=false',
	'skia_use_mesa=false',
	'skia_use_piex=false',
	'skia_use_zlib=false',
	'skia_enable_gpu=false'
]

def main():
	if sys.platform == 'darwin':
		args.append('extra_cflags_cc=["-mmacosx-version-min=10.11"]')

	argString = ' '.join(str(x) for x in args)
	argument = '--args="{0}"'.format(argString)

	path = os.path.dirname(os.path.realpath(__file__))
	skiaPath = os.path.join(path, 'skia')

	if os.path.exists(skiaPath) == False:
		print('Could not find skia path, please check it out')
		return

	os.chdir(skiaPath)

	os.system('gn gen out/Rayne --args=\'{0}\''.format(argString))
	os.system('ninja -C out/Rayne skia')

if __name__ == '__main__':
	main()
