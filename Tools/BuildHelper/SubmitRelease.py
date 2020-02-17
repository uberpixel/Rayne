import os
import sys
import platform
import urllib
import zipfile
import subprocess
import Utilities


def downloadItchIOButler(helperdir):
	butlerDirectory = os.path.join(helperdir, "Vendor")
	butlerDirectory = os.path.join(butlerDirectory, "itchio")
	butlerFile = os.path.join(butlerDirectory, 'butler')
	if platform.system() == 'Windows':
		butlerFile = os.path.join(butlerDirectory, 'butler.exe')

	if os.path.isfile(butlerFile):
		return butlerFile

	if not os.path.exists(butlerDirectory):
		os.makedirs(butlerDirectory)

	butlerDownloadURL = None
	if platform.system() == 'Darwin':
		butlerDownloadURL = 'https://broth.itch.ovh/butler/darwin-amd64/LATEST/archive/default'
	elif platform.system() == 'Windows':
		butlerDownloadURL = 'https://broth.itch.ovh/butler/windows-amd64/LATEST/archive/default'
	elif platform.system() == 'Linux':
		butlerDownloadURL = 'https://broth.itch.ovh/butler/linux-amd64/LATEST/archive/default'
	else:
		print 'Platform ' + platform.system() + ' not supported.'
		return None

	butlerZipFilePath = os.path.join(butlerDirectory, 'butler.zip')
	urllib.urlretrieve(butlerDownloadURL, butlerZipFilePath)

	zippedFile = zipfile.ZipFile(butlerZipFilePath, 'r')
	zippedFile.extractall(butlerDirectory)
	zippedFile.close()

	if os.path.exists(butlerZipFilePath):
		os.remove(butlerZipFilePath)

	os.chmod(butlerFile, 0o775)

	return butlerFile


def downloadOculusPlatformUtil(helperdir):
	oculusDirectory = os.path.join(helperdir, "Vendor")
	oculusDirectory = os.path.join(oculusDirectory, "oculus")
	utilityFile = os.path.join(oculusDirectory, 'ovr-platform-util.dms')
	if platform.system() == 'Windows':
		utilityFile = os.path.join(oculusDirectory, 'ovr-platform-util.exe')

	if os.path.isfile(utilityFile):
		return utilityFile

	if not os.path.exists(oculusDirectory):
		os.makedirs(oculusDirectory)

	utilityDownloadURL = None
	if platform.system() == 'Darwin':
		utilityDownloadURL = 'https://www.oculus.com/download_app/?id=1462426033810370'
	elif platform.system() == 'Windows':
		utilityDownloadURL = 'https://www.oculus.com/download_app/?id=1076686279105243'
	else:
		print 'Platform ' + platform.system() + ' not supported.'
		return None

	urllib.urlretrieve(utilityDownloadURL, utilityFile)
	os.chmod(utilityFile, 0o775)

	return utilityFile


def main():
	if len(sys.argv) < 4:
		print 'Missing Argument!'
		print 'Correct Usage:'
		print 'python SubmitRelease.py build-config.json platform (windows, linux, android or macos) storefront (oculus, steam, itchio) [devicetype (quest, go)]'
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
	supportedPlatforms = ['windows', 'linux', 'android', 'macos', 'test']
	if not platform in supportedPlatforms:
		print 'Platform (' + platform + ') not supported!'
		return

	storefront = sys.argv[3]
	supportedStorefronts = ['oculus', 'steam', 'itchio', 'test']
	if not storefront in supportedStorefronts:
		print 'Storefront (' + storefront + ') not supported!'
		return

	configName = Utilities.getSettingFromConfig(platform, "name", buildConfigData)
	configReleaseDirectory = Utilities.getSettingFromConfig(platform, "release-directory", buildConfigData)
	configAppInfoOculus = Utilities.getSettingFromConfig(platform, "appinfo-file-oculus", buildConfigData)
	if not configName:
		print("config file is missing name!")
		return
	if not configReleaseDirectory:
		print("config file is missing release-directory!")
		return
	if not configAppInfoOculus and storefront == "oculus":
		print("config file is missing appinfo-file-oculus, which is required for submitting to the oculus store!")
		return

	releasesDirectoryPath = os.path.join(projectRootPath, configReleaseDirectory)

	version = Utilities.getVersion(os.path.join(projectRootPath, "VERSION"))

	if storefront == "itchio":
		butlerFile = downloadItchIOButler(buildHelperPath)
		if not butlerFile:
			return

		subprocess.call([butlerFile, 'upgrade'])
		subprocess.call([butlerFile, 'login'])

		if platform == 'windows':
			subprocess.call([butlerFile, 'push', os.path.join(releasesDirectoryPath, 'windows_independent'), "slin/"+configName+":windows"])
		elif platform == 'linux':
			subprocess.call([butlerFile, 'push', os.path.join(releasesDirectoryPath, 'linux_independent'), "slin/"+configName+":linux"])
		elif platform == 'android':
			subprocess.call([butlerFile, 'push', os.path.join(releasesDirectoryPath, 'android_independent'), "slin/"+configName+":sidequest"])
		elif platform == 'macos':
			subprocess.call([butlerFile, 'push', os.path.join(releasesDirectoryPath, 'macos_independent'), "slin/"+configName+":macos"])

	elif storefront == "oculus":
		oculusUtilityFile = downloadOculusPlatformUtil(buildHelperPath)
		if not oculusUtilityFile:
			return

		deviceType = "rift"
		if platform == "android":
			deviceType = "go"
			if len(sys.argv) == 5:
				deviceType = sys.argv[4]

		with open(os.path.join(projectRootPath, configAppInfoOculus), "rb") as appInfoFile:
			appInfoData = json.load(appInfoFile)
			appInfoDevicdData = appInfoData[deviceType]
			if appInfoDevicdData:
				appID = appInfoDevicdData["app-id"]
				appSecret = appInfoDevicdData["secret"]

		if not appID or not appSecret:
			print("no app-id or secret in " + configAppInfoOculus + " for " + deviceType)
			return

		if platform == 'windows':
			directoryToUpload = os.path.join(releasesDirectoryPath, 'windows_oculus')
			directoryToUpload = os.path.join(directoryToUpload, configName)
			subprocess.call([oculusUtilityFile, 'upload-rift-build', '-a', appID, '-s', appSecret, '-d', directoryToUpload, '-l', configName + '.exe', '-c', 'alpha', '-v', version, '-P', '--pancake', '-r', '1183534128364060'])
		elif platform == 'android':
			apkToUpload = os.path.join(releasesDirectoryPath, 'android_oculus')
			apkToUpload = os.path.join(apkToUpload, configName+"-"+"oculus"+".apk")
			subprocess.call([oculusUtilityFile, 'upload-mobile-build', '--apk', apkToUpload, '-a', appID, '-s', appSecret, '-c', 'alpha'])

	elif storefront == "steam":
		print("Submitting to steam is not implemented yet.")
		#appBuildFile = 'app_build_919730_MacOS.vdf'
		#builderFile = '../Vendor/Steamworks/tools/ContentBuilder/builder_osx/steamcmd.sh'
		#if platform.system() == 'Windows':
		#	appBuildFile = 'app_build_919730_Win64.vdf'
		#	builderFile = '../Vendor/Steamworks/tools/ContentBuilder/builder/steamcmd.exe'
		#elif platform.system() == 'Linux':
		#	appBuildFile = 'app_build_919730_Linux64.vdf'
		#	builderFile = '../Vendor/Steamworks/tools/ContentBuilder/builder_linux/steamcmd.sh'

		#subprocess.call([builderFile, '+login', 'slindev', '+run_app_build_http', os.path.join(currentDirectory, appBuildFile), '+quit'])


if __name__ == '__main__':
	main()
