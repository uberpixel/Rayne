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
		print('Missing Argument!')
		print('Correct Usage:')
		print('python BuildProject.py build-config.json platform (windows, linux, macos or android) type (independent, oculus, steam or headless) [demo] (will add "demo" to the bundle id and name)')
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
		print('Platform (' + platform + ') not supported!')
		return

	configuration = sys.argv[3]
	supportedTypes = ['independent', 'oculus', 'steam', 'headless', 'test']
	if not configuration in supportedTypes:
		print('Build type (' + configuration + ') not supported!')
		return

	isDemo = False
	if len(sys.argv) == 5 and sys.argv[4] == "demo":
		isDemo = True

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

	configNameFull = configName
	if isDemo:
		configBundleID += "_demo"
		configNameFull += " Demo"

	versionFilePath = os.path.join(projectRootPath, 'VERSION')
	buildNumber = Utilities.getBuildNumber(versionFilePath)+1
	versionString = Utilities.getVersion(versionFilePath)
	Utilities.setVersionAndBuildNumber(versionFilePath, versionString, buildNumber)

	buildDirectory = os.path.join(projectRootPath, configBuildDirectory)
	buildDirectory = os.path.join(buildDirectory, platform+'_'+configuration)
	if isDemo:
		buildDirectory += "_demo"
	os.chdir(buildDirectory)

	print(buildDirectory)

	if platform == 'windows':
		vswhereOutput = subprocess.check_output([os.path.join(os.environ["ProgramFiles(x86)"], "Microsoft Visual Studio/Installer/vswhere.exe"), "-latest", "-products", "*", "-requires", "Microsoft.Component.MSBuild"])
		vswhereLines = vswhereOutput.splitlines()
		vsInstallationPath = None
		for vswhereLine in vswhereLines:
			if vswhereLine.startswith(b'installationPath: '):
				vsInstallationPath = str(vswhereLine.split(b': ', 1)[1], 'utf-8')
		if not vsInstallationPath:
			print("No visual studio installation with MSBuild found!")
			return
		msbuildSearchDirectory = os.path.join(vsInstallationPath, "MSBuild")
		msbuildPath = None
		for root, dirs, files in os.walk(msbuildSearchDirectory):
			for file in files:
				if file == "MSBuild.exe":
					msbuildPath = os.path.join(root, file)
		if not msbuildPath:
			print("MSBuild not found!")
			return
		subprocess.call([msbuildPath, configName.replace(" ", "")+'.vcxproj', '-maxcpucount', '/p:configuration='+configCmakeBuildType, '/p:platform=x64'])
	elif platform == 'linux':
		subprocess.call(['ninja'])
	elif platform == 'macos':
		subprocess.call(['/usr/libexec/PlistBuddy', '-c', 'Set :CFBundleShortVersionString '+versionString, 'CMakeFiles/Concealed.dir/Info.plist'])
		subprocess.call(['/usr/libexec/PlistBuddy', '-c', 'Set :CFBundleVersion '+str(buildNumber), 'CMakeFiles/Concealed.dir/Info.plist'])
		subprocess.call(['xcodebuild', 'build', '-project', configName+'.xcodeproj', '-target', configName, '-configuration', configCmakeBuildType])
	elif platform == 'android':
		Utilities.setGradleProperty('gradle.properties', 'projectVersion', versionString)
		Utilities.setGradleProperty('gradle.properties', 'projectBuildNumber', str(buildNumber))

		secretsFile = Utilities.getSettingFromConfig(platform, "build-secrets", buildConfigData)
		if secretsFile:
			secretsFile = os.path.join(projectRootPath, secretsFile)
			with open(secretsFile) as json_file:
				secretsDict = json.load(json_file)
				if "keystore-password" in secretsDict:
					storePassword = secretsDict["keystore-password"]
				if "keystore-key-password" in secretsDict:
					keyPassword = secretsDict["keystore-key-password"]

		if not storePassword or len(storePassword) == 0:
			print('Keystore password?')
			storePassword = getpass.getpass()

		if not keyPassword or len(keyPassword) == 0:
			print('Key password?')
			keyPassword = getpass.getpass()

		subprocess.call(['./gradlew', 'assembleRelease'])
		subprocess.call(['jarsigner', '-verbose', '-keystore', os.path.join(projectRootPath, configKeystore), '-storepass', storePassword, 'app/build/outputs/apk/release/app-release-unsigned.apk', 'AndroidReleaseKey', '-keypass', keyPassword])
		apkFilePath = os.path.join('app/build/outputs/apk/release', configNameFull.replace(" ", "-").lower() + "-" + configuration + ".apk")
		subprocess.call(['/Users/slin/Library/Android/sdk/build-tools/29.0.2/zipalign', '-f', '4', 'app/build/outputs/apk/release/app-release-unsigned.apk', apkFilePath])

if __name__ == '__main__':
	main()
