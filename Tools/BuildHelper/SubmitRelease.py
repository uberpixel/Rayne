import os
import sys
import platform
import zipfile
import subprocess
import Utilities
import json
import urllib.request
import shutil

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
		print('Platform ' + platform.system() + ' not supported.')
		return None

	butlerZipFilePath = os.path.join(butlerDirectory, 'butler.zip')
	with urllib.request.urlopen(butlerDownloadURL) as response, open(butlerZipFilePath, 'wb') as out_file:
		shutil.copyfileobj(response, out_file)

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
		print('Platform ' + platform.system() + ' not supported.')
		return None

	with urllib.request.urlopen(utilityDownloadURL) as response, open(utilityFile, 'wb') as out_file:
		shutil.copyfileobj(response, out_file)

	os.chmod(utilityFile, 0o775)

	return utilityFile

def main():
	if len(sys.argv) < 4:
		print('Missing Argument!')
		print('Correct Usage:')
		print('python SubmitRelease.py build-config.json platform (windows, linux, android or macos) storefront (oculus, steam, itchio, github) [demo] (will add "demo" to the bundle id and name)')
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
		print('Platform (' + platform + ') not supported!')
		return

	storefront = sys.argv[3]
	supportedStorefronts = ['oculus', 'steam', 'itchio', 'github', 'test']
	if not storefront in supportedStorefronts:
		print('Storefront (' + storefront + ') not supported!')
		return

	isDemo = False
	if len(sys.argv) == 5 and sys.argv[4] == "demo":
		isDemo = True

	configName = Utilities.getSettingFromConfig(platform, "name", buildConfigData)
	configName = configName.replace(" ", "-")
	configNameLower = configName.lower()
	configReleaseDirectory = Utilities.getSettingFromConfig(platform, "release-directory", buildConfigData)
	configBuildSecrets = Utilities.getSettingFromConfig(platform, "build-secrets", buildConfigData)
	configChangelog = Utilities.getSettingFromConfig(platform, "changelog", buildConfigData)
	if not configName:
		print("config file is missing name!")
		return
	if not configReleaseDirectory:
		print("config file is missing release-directory!")
		return
	if not configBuildSecrets and (storefront == "oculus" or storefront == "github"):
		print("config file is missing build-secrets, which is required for submitting to the oculus store and github!")
		return

	if isDemo:
		configNameLower += "-demo"

	releasesDirectoryPath = os.path.join(projectRootPath, configReleaseDirectory)

	version = Utilities.getVersion(os.path.join(projectRootPath, "VERSION"))

	if storefront == "itchio":
		butlerFile = downloadItchIOButler(buildHelperPath)
		if not butlerFile:
			return

		subprocess.call([butlerFile, 'upgrade'])
		subprocess.call([butlerFile, 'login'])

		appName = Utilities.getSettingFromConfig(platform, "appname-itchio", buildConfigData)
		if not appName:
			appName = configName.lower() #This does NOT include "-demo"

		if not isDemo:
			if platform == 'windows':
				subprocess.call([butlerFile, 'push', os.path.join(releasesDirectoryPath, 'windows_independent'), "slin/"+appName+":windows"])
			elif platform == 'linux':
				subprocess.call([butlerFile, 'push', os.path.join(releasesDirectoryPath, 'linux_independent'), "slin/"+appName+":linux"])
			elif platform == 'android':
				subprocess.call([butlerFile, 'push', os.path.join(releasesDirectoryPath, 'android_independent'), "slin/"+appName+":sidequest"])
			elif platform == 'macos':
				subprocess.call([butlerFile, 'push', os.path.join(releasesDirectoryPath, 'macos_independent'), "slin/"+appName+":macos"])
		else:
			if platform == 'windows':
				subprocess.call([butlerFile, 'push', os.path.join(releasesDirectoryPath, 'windows_independent_demo'), "slin/"+appName+":windows"])
			elif platform == 'linux':
				subprocess.call([butlerFile, 'push', os.path.join(releasesDirectoryPath, 'linux_independent_demo'), "slin/"+appName+":linux"])
			elif platform == 'android':
				subprocess.call([butlerFile, 'push', os.path.join(releasesDirectoryPath, 'android_independent_demo'), "slin/"+appName+":sidequest_demo"])
			elif platform == 'macos':
				subprocess.call([butlerFile, 'push', os.path.join(releasesDirectoryPath, 'macos_independent_demo'), "slin/"+appName+":macos"])

	elif storefront == "oculus":
		oculusUtilityFile = downloadOculusPlatformUtil(buildHelperPath)
		if not oculusUtilityFile:
			return

		deviceType = "rift"
		if platform == "android":
			deviceType = "quest"

		if isDemo:
			deviceType += "-demo"

		appID = None
		appSecret = None
		with open(os.path.join(projectRootPath, configBuildSecrets), "rb") as appInfoFile:
			appInfoData = json.load(appInfoFile)
			if 'oculus' in appInfoData and deviceType in appInfoData['oculus']:
				appInfoDeviceData = appInfoData['oculus'][deviceType]
				if appInfoDeviceData:
					appID = appInfoDeviceData["app-id"]
					appSecret = appInfoDeviceData["secret"]

		if not appID or not appSecret:
			print("no app-id or secret in " + configBuildSecrets + " for " + deviceType)
			return

		if platform == 'windows':
			directoryToUpload = os.path.join(releasesDirectoryPath, 'windows_oculus')
			if isDemo:
				directoryToUpload += "_demo"
			#directoryToUpload = os.path.join(directoryToUpload, configName)
			uploadCommand = [oculusUtilityFile, 'upload-rift-build', '-a', appID, '-s', appSecret, '-d', directoryToUpload, '-l', configName + '.exe', '-c', 'alpha', '-v', version, '-P', '--pancake', '-r', '1183534128364060']
			if configChangelog != None:
				with open(configChangelog, "r") as f:
					changes = f.read()
					uploadCommand.append('--notes')
					uploadCommand.append('"' + changes + '"')
			subprocess.call(uploadCommand)
		elif platform == 'android':
			releasesDirectoryPath = os.path.join(releasesDirectoryPath, 'android_oculus')
			if isDemo:
				releasesDirectoryPath += "_demo"
			apkToUpload = os.path.join(releasesDirectoryPath, configNameLower+"-"+"oculus"+".apk")
			uploadCommand = [oculusUtilityFile, 'upload-quest-build', '--apk', apkToUpload, '-a', appID, '-s', appSecret, '-c', 'alpha', '--debug_symbols_dir', os.path.join(releasesDirectoryPath, 'symbols')]
			if configChangelog != None:
				with open(configChangelog, "r") as f:
					changes = f.read()
					uploadCommand.append('--notes')
					uploadCommand.append('"' + changes + '"')
			subprocess.call(uploadCommand)

			#--debug_symbols_dir /Users/slin/Dev/Rayne/Games/GRAB/Builds/android_oculus_demo/app/.cxx/cmake/release/arm64-v8a/Build --debug-symbols-pattern "*.so"

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

	elif storefront == "github":
		
		githubRepository = None
		githubToken = None
		with open(os.path.join(projectRootPath, configBuildSecrets), "rb") as buildSecretsFile:
			buildSecretsData = json.load(buildSecretsFile)
			if 'github' in buildSecretsData:
				githubData = buildSecretsData['github']
				githubRepository = githubData["repository"]
				githubToken = githubData["token"]

		if not githubRepository or not githubToken:
			print("no github repository or token in " + configBuildSecrets)
			return

		githubToken = "token " + githubToken

		releaseInfo = None
		print("Github API: looking for an existing release for the current version")

		#Check if a release for this version already exists
		with urllib.request.urlopen("https://api.github.com/repos/" + githubRepository + "/releases") as response:
			responseJson = json.load(response)
			print("Github API: Successfully queried releases")
			for result in responseJson:
				if result["tag_name"] == "v" + version:
					releaseInfo = result
					print("Github API: Found an existing release for v" + version)
					break

		#Create a new release if not
		if not releaseInfo:
			print("Github API: No existing release for this version, creating a new one for v" + version)
			headersDict = dict()
			headersDict["Authorization"] = githubToken
			headersDict["accept"] = "application/vnd.github.v3+json"
			bodyDict = dict()
			bodyDict["tag_name"] = "v" + version
			request = urllib.request.Request("https://api.github.com/repos/" + githubRepository + "/releases", json.dumps(bodyDict).encode('ascii'), headersDict, method='POST')
			with urllib.request.urlopen(request) as response:
				releaseInfo = json.load(response)
				print("Github API: Successfully created a new release v" + version)

		if releaseInfo:
			uploadFileName = None
			apkToUpload = releasesDirectoryPath
			if platform == "android":
				apkToUpload = os.path.join(apkToUpload, platform + '_independent')
				if isDemo:
					apkToUpload += "_demo"
				uploadFileName = configNameLower+"-"+"independent"+".apk"
			else:
				uploadFileName = configNameLower + "-" + platform + "_independent"
				directoryToArchive = os.path.join(apkToUpload, platform + '_independent')
				if isDemo:
					directoryToArchive += "_demo"
				shutil.make_archive(os.path.join(apkToUpload, uploadFileName), 'zip', directoryToArchive)
				uploadFileName += ".zip"

			apkToUpload = os.path.join(apkToUpload, uploadFileName)

			#if asset with the same name already exists, remove it
			if "assets" in releaseInfo:
				for asset in releaseInfo["assets"]:
					if asset["name"] == uploadFileName:
						print("Github API: Asset with name " + uploadFileName + " already exists, deleting")
						headersDict = dict()
						headersDict["Authorization"] = githubToken
						headersDict["accept"] = "application/vnd.github.v3+json"
						request = urllib.request.Request(asset["url"], headers=headersDict, method='DELETE')
						with urllib.request.urlopen(request) as response:
							print("Github API: deleted existing release asset " + uploadFileName)


			#upload release file
			headersDict = dict()
			headersDict["Authorization"] = githubToken
			headersDict["accept"] = "application/vnd.github.v3+json"
			headersDict["Content-Type"] = "application/zip"

			with open(apkToUpload, "rb") as apkFile:
				fileContent = apkFile.read()
				url = releaseInfo["upload_url"]
				url = url.split("{")[0]
				url += "?name=" + uploadFileName
				request = urllib.request.Request(url, fileContent, headers=headersDict, method='POST')

				print("Github API: upload release asset " + uploadFileName)

				try:
				    response = urllib.request.urlopen(request)
				except urllib.error.URLError as e:
				    if hasattr(e, 'reason'):
				        print('Failed to reach a server.')
				        print('Reason: ', e.reason)
				    
				    if hasattr(e, 'code'):
				        print('The server couldn\'t fulfill the request.')
				        print('Error code: ', e.code)
				else:
					uploadInfo = json.load(response)
					print("Github API: Successfully uploaded release asset " + uploadFileName + " for github release v" + version)

if __name__ == '__main__':
	main()
