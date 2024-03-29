import os
import sys
import subprocess
import shutil
import Utilities
import json


def main():
	if len(sys.argv) < 4:
		print('Missing Argument!')
		print('Correct Usage:')
		print('python CreateBuildProject.py build-config.json os (windows, linux, macos, android, ios, visionos, ios_sim, visionos_sim) configuration (independent, oculus, steam, pico or headless) [demo] (will add "demo" to the bundle id and name)')
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

	operatingSystem = sys.argv[2]
	supportedOperatingSystems= ['windows', 'linux', 'macos', 'ios', 'visionos', 'ios_sim', 'visionos_sim', 'android', 'test']
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

	configBundleID = Utilities.getSettingFromConfig(operatingSystem, configuration, "bundle-id", buildConfigData)
	configName = Utilities.getSettingFromConfig(operatingSystem, configuration, "name", buildConfigData)
	configBuildDirectory = Utilities.getSettingFromConfig(operatingSystem, configuration, "build-directory", buildConfigData)
	configCmakeBuildType = Utilities.getSettingFromConfig(operatingSystem, configuration, "cmake-build-type", buildConfigData)
	configCmakeParameters = Utilities.getSettingFromConfig(operatingSystem, configuration, "cmake-parameters", buildConfigData, False)
	configIconsDirectory = Utilities.getSettingFromConfig(operatingSystem, configuration, "icons", buildConfigData)
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

	if isDemo:
		configBundleID += "_demo"
		configName += " Demo"
		configCmakeParameters += "-DRN_BUILD_IS_DEMO=ON"

	versionFilePath = os.path.join(projectRootPath, "VERSION")
	buildNumber = Utilities.getBuildNumber(versionFilePath)
	versionString = Utilities.getVersion(versionFilePath)

	buildDirectory = os.path.join(projectRootPath, configBuildDirectory)
	buildDirectory = os.path.join(buildDirectory, operatingSystem+'_'+configuration)
	if isDemo:
		buildDirectory += "_demo"
	if os.path.isdir(buildDirectory):
		shutil.rmtree(buildDirectory, ignore_errors=True)

	os.makedirs(buildDirectory)
	os.chdir(buildDirectory)

	buildconfiguration = ["-DRN_BUILD_CONFIGURATION="+configuration]
	if len(configCmakeParameters) > 0:
		buildconfiguration.extend(configCmakeParameters.split(','))
	buildType = "-DCMAKE_BUILD_TYPE="+configCmakeBuildType

	if operatingSystem == 'windows':
		cmakeCall = ['cmake', projectRootPath, '-G', 'Visual Studio 17 2022', '-A', 'x64', buildType]
		cmakeCall.extend(buildconfiguration)
		subprocess.run(cmakeCall)
	elif operatingSystem == 'linux':
		cmakeCall = ['cmake', '-G', 'Ninja', projectRootPath, buildType]
		cmakeCall.extend(buildconfiguration)
		subprocess.run(cmakeCall)
	elif operatingSystem == 'macos':
		cmakeCall = ['cmake', '-G', 'Xcode', projectRootPath]
		cmakeCall.extend(buildconfiguration)
		subprocess.run(cmakeCall)
	elif operatingSystem =='ios':
		cmakeCall = ['cmake', '-G', 'Xcode', '-DCMAKE_SYSTEM_NAME=iOS', projectRootPath]
		cmakeCall.extend(buildconfiguration)
		subprocess.run(cmakeCall)
	elif operatingSystem =='ios_sim':
		simulatorRootOutput = subprocess.run(['xcodebuild', '-version', '-sdk', 'iphonesimulator', 'Path'], capture_output=True)
		simulatorRoot = simulatorRootOutput.stdout.decode("utf-8").strip()
		cmakeCall = ['cmake', '-G', 'Xcode', '-DCMAKE_SYSTEM_NAME=iOS', '-DCMAKE_OSX_SYSROOT='+simulatorRoot, projectRootPath]
		cmakeCall.extend(buildconfiguration)
		subprocess.run(cmakeCall)
	elif operatingSystem =='visionos':
		sysRootOutput = subprocess.run(['xcodebuild', '-version', '-sdk', 'xros', 'Path'], capture_output=True)
		sysRoot = sysRootOutput.stdout.decode("utf-8").strip()
		cmakeCall = ['cmake', '-G', 'Xcode', '-DCMAKE_SYSTEM_NAME=visionOS', '-DCMAKE_OSX_SYSROOT='+sysRoot, projectRootPath]
		cmakeCall.extend(buildconfiguration)
		subprocess.run(cmakeCall)
	elif operatingSystem =='visionos_sim':
		simulatorRootOutput = subprocess.run(['xcodebuild', '-version', '-sdk', 'xrsimulator', 'Path'], capture_output=True)
		simulatorRoot = simulatorRootOutput.stdout.decode("utf-8").strip()
		cmakeCall = ['cmake', '-G', 'Xcode', '-DCMAKE_SYSTEM_NAME=visionOS', '-DCMAKE_OSX_SYSROOT='+simulatorRoot, projectRootPath]
		cmakeCall.extend(buildconfiguration)
		subprocess.run(cmakeCall)
	elif operatingSystem == 'android':
		shutil.copytree(os.path.join(buildHelperPath, "gradle-wrapper"), buildDirectory, dirs_exist_ok=True)
		subprocess.run([os.path.join(buildDirectory, 'gradlew'), 'init', '--type', 'basic', '--dsl', 'groovy', '--project-name', configName])
		Utilities.copyAndroidBuildSystem(os.path.join(buildHelperPath, "android-buildsystem"), projectRootPath, buildConfigData, configuration, isDemo)
		if configIconsDirectory:
			shutil.copytree(os.path.join(projectRootPath, configIconsDirectory), os.path.join(buildDirectory, "app/src/main/res"), dirs_exist_ok=True)
		Utilities.setGradleProperty('gradle.properties', 'projectCmakeArguments', ','.join(buildconfiguration))
		Utilities.setGradleProperty('gradle.properties', 'projectVersion', versionString)
		Utilities.setGradleProperty('gradle.properties', 'projectBuildNumber', str(buildNumber))

if __name__ == '__main__':
	main()
