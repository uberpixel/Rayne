import os
import sys
import platform
import subprocess
import shutil
import json
import Utilities
import glob


def main():
	if len(sys.argv) < 4:
		print('Missing Argument!')
		print('Correct Usage:')
		print('python CreateRelease.py build-config.json platform (windows, linux, macos or android) type (independent, oculus, steam, pico or headless) [demo] (will add "demo" to the bundle id and name)')
		return

	with open(sys.argv[1]) as json_file:
		buildConfigData = json.load(json_file)

	if not buildConfigData:
		print("Failed to open " + sys.argv[1] + "!")
		return

	projectRootPath = os.path.dirname(sys.argv[1])
	if not projectRootPath:
		projectRootPath = ""

	platform = sys.argv[2]
	supportedPlatforms = ['windows', 'linux', 'macos', 'android', 'test']
	if not platform in supportedPlatforms:
		print('Platform (' + platform + ') not supported!')
		return

	configuration = sys.argv[3]
	supportedTypes = ['independent', 'oculus', 'steam', 'pico', 'headless', 'test']
	if not configuration in supportedTypes:
		print('Build type (' + configuration + ') not supported!')
		return

	isDemo = False
	if len(sys.argv) == 5 and sys.argv[4] == "demo":
		isDemo = True

	configName = Utilities.getSettingFromConfig(platform, "name", buildConfigData)
	configBuildDirectory = Utilities.getSettingFromConfig(platform, "build-directory", buildConfigData)
	configReleaseDirectory = Utilities.getSettingFromConfig(platform, "release-directory", buildConfigData)
	if not configName:
		print("config file is missing name!")
		return
	if not configBuildDirectory:
		print("config file is missing build-directory!")
		return
	if not configReleaseDirectory:
		print("config file is missing release-directory!")
		return

	if isDemo:
		configName += " Demo"

	sourceDirectory = os.path.join(projectRootPath, configBuildDirectory)
	sourceDirectory = os.path.join(sourceDirectory, platform+'_'+configuration)
	buildDirectory = None
	if isDemo:
		sourceDirectory += "_demo"
	if platform == 'android':
		buildDirectory = os.path.join(sourceDirectory, 'app/.cxx/cmake/release/arm64-v8a/')
		sourceDirectory = os.path.join(sourceDirectory, 'app/build/outputs/apk/release')
	elif platform == 'linux':
		sourceDirectory = os.path.join(sourceDirectory, 'Build')
	elif platform == 'windows':
		sourceDirectory = os.path.join(sourceDirectory, 'Build/Release/'+configName.replace(" ", ""))
	elif platform == 'macos':
		sourceDirectory = os.path.join(sourceDirectory, 'Build/'+configName+'/Release')
	
	if not os.path.isdir(sourceDirectory):
		print("Build does not exist: " + sourceDirectory)
		return

	destinationDirectory = os.path.join(projectRootPath, configReleaseDirectory)
	destinationDirectory = os.path.join(destinationDirectory, platform+'_'+configuration)
	if isDemo:
		destinationDirectory += "_demo"
	if os.path.isdir(destinationDirectory):
		shutil.rmtree(destinationDirectory, ignore_errors=True)

	if platform == 'windows':
		shutil.copytree(sourceDirectory, destinationDirectory, symlinks=False, ignore=shutil.ignore_patterns('*.lib', '*.exp'))
	elif platform == 'linux':
		shutil.copytree(sourceDirectory, destinationDirectory, symlinks=False, ignore=None)
	elif platform == 'macos':
		shutil.copytree(sourceDirectory, destinationDirectory, symlinks=False, ignore=shutil.ignore_patterns('.DS_STORE'))
	elif platform == 'android':
		os.makedirs(destinationDirectory)
		apkFileName = configName.replace(" ", "-").lower()+"-"+configuration+".apk"
		shutil.copy2(os.path.join(sourceDirectory, apkFileName), os.path.join(destinationDirectory, apkFileName))

		#Also copy .so files with debug symbols into the release directory
		symbolDestinationDir = os.path.join(destinationDirectory, 'symbols')
		os.makedirs(symbolDestinationDir)
		for file in glob.glob('Build/**/*.so', root_dir=buildDirectory, recursive=True):
			shutil.copy(os.path.join(buildDirectory, file), symbolDestinationDir)
		for file in glob.glob('Rayne/Build/**/*.so', root_dir=buildDirectory, recursive=True):
			shutil.copy(os.path.join(buildDirectory, file), symbolDestinationDir)

if __name__ == '__main__':
	main()
