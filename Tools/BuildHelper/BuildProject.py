import os
import sys
import subprocess
import shutil
import getpass
import json
import Utilities


def main():
	if len(sys.argv) < 4:
		print('Missing Argument!')
		print('Correct Usage:')
		print('python BuildProject.py build-config.json os (windows, linux, macos or android) type (independent, oculus, steam, pico or headless) [demo] (will add "demo" to the bundle id and name)')
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

	configBundleID = Utilities.getSettingFromConfig(operatingSystem, configuration, "bundle-id", buildConfigData)
	configName = Utilities.getSettingFromConfig(operatingSystem, configuration, "name", buildConfigData)
	configBuildDirectory = Utilities.getSettingFromConfig(operatingSystem, configuration, "build-directory", buildConfigData)
	configCmakeBuildType = Utilities.getSettingFromConfig(operatingSystem, configuration, "cmake-build-type", buildConfigData)
	configKeystore = Utilities.getSettingFromConfig(operatingSystem, configuration, "keystore", buildConfigData)
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
	if not configKeystore and operatingSystem == "android":
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
	buildDirectory = os.path.join(buildDirectory, operatingSystem+'_'+configuration)
	if isDemo:
		buildDirectory += "_demo"
	os.chdir(buildDirectory)

	print(buildDirectory)

	if operatingSystem == 'windows':
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
	elif operatingSystem == 'linux':
		subprocess.call(['ninja'])
	elif operatingSystem == 'macos':
		subprocess.call(['/usr/libexec/PlistBuddy', '-c', 'Set :CFBundleShortVersionString '+versionString, 'CMakeFiles/Concealed.dir/Info.plist'])
		subprocess.call(['/usr/libexec/PlistBuddy', '-c', 'Set :CFBundleVersion '+str(buildNumber), 'CMakeFiles/Concealed.dir/Info.plist'])
		subprocess.call(['xcodebuild', 'build', '-project', configName+'.xcodeproj', '-target', configName, '-configuration', configCmakeBuildType])
	elif operatingSystem == 'android':
		Utilities.setGradleProperty('gradle.properties', 'projectVersion', versionString)
		Utilities.setGradleProperty('gradle.properties', 'projectBuildNumber', str(buildNumber))

		secretsFile = Utilities.getSettingFromConfig(operatingSystem, configuration, "build-secrets", buildConfigData)
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
		#subprocess.call(['jarsigner', '-verbose', '-keystore', os.path.join(projectRootPath, configKeystore), '-storepass', storePassword, 'app/build/outputs/apk/release/app-release-unsigned.apk', 'AndroidReleaseKey', '-keypass', keyPassword])
		apkFilePath = os.path.join('app/build/outputs/apk/release', configNameFull.replace(" ", "-").lower() + "-" + configuration + ".apk")
		subprocess.call(['/Users/slin/Library/Android/sdk/build-tools/32.0.0/zipalign', '-p', '-f', '-v', '4', 'app/build/outputs/apk/release/app-release-unsigned.apk', apkFilePath])
		subprocess.call(['/Users/slin/Library/Android/sdk/build-tools/32.0.0/apksigner', 'sign', '--verbose', '--v1-signing-enabled', 'true', '--v2-signing-enabled', 'true', '--v3-signing-enabled', 'false', '--ks', os.path.join(projectRootPath, configKeystore), '--ks-pass', 'pass:' + storePassword, '--ks-key-alias', 'AndroidReleaseKey', '--key-pass', 'pass:' + keyPassword, apkFilePath])

if __name__ == '__main__':
	main()
