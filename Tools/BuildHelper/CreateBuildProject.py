import os
import sys
import platform
import subprocess
import shutil
import distutils.dir_util
import Utilities
import json


def main():
	if len(sys.argv) < 4:
		print 'Missing Argument!'
		print 'Correct Usage:'
		print 'python CreateBuildProject.py build-config.json platform (windows, linux, macos or android) configuration (independent, oculus, steam or gamelift)'
		return

	with open(sys.argv[1]) as json_file:
		buildConfigData = json.load(json_file)

	if not buildConfigData:
		print("Failed to open " + sys.argv[1] + "!")
		return

	buildHelperPath = os.path.dirname(sys.argv[0])
	projectRootPath = os.path.dirname(sys.argv[1])
	if not buildHelperPath:
		buildHelperPath = ""
	if not projectRootPath:
		projectRootPath = ""

	buildHelperPath = os.path.abspath(buildHelperPath)
	projectRootPath = os.path.abspath(projectRootPath)

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

	configBundleID = Utilities.getSettingFromConfig(platform, "bundle-id", buildConfigData)
	configName = Utilities.getSettingFromConfig(platform, "name", buildConfigData)
	configBuildDirectory = Utilities.getSettingFromConfig(platform, "build-directory", buildConfigData)
	configCmakeBuildType = Utilities.getSettingFromConfig(platform, "cmake-build-type", buildConfigData)
	configCmakeParameters = Utilities.getSettingFromConfig(platform, "cmake-parameters", buildConfigData, False)
	if not configBundleID:
		print("config file is missing bundle-id!")
		return
	if not configName:
		print("config file is missing name!")
		return
	if not configBuildDirectory:
		print("config file is missing build-directory!")
		return
	if not configCmakeBuildType:
		print("config file is missing cmake-build-type, using Release.")
		configCmakeBuildType = "Release"
	if not configCmakeParameters:
		configCmakeParameters = ""

	versionFilePath = os.path.join(projectRootPath, "VERSION")
	buildNumber = Utilities.getBuildNumber(versionFilePath)
	versionString = Utilities.getVersion(versionFilePath)

	buildDirectory = os.path.join(projectRootPath, configBuildDirectory)
	buildDirectory = os.path.join(buildDirectory, platform+'_'+configuration)
	if os.path.isdir(buildDirectory):
		shutil.rmtree(buildDirectory, ignore_errors=True)

	os.makedirs(buildDirectory)
	os.chdir(buildDirectory)

	buildconfiguration = "-DRN_BUILD_CONFIGURATION="+configuration + " " + configCmakeParameters
	buildType = "-DCMAKE_BUILD_TYPE="+configCmakeBuildType

	if platform == 'windows':
		subprocess.call(['cmake', projectRootPath, '-G', 'Visual Studio 15 2017 Win64', buildconfiguration, buildType])
	elif platform == 'linux':
		subprocess.call(['cmake', '-G', 'Ninja', projectRootPath, buildconfiguration, buildType])
	elif platform == 'macos':
		subprocess.call(['cmake', '-G', 'Xcode', projectRootPath, buildconfiguration])
	elif platform == 'android':
		subprocess.call(['gradle', 'init', '--type', 'basic', '--dsl', 'groovy', '--project-name', configName])
		Utilities.copyAndroidBuildSystem(os.path.join(buildHelperPath, "android-buildsystem"), '.', buildConfigData)
		Utilities.setGradleProperty('gradle.properties', 'projectCmakeArguments', buildconfiguration + " " + buildType)
		Utilities.setGradleProperty('gradle.properties', 'projectVersion', versionString)
		Utilities.setGradleProperty('gradle.properties', 'projectBuildNumber', str(buildNumber))
		Utilities.copyToFolder(os.path.join(buildHelperPath, "../../Modules/OculusMobile/Vendor/ovr_sdk_mobile_1/VrApi/Libs/Android/arm64-v8a/Release/libvrapi.so"), 'app/src/main/libs/arm64-v8a/')
		Utilities.copyToFolder(os.path.join(buildHelperPath, "../../Modules/OculusMobile/Vendor/ovr_sdk_mobile_1/VrApi/Libs/Android/arm64-v8a/Debug/libvrapi.so"), 'app/src/debug/libs/arm64-v8a/')

if __name__ == '__main__':
	main()
