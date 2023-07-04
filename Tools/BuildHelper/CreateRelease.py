import os
import sys
import subprocess
import shutil
import json
import Utilities
import glob


def main():
	if len(sys.argv) < 4:
		print('Missing Argument!')
		print('Correct Usage:')
		print('python CreateRelease.py build-config.json os (windows, linux, macos or android) type (independent, oculus, steam, pico or headless) [demo] (will add "demo" to the bundle id and name)')
		return

	with open(sys.argv[1]) as json_file:
		buildConfigData = json.load(json_file)

	if not buildConfigData:
		print("Failed to open " + sys.argv[1] + "!")
		return

	projectRootPath = os.path.dirname(sys.argv[1])
	if not projectRootPath:
		projectRootPath = ""

	operatingSystem = sys.argv[2]
	supportedOperatingSystems = ['windows', 'linux', 'macos', 'android', 'test']
	if not operatingSystem in supportedOperatingSystems:
		print('OS (' + operatingSystem + ') not supported!')
		return

	configuration = sys.argv[3]
	supportedTypes = ['independent', 'oculus', 'steam', 'pico', 'headless', 'test']
	if not configuration in supportedTypes:
		print('Build type (' + configuration + ') not supported!')
		return

	isDemo = False
	if len(sys.argv) == 5 and sys.argv[4] == "demo":
		isDemo = True

	configName = Utilities.getSettingFromConfig(operatingSystem, configuration, "name", buildConfigData)
	configBuildDirectory = Utilities.getSettingFromConfig(operatingSystem, configuration, "build-directory", buildConfigData)
	configReleaseDirectory = Utilities.getSettingFromConfig(operatingSystem, configuration, "release-directory", buildConfigData)
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
	sourceDirectory = os.path.join(sourceDirectory, operatingSystem+'_'+configuration)
	buildDirectory = None
	if isDemo:
		sourceDirectory += "_demo"
	if operatingSystem == 'android':
		buildDirectory = os.path.join(sourceDirectory, 'app/.cxx/Release/')
		sourceDirectory = os.path.join(sourceDirectory, 'app/build/outputs/apk/release')
	elif operatingSystem == 'linux':
		sourceDirectory = os.path.join(sourceDirectory, 'Build')
	elif operatingSystem == 'windows':
		sourceDirectory = os.path.join(sourceDirectory, 'Build/Release/'+configName.replace(" ", ""))
	elif operatingSystem == 'macos':
		sourceDirectory = os.path.join(sourceDirectory, 'Build/'+configName+'/Release')
	
	if not os.path.isdir(sourceDirectory):
		print("Build does not exist: " + sourceDirectory)
		return

	destinationDirectory = os.path.join(projectRootPath, configReleaseDirectory)
	destinationDirectory = os.path.join(destinationDirectory, operatingSystem+'_'+configuration)
	if isDemo:
		destinationDirectory += "_demo"
	if os.path.isdir(destinationDirectory):
		shutil.rmtree(destinationDirectory, ignore_errors=True)

	if operatingSystem == 'windows':
		shutil.copytree(sourceDirectory, destinationDirectory, symlinks=False, ignore=shutil.ignore_patterns('*.lib', '*.exp'))
	elif operatingSystem == 'linux':
		shutil.copytree(sourceDirectory, destinationDirectory, symlinks=False, ignore=None)
	elif operatingSystem == 'macos':
		shutil.copytree(sourceDirectory, destinationDirectory, symlinks=False, ignore=shutil.ignore_patterns('.DS_STORE'))
	elif operatingSystem == 'android':
		os.makedirs(destinationDirectory)
		apkFileName = configName.replace(" ", "-").lower()+"-"+configuration+".apk"
		shutil.copy2(os.path.join(sourceDirectory, apkFileName), os.path.join(destinationDirectory, apkFileName))

		#Also copy .so files with debug symbols into the release directory
		symbolDestinationDir = os.path.join(destinationDirectory, 'symbols')
		os.makedirs(symbolDestinationDir)
		for file in glob.glob('**/*.so', root_dir=buildDirectory, recursive=True):
			if not "/arm64-v8a/" in file:
				continue
			shutil.copy(os.path.join(buildDirectory, file), symbolDestinationDir)

if __name__ == '__main__':
	main()
