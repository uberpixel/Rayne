import os
import sys
import platform
import subprocess
import shutil
import json
import Utilities


def main():
	if len(sys.argv) < 4:
		print 'Missing Argument!'
		print 'Correct Usage:'
		print 'python SubmitRelease_Steam.py build-config.json platform (windows, linux, macos or android) type (independent, oculus or steam)'
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
		print 'Platform (' + platform + ') not supported!'
		return

	configuration = sys.argv[3]
	supportedTypes = ['independent', 'oculus', 'steam', 'gamelift', 'test']
	if not configuration in supportedTypes:
		print 'Build type (' + configuration + ') not supported!'
		return

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

	sourceDirectory = os.path.join(projectRootPath, configBuildDirectory)
	sourceDirectory = os.path.join(sourceDirectory, platform+'_'+configuration)
	if platform == 'android':
		sourceDirectory = os.path.join(sourceDirectory, 'app/build/outputs/apk/release')
	elif platform == 'linux':
		sourceDirectory = os.path.join(sourceDirectory, 'Build')
	elif platform == 'windows':
		sourceDirectory = os.path.join(sourceDirectory, 'Build/Release/Concealed')
	elif platform == 'macos':
		sourceDirectory = os.path.join(sourceDirectory, 'Build/Concealed/Release')
	
	if not os.path.isdir(sourceDirectory):
		print "Build does not exist: " + sourceDirectory
		return

	destinationDirectory = os.path.join(projectRootPath, configReleaseDirectory)
	destinationDirectory = os.path.join(destinationDirectory, platform+'_'+configuration)
	if os.path.isdir(destinationDirectory):
		shutil.rmtree(destinationDirectory, ignore_errors=True)

	if platform == 'windows':
		shutil.copytree(sourceDirectory, destinationDirectory, symlinks=False, ignore=shutil.ignore_patterns('*.lib', '*.exp', '../Builds/Steam/Build/Release/Concealed/RayneOgg.dll', 'RayneBullet', 'RayneOculus', 'RayneOpenAL', 'RayneVR', 'RayneOpenVR'))
	elif platform == 'linux':
		shutil.copytree(sourceDirectory, destinationDirectory, symlinks=False, ignore=None)
	elif platform == 'macos':
		shutil.copytree(sourceDirectory, destinationDirectory, symlinks=False, ignore=shutil.ignore_patterns('.DS_STORE'))
	elif platform == 'android':
		os.makedirs(destinationDirectory)
		apkFileName = configName+"-"+configuration+".apk"
		shutil.copy2(os.path.join(sourceDirectory, apkFileName), os.path.join(destinationDirectory, apkFileName))

if __name__ == '__main__':
	main()
