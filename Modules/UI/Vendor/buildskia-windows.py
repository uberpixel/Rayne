import os
import subprocess
import sys
import shutil

args = [
	'clang_win=\"C:/Program Files/LLVM\"',
	'is_official_build=true',
	'is_debug=false',
	'is_component_build=false',
	'skia_use_fontconfig=false',
	'skia_use_freetype=true',
	'skia_enable_atlas_text=false',
	'skia_use_system_freetype2=false',
	'skia_use_icu=false',
	'skia_use_system_libpng=false',
	'skia_use_system_zlib=false',
	'skia_use_system_expat=false',
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

	subprocess.call(["bin/gn", "gen", "build/windows/release", "--args={0} extra_cflags=[\"/MD\", \"-DSK_RELEASE\"]".format(argString)])
	subprocess.call(["ninja", "-C", "build/windows/release", "skia"])

	subprocess.call(["bin/gn", "gen", "build/windows/debug", "--args={0} extra_cflags=[\"/MDd\", \"-DSK_RELEASE\"]".format(argString)])
	subprocess.call(["ninja", "-C", "build/windows/debug", "skia"])

	try:
		os.makedirs("../libskia/windows/release/")
	except OSError as exc:
		pass

	try:
		os.makedirs("../libskia/windows/debug/")
	except OSError as exc:
		pass
	
	shutil.copyfile("build/windows/release/skia.lib", "../libskia/windows/release/skia.lib")
	shutil.copyfile("build/windows/debug/skia.lib", "../libskia/windows/debug/skia.lib")

if __name__ == '__main__':
	main()
