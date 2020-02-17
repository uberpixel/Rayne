import os
import sys
import platform
import subprocess
import shutil
import getpass
import json
import Utilities


def main():
	if len(sys.argv) < 4:
		print 'Missing Argument!'
		print 'Correct Usage:'
		print 'python BuildProject.py build-config.json platform (windows, linux, macos or android) type (independent, oculus, steam or gamelift)'
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
	configKeystore = Utilities.getSettingFromConfig(platform, "keystore", buildConfigData)
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
	if not configKeystore and platform == "android":
		print("config file is missing keystore, this is required for android builds!")
		return

	versionFilePath = os.path.join(projectRootPath, 'VERSION')
	buildNumber = Utilities.getBuildNumber(versionFilePath)+1
	versionString = Utilities.getVersion(versionFilePath)
	Utilities.setVersionAndBuildNumber(versionFilePath, versionString, buildNumber)

	buildDirectory = os.path.join(projectRootPath, configBuildDirectory)
	buildDirectory = os.path.join(buildDirectory, platform+'_'+configuration)
	os.chdir(buildDirectory)

	print(buildDirectory)

	if platform == 'windows':
		vswhereOutput = subprocess.check_output([os.path.join(os.environ["ProgramFiles(x86)"], "Microsoft Visual Studio/Installer/vswhere.exe"), "-latest", "-products", "*", "-requires", "Microsoft.Component.MSBuild"])
		vswhereLines = vswhereOutput.splitlines()
		for vswhereLine in vswhereLines:
			if vswhereLine.startswith("installationPath: "):
				vsInstallationPath = vswhereLine.split(": ", 1)[1]
		if not vsInstallationPath:
			print("No visual studio installation with MSBuild found!")
			return
		msbuildSearchDirectory = os.path.join(vsInstallationPath, "MSBuild")
		for root, dirs, files in os.walk(msbuildSearchDirectory):
			for file in files:
				if file == "MSBuild.exe":
					msbuildPath = os.path.join(root, file)
		if not msbuildPath:
			print("MSBuild not found!")
			return
		subprocess.call([msbuildPath, configName+'.vcxproj', '/p:configuration='+configCmakeBuildType, '/p:platform=x64'])
	elif platform == 'linux':
		subprocess.call(['ninja'])
	elif platform == 'macos':
		subprocess.call(['/usr/libexec/PlistBuddy', '-c', 'Set :CFBundleShortVersionString '+versionString, 'CMakeFiles/Concealed.dir/Info.plist'])
		subprocess.call(['/usr/libexec/PlistBuddy', '-c', 'Set :CFBundleVersion '+str(buildNumber), 'CMakeFiles/Concealed.dir/Info.plist'])
		subprocess.call(['xcodebuild', 'build', '-project', configName+'.xcodeproj', '-target', configName, '-configuration', configCmakeBuildType])
	elif platform == 'android':
		Utilities.setGradleProperty('gradle.properties', 'projectVersion', versionString)
		Utilities.setGradleProperty('gradle.properties', 'projectBuildNumber', str(buildNumber))
		print 'Keystore password?'
		storePassword = getpass.getpass()
		print 'Key password?'
		keyPassword = getpass.getpass()
		subprocess.call(['./gradlew', 'assembleRelease'])
		subprocess.call(['jarsigner', '-verbose', '-keystore', os.path.join(projectRootPath, configKeystore), '-storepass', storePassword, 'app/build/outputs/apk/release/app-release-unsigned.apk', 'AndroidReleaseKey', '-keypass', keyPassword])
		subprocess.call(['/Users/slin/Library/Android/sdk/build-tools/29.0.2/zipalign', '-f', '4', 'app/build/outputs/apk/release/app-release-unsigned.apk', os.path.join('app/build/outputs/apk/release', configName+"-"+configuration+".apk")])

if __name__ == '__main__':
	main()
